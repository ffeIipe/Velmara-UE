# Overlay System (estilo ALS) — Guía para agentes

Objetivo: el cuerpo de abajo lo sigue moviendo GASP (Motion Matching). Arriba del torso se aplica **una sola pose por arma**, sacada del Idle del pack de Grruzam, con pesos por zona. Para agregar un arma nueva solo se crea un Data Asset; no se toca C++ ni ningún ABP.

## Qué hay hoy y por qué falla (revisado el 2026-11-21)

- `ASword::Equip` llama a `LinkAnimClassLayers(SwordData->AnimLayer)` y le pasa `OverlayPose` y `OverlayPoseTime` a `UWeaponOverlayAnimInstance`.
- `ABP_Weapon_GreatSword` es hija de `UWeaponOverlayAnimInstance` e implementa `ALI_Weapon / WeaponGraph`. **No lee `OverlayPose`**: su EventGraph está vacío y su pose sale de un `WeaponStateMachine`. Por eso lo que manda C++ se pierde.
- En `ABP_SandboxCharacter_Manny`, el nodo `ALI_Weapon - WeaponGraph` pasa por un `Blend Poses by bool` controlado por un Property Access. Ese bool se pone en true recién al atacar, y por eso **la pose aparece con el primer golpe**. Además, los pines `IsEquipped` e `IsAttacking` del nodo de la capa están sin conectar (valen false siempre).
- Solución: se elimina el selector por bool. La capa se llama siempre. Si no hay arma, la capa sin enlazar devuelve la pose de entrada tal cual; si hay arma, aplica el overlay. Así funciona Lyra/ALS, y el bug desaparece con el propio diseño.

## Decisiones de diseño (cerradas, no reabrir)

| Tema | Decisión |
|---|---|
| Piernas y pelvis | 100% GASP |
| Columna y cabeza | **Aditivo en mesh space**: (pose del arma − pose base neutra) sumado sobre GASP. El torso conserva el balanceo de la locomoción |
| Brazos | **Override en local space** con la pose del arma, para que el arma quede firme. Peso por brazo |
| Mano izquierda | Two Bone IK hacia un socket del arma (`LeftHandIK`), con su propio peso |
| Pesos | Por arma, en el Data Asset: `SpineWeight`, `HeadWeight`, `ArmLeftWeight`, `ArmRightWeight`, `LeftHandIKWeight` |
| Montajes (ataques, parkour) | En el ABP de GASP, `DefaultSlot` va **antes** de la capa. Por eso `OverlayAlpha` se multiplica por `1 - GetSlotMontageGlobalWeight("DefaultSlot")`: el overlay se apaga mientras suena un montaje de cuerpo entero |
| IK mano izquierda | Las dos manos cuelgan de `spine_05` y los brazos se pisan en local space, así que la separación entre las manos del idle se conserva sola. La IK (`LeftHandIKWeight`, en 0 por defecto) solo hace falta si el mesh del arma no coincide con el agarre del pack; en ese caso hay que agregar el socket `LeftHandIK` |
| Parkour | Además, `OverlayAlpha` baja a 0 mientras el movimiento está en `MOVE_Flying` (GASP usa ese modo durante el traversal) |
| ABP de la capa | **Uno solo, genérico**: `ABP_WeaponOverlay`. Las armas no tienen ABP propio |
| Alcance | Primero el jugador. Después, enemigos que usen el esqueleto de Manny (fase 6) |

## Reglas para todos los agentes

- No escribir comentarios en `Source/`.
- C++: no usar Live Coding (crashea). Guardar, cerrar el editor y compilar:
  `C:/UnrealVersions/UE_5.5/Engine/Build/BatchFiles/Build.bat TesisUEEditor Win64 Development -Project="I:/UnrealProjects/TesisUE/TesisUE.uproject" -WaitMutex -Log=I:/UnrealProjects/TesisUE/Saved/Logs/UBT_Overlay.log`
- Los Blueprints se editan por el MCP `unreal`, que exige el editor abierto **antes** de iniciar la sesión del agente. Por eso las fases C++ (1–2) se hacen con el editor cerrado y las fases BP (3–5) con el editor abierto.
- La sesión principal revisa el diff y el log de compilación de cada `coder` antes de pasar a la fase siguiente.

