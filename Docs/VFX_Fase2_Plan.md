# VFX Fase 2 — VFX nuevos de Velmara

Base: `Docs/VFX_Velmara_Palette_Plan.md` (paleta, roles, técnica validada). Todos los sistemas nuevos leen color de `NPC.NPC_VelmaraPalette.<Token>` y los trails usan `/Game/VFX/SwordTrailVFX/Materials/M_Trail_Refraction`.

Técnica extra validada en fase 1: fade de alpha con dynamic input `/Niagara/DynamicInputs/Math/OneMinusFloat` (Float enlazado a `Particles.NormalizedAge`) en `Scale Alpha`; las curvas no se pueden editar por MCP. Solo inputs LinearColor se enlazan al NPC; para Vector3 usar user params con el valor lineal.

## Patrón Niagara en C++ (reutilizar)
- `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<UNiagaraSystem>` + default con `ConstructorHelpers::FObjectFinder` (`BossProjectile.cpp:55-59`).
- `UNiagaraFunctionLibrary::SpawnSystemAtLocation` (`BossProjectile.cpp:272`), `SpawnSystemAttached` (`ShamanStaffMagicAbility.cpp:267`).
- Impactos melee: `ASword::TraceAndApplyHits` ejecuta `CurrentCueTag` (`Sword.cpp:199-205`), lo setea BP con `SetDamageType` (`Weapon.cpp:5-14`). Cues Default/Impact/Puncture en `DefaultGameplayTags.ini:60-63`.

## Paso 0 — lectura por MCP (sin escribir)
a) `BleedCueTag` en CDO de `/Game/Blueprints/GAS/Abilities/Player/GA_BloodSuck`.
b) Qué cue tag pasa cada arma del jugador a `SetDamageType` (GA_ComboBase_Generic, notifies de montajes): Katana, GreatSword, DualSword, Spear, Scythe, Sword_Hook.
c) Cómo spawnean `GCNS_Base`, `GCNS_Impact`, `GCNS_Puncture` (propiedad o grafo OnExecute).
d) Montaje/mecanismo de `GA_Dodge` (AM_Dodge, por arma, o solo curva `CF_VampireDodge`).
e) Referencers de `/Game/VFX/BatSwarm/NS_Batnado`; si GA_Vampire / GA_VampirePossessing implementan murciélagos.
f) Notify y socket de `NS_SlashTrail_Dark` en `AM_GreatSword_Attack_3Combo_1_R_Montage`.
g) Qué entrada de `DA_EffectsManager` resuelve el impacto de la pistola.

## Paquete A — assets Niagara nuevos (MCP, sin C++)
Cada asset: valida (`get_niagara_info info=validate`) y guardado.

**A1 Espectral** (`/Game/VFX/Velmara/Possession/`, `/Game/VFX/Velmara/Totem/`)
- `NS_Possess_Enter` (~0.9 s, attached al mesh del host): burst núcleo Bone ×30 2 frames; 12–20 ribbons Spectral ×5 en espiral hacia pelvis en 0.4 s; humo Void sin emisivo 0.8 s.
- `NS_Possess_Exit` (~0.8 s, en ubicación): flash Bone; anillo Spectral expandiéndose en el suelo; wisps hacia afuera que caen a Void.
- `NS_Totem_Activate` (~1.2 s, en ubicación): núcleo Bone; pilar Spectral ascendente; anillo Spectral en suelo; humo Charcoal.

**A2 Sangre / movimiento** (`/Game/VFX/Velmara/BloodSuck/`, `/Game/VFX/Velmara/Movement/`)
- `NS_BloodSuck_Pulse` (~0.5 s, en ubicación, orientado a la Normal del cue): núcleo Bone 1 frame; gotas BloodBright→Blood con gravedad en cono de la normal; neblina BloodDried.
- `NS_BloodSuck_Lunge` (~0.25 s, attached a spine_03): ribbon de refracción + 4–6 sprites BloodDried que caen a Void.
- `NS_Dodge_Afterimage` (~0.3 s, attached a pelvis): ribbon de refracción; puff de humo Charcoal/Void en los pies; sin color saturado.