---

## Fase 1 — Datos del overlay en `WeaponData` (coder, C++)

**Archivos:** `Public/DataAssets/Items/Weapons/WeaponData.h`, `Public/DataAssets/Items/Weapons/SwordData.h`.

1. En `WeaponData.h`, crear el struct `FWeaponOverlaySettings` (BlueprintType) con estos campos:
   - `TObjectPtr<UAnimSequence> OverlayPose`
   - `float OverlayPoseTime = 0` (ClampMin 0)
   - `float SpineWeight = 1`, `float HeadWeight = 0.5`, `float ArmLeftWeight = 1`, `float ArmRightWeight = 1`, `float LeftHandIKWeight = 0` (todos con ClampMin 0, ClampMax 1)
   - `FName LeftHandIKSocket = "LeftHandIK"`
2. En `UWeaponData`, agregar `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overlay") FWeaponOverlaySettings Overlay;` al lado de `AnimLayer`. El overlay vive en la clase base para que lo use cualquier tipo de arma, no solo las espadas.
3. Borrar `OverlayPose` y `OverlayPoseTime` de `SwordData.h`.

**Hecho cuando:** compila y no queda ninguna referencia a `SwordData->OverlayPose`.
**Aviso:** `DA_Sword` y `DA_Sword_Great` pierden los valores que tenían en esos campos; se vuelven a cargar en la fase 5.

## Fase 2 — `UWeaponOverlayAnimInstance` y `ASword` (coder, C++)

**Archivos:** `Public/Entities/WeaponOverlayAnimInstance.h` (+ crear `Private/Entities/WeaponOverlayAnimInstance.cpp`) y `Private/Items/Weapons/Sword.cpp`.

1. Contenido de `UWeaponOverlayAnimInstance`:
   - `BlueprintReadOnly FWeaponOverlaySettings Settings`
   - `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UAnimSequence> BasePose`: la pose neutra de GASP que se resta para armar el aditivo. Se asigna en los Class Defaults del ABP.
   - `BlueprintReadOnly float OverlayAlpha = 0`
   - `BlueprintReadOnly FVector LeftHandIKLocation`: posición del socket del arma en el espacio del hueso `hand_r`.
   - `BlueprintReadOnly float EffectiveLeftHandIKWeight`
   - `UPROPERTY(EditDefaultsOnly) float OverlayBlendSpeed = 8`
   - `void ApplyOverlay(const FWeaponOverlaySettings& InSettings, const UMeshComponent* WeaponMesh)`. Copia los settings. Si `WeaponMesh` tiene el socket `LeftHandIKSocket`, calcula `WeaponMesh->GetSocketTransform(Socket).GetRelativeTransform(OwnerMesh->GetSocketTransform("hand_r")).GetLocation()`; si no lo tiene, la IK queda en peso 0. Ese valor es constante, porque el arma está pegada rígida a la mano derecha.
   - `NativeUpdateAnimation`: el target vale 0 si el `ACharacter` dueño está en `MOVE_Flying`, y 1 en cualquier otro caso. Después, `OverlayAlpha = FMath::FInterpTo(OverlayAlpha, Target, Dt, OverlayBlendSpeed)` y `EffectiveLeftHandIKWeight = Settings.LeftHandIKWeight * OverlayAlpha`. Como `OverlayAlpha` arranca en 0, al equipar entra suave.
2. En `ASword::Equip`, reemplazar las dos asignaciones actuales por `Overlay->ApplyOverlay(SwordData->Overlay, <mesh del arma>)`. Mantener `LinkAnimClassLayers` y `UnlinkAnimClassLayers` como están.

**Hecho cuando:** compila. El editor abre sin errores en `ABP_Weapon_GreatSword` (le van a faltar las variables viejas; se corrige en la fase 3).

## Fase 3 — `ABP_WeaponOverlay`, la capa genérica (coder, MCP con el editor abierto)

1. Renombrar `ABP_Weapon_GreatSword` a `/Game/Blueprints/Entities/ABP_WeaponOverlay` (con fixup de redirectors), o crearla nueva con padre `WeaponOverlayAnimInstance` que implemente `ALI_Weapon`. Antes de rehacer el grafo, guardar el nodo `Copy Bone ik_hand_gun → weapon_r` para conservarlo.
2. En los Class Defaults, `BasePose` = el idle neutro de GASP con el que se armó la locomoción (buscar un `*Idle*` en las animaciones de GASP/UEFN Mannequin; usar el frame 0).
3. Grafo `WeaponGraph`, en este orden:
   1. `Input Pose` → Save cached `Locomotion`.
   2. Sequence Evaluator (`Sequence` = `Settings.OverlayPose` como pin, `ExplicitTime` = `Settings.OverlayPoseTime`) → `OverlayLS`.
   3. Sequence Evaluator (`BasePose`, tiempo 0) → `BaseLS`.
   4. `Make Dynamic Additive` (Base = `BaseLS`, Additive = `OverlayLS`, Mesh Space = true) → `OverlayAdditive`.
   5. `Apply Mesh Space Additive` (Base = `Locomotion`, Additive = `OverlayAdditive`, Alpha = `OverlayAlpha`) → `UpperAdditive`.
   6. Columna: `Layered blend per bone` (Base = `Locomotion`, Blend0 = `UpperAdditive`, rama `spine_01`, weight = `Settings.SpineWeight`, Mesh Space Rotation Blend = true).
   7. Cabeza: `Layered blend per bone` (Base = resultado del paso 6, Blend0 = `Locomotion`, rama `neck_01`, weight = `1 - Settings.HeadWeight`).
   8. Brazos: `Layered blend per bone` (Base = resultado del paso 7, Blend0 = `OverlayLS` con rama `clavicle_l` y weight = `Settings.ArmLeftWeight * OverlayAlpha`, Blend1 = `OverlayLS` con rama `clavicle_r` y weight = `Settings.ArmRightWeight * OverlayAlpha`, local space, es decir Mesh Space Rotation Blend = false).
   9. Local To Component → `Two Bone IK` (IKBone `hand_l`, Effector Location Space = BoneSpace, Effector Target = `hand_r`, Effector Location = `LeftHandIKLocation`, Joint Target `lowerarm_l` en ParentBoneSpace, `bMaintainEffectorRelRot` = true, Alpha = `EffectiveLeftHandIKWeight`) → Component To Local → Output Pose. **No** usar el `Copy Bone ik_hand_gun → weapon_r` heredado: en las animaciones de Grruzam `ik_hand_gun` no sigue a la mano y deja el arma lejos de ella (`weapon_r` ya cuelga de `hand_r`).
4. Borrar el `WeaponStateMachine`, los Property Access y los bools viejos.

**Hecho cuando:** el ABP compila sin warnings y la vista previa con un `OverlayPose` asignado a mano muestra la pose de arriba sobre el idle.

## Fase 4 — Conectar la capa en `ABP_SandboxCharacter_Manny` (coder, MCP)

1. Borrar el `Blend Poses by bool` (`AnimGraphNode_BlendListByBool_2`) y su Property Access. Conectar `ALI_Weapon - WeaponGraph` directo entre el cached pose `GASP - Locomotion` y lo que venía después.
2. Comprobar que la implementación por defecto de `WeaponGraph` dentro de este ABP sea `Input Pose → Output Pose` (passthrough). Es lo que corre cuando no hay arma enlazada.
3. Comprobar que el `DefaultSlot` de los montajes esté **después** de la capa. Si no lo está, moverlo.
4. Quitar las entradas `IsEquipped` e `IsAttacking` de `ALI_Weapon` (ya no se usan) y recompilar sus implementadores.

**Hecho cuando:** compila, y sin arma la locomoción se ve igual que antes (no aparece ninguna pose rara).

## Fase 5 — Configurar las armas (coder, MCP)

1. En `DA_Sword` (katana) y `DA_Sword_Great`: `AnimLayer` = `ABP_WeaponOverlay_C`. Cargar `Overlay.OverlayPose` con el Idle de Grruzam de cada arma y `OverlayPoseTime` = un frame estable del idle.
2. Agregar el socket `LeftHandIK` al mesh de cada arma, en el punto del mango donde va la mano izquierda (mirar el idle de Grruzam como referencia). Empezar con `LeftHandIKWeight` = 1 en la GreatSword y en la katana si se usa a dos manos.
3. Pesos iniciales: Spine 1, Head 0.5, brazos 1. Ajustar en PIE.