**A3 Armas** (`/Game/VFX/Velmara/Weapons/`)
- `NS_Velmara_BladeTrail` (loop, attached; partir de `/Game/VFX/RefractionTrail/NS_RefractionTrail` o del NS_SwordSlash ya convertido): ribbon `M_Trail_Refraction`, vida 0.15 s; ancho = `User.BladeLength` (float), dirección = `User.BladeTipLocal` (vector local); borde BoneMuted opcional ×0.5.
- `NS_Velmara_Impact_Heavy` (~0.6 s; GreatSword/Scythe vía GCNS_Impact): flash Bone ×40 2 frames; streaks BloodBright alineados; anillo de onda Bone tenue; polvo Ash/BoneMuted.
- `NS_Velmara_Impact_Pierce` (~0.4 s; lanza/pistola vía GCNS_Puncture): flash Bone; streaks estrechos BloodBright a lo largo de −Normal; humo Charcoal.

## Paquete B — C++ (se compila al final con el editor cerrado; nunca LiveCoding; sin comentarios en Source/)
- **B1** `CustomPlayerController.h/.cpp`: `PossessionEnterEffect`, `PossessionExitEffect` (Category "Possession|VFX"), defaults a `/Game/VFX/Velmara/Possession/NS_Possess_Enter|Exit`. Enter: en `PerformPossession` rama `!bReturningToOriginalBody` junto a `OnPossessionStarted.Broadcast`, `SpawnSystemAttached` al mesh del nuevo pawn. Exit: rama de retorno dentro de `if (bWasPossessingHost)`, `SpawnSystemAtLocation` en `OriginalBody->GetActorLocation()`.
- **B2** `BloodSuckAbility.h/.cpp`: `LungeEffect` (Category "BloodSuck|Feedback"), default `/Game/VFX/Velmara/BloodSuck/NS_BloodSuck_Lunge`; spawn attached tras `LungeStartTime` en `ActivateAbility`.
- **B3** `TotemChargeComponent.h/.cpp`: `ActivationEffect` (Category "Totem | Visuals"), sin default; en `ActivatePossession` spawnear solo en la transición a activado.
- **B4** Trail de armas: `SwordData.h` `BladeTrailEffect` (Category "VFX"); en `Sword.h` helper `UpdateBladeTrail(...)` + `UPROPERTY(Transient) TObjectPtr<UNiagaraComponent> BladeTrail` (y el de la otra mano para DualSword). Crear el componente una vez (`SpawnSystemAttached` a TraceStart, sin autodestroy), setear `User.BladeLength` y `User.BladeTipLocal`, Activate/Deactivate. Llamar desde `ASword::SetWeaponCollisionEnabled` y `ADualSword::ApplyCollision` (no llama a Super). Null = sin trail (armas de enemigo).

## Paquete C1 — cableado MCP antes de compilar (depende de A y Paso 0)
- Tag `GameplayCue.Ability.BloodSuck.Bleed` + cue `GCNS_BloodSuck` (patrón GCNS_Base) que spawnea `NS_BloodSuck_Pulse` (y mantiene NS_BloodHit si hoy se usa por el decal); asignarlo a `BleedCueTag` en GA_BloodSuck.
- Dodge: notify `TimedNiagaraEffect` con `NS_Dodge_Afterimage` en pelvis durante la ventana del dash (según Paso 0d).
- `GCNS_Impact` → `NS_Velmara_Impact_Heavy`; `GCNS_Puncture` → `NS_Velmara_Impact_Pierce` (solo si Paso 0b confirma que son de armas del jugador).
- `DA_EffectsManager` (Paso 0g) → `NS_Velmara_Impact_Pierce`.

## Paquete C2 — cableado después de compilar
- `BladeTrailEffect = NS_Velmara_BladeTrail` en DA_Sword_Katana, DA_DualSword, DA_Spear, DA_Scythe, DA_Sword_Hook (GreatSword según decisión).
- `BP_Totem` → `ActivationEffect = NS_Totem_Activate`.
- PIE en `/Game/Maps/Felipe`.

## Riesgos
- B requiere compilar; las rutas default de B1/B2 deben existir antes de compilar.
- Editar notifies de montaje por MCP puede no estar soportado → manual. Nunca tocar ABP_SandboxCharacter_Manny por MCP.
- Orientación del trail depende de BoxTraceStart/End en cada BP de arma.

## Decisiones abiertas
1. Murciélagos: no hay ability en C++. ¿Diseñar la ability o solo recolorear NS_Batnado?
2. GreatSword: ¿reemplazar el notify NS_SlashTrail_Dark por el BladeTrail de C++?
3. ¿Aura en loop sobre el host durante la posesión?