**Hecho cuando:** en PIE, al equipar, la pose entra en unos 0.15 s **sin tener que atacar**. Caminar y correr mueven las piernas con GASP y mantienen la pose arriba. Atacar reproduce el montaje completo y al terminar vuelve a la pose. El parkour se ve limpio (sin arma pegada al torso). Al enfundar, se vuelve a la locomoción normal.

## Fase 6 — Enemigos (después; un enemigo piloto)

1. Elegir un enemigo con esqueleto de Manny y un arma con `UWeaponData`.
2. Agregar el nodo de capa `ALI_Weapon - WeaponGraph` en su ABP, antes del slot de montajes, con el mismo passthrough por defecto.
3. Si su arma no es `ASword`, mover el bloque Link/ApplyOverlay/Unlink de `ASword` a la clase base del arma, para que cualquier arma lo herede.

**Hecho cuando:** el enemigo piloto sostiene su arma con la pose de su DA y ataca igual que antes.

---

## Agregar un arma nueva (lo que queda al final)

1. Crear el DA del arma con `AnimLayer = ABP_WeaponOverlay_C` y elegir la pose de su idle.
2. Agregar el socket `LeftHandIK` al mesh si se usa a dos manos.
3. Ajustar los pesos. Nada más.

## Fuera de alcance por ahora

- Animaciones de equipar y desenfundar: van como montaje en un slot de torso, por separado.
- Poses distintas por gait (walk, run, sprint) y el "arm sway" aditivo de ALS. Se agregan después como campos opcionales en `FWeaponOverlaySettings`, si la pose única se ve rígida al correr.

## Piernas en quieta (`IdleLegsWeight`)

- `FWeaponOverlaySettings::IdleLegsWeight` (0–1, 0 por defecto). Cuando el personaje está quieto en el piso (velocidad 2D < `IdleSpeedThreshold`), el cuerpo entero hace blend hacia el idle del pack (`OverlayPose`), que se reproduce en loop. Al moverse, el peso baja a 0 con `IdleLegsBlendSpeed`: vuelven las piernas de GASP y queda el overlay de torso y brazos.
- Peso efectivo = `IdleLegsWeight × alpha de quieta × OverlayAlpha`, así que también se apaga durante montajes y parkour.
- En el ABP es el último `Layered blend per bone` (rama `pelvis`, local space) antes de la IK, con un `Sequence Player` cuya `Sequence` sale de `Settings.OverlayPose`.
- Hace falta toda la rama `pelvis` porque las posturas del pack (la gran espada, sobre todo) mueven y giran la pelvis. Copiar solo los muslos deja una pierna levantada.
- Katana y gran espada están en 1.

## Armas de entidades con socket propio (ej. bastón del Shaman)

- Todas las entidades llevan el arma como `ChildActor` con una clase hija de `BP_Base_Sword` (`ASword`). Su `USwordData` otorga las habilidades (`AbilitiesToGrant`), así que al poseer a la entidad el jugador también las tiene.
- `AWeapon::Equip` pega el arma a `GetEquipSocketName()`. `ASword` devuelve `SwordData->CustomInSocketName`, o `RightHandSword` si está vacío.
- Bastón del Shaman: `BP_ShamanStaff` + `DA_Staff_Shaman` (`CustomInSocketName = RightHandStaff`, `GA_ShamanStaffMagic`, overlay con `MG_AS_Idle_Combat_Seq`). `RightHandStaff` está en `SK_Mannequin`, sobre `hand_r`, y reproduce el agarre del pack Mage. El agarre se ajusta moviendo el socket, no el mesh.
- `UShamanStaffMagicAbility` busca el socket `StaffTip` en el avatar y en los actores pegados a él (el arma equipada).
- `AEntity::InnateWeaponOverlay` queda disponible para entidades sin arma de inventario; hoy ninguna lo usa.
