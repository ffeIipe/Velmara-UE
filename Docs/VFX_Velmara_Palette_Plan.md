# VFX — Unificación de paleta Velmara

Feedback de revisión: "Si quieren parecerse a un Hack & Slash, deberán mejorar el apartado visual". Los VFX usan colores dispares, ajenos a la estética de Velmara (referencia: UI y mockups en `UI_Raw/`, p. ej. `Mockup_V2_10_GameplayHUD_Boss.png`, `Mockup_V2_07_BloodAltar_Upgrades.png`).

## Dirección de arte

- Mundo gótico casi monocromo: negros, grises cálidos y hueso. **El rojo sangre es el único acento saturado.**
- Lavanda espectral para lo sobrenatural, dorado solar solo para el boss y lo solar.
- Hack & slash: los impactos deben **leerse** — núcleo HDR blanco hueso muy breve (1–3 frames), luego el color del rol, y caída a humo oscuro (Charcoal/Void). Nada de verdes, azules, cian o naranjas de pack.
- Emisivo: núcleo hasta ×20–50 en HDR para el flash, color de rol ×3–10; humos sin emisivo.

## Paleta (fuente: `Source/TesisUE/Private/UI/Style/VelmaraUITheme.cpp`)

Valores lineales (los que van en Niagara/materiales):

| Token | sRGB | Lineal RGB |
|---|---|---|
| Void | #0A0809 | 0.0030, 0.0024, 0.0027 |
| Charcoal | #1C1A1C | 0.0116, 0.0103, 0.0116 |
| Ash | #3A3638 | 0.0423, 0.0369, 0.0395 |
| Bone | #E8E0D0 | 0.8070, 0.7454, 0.6308 |
| BoneMuted | #A39C8E | 0.3663, 0.3325, 0.2705 |
| Blood | #8E0F14 | 0.2705, 0.0048, 0.0070 |
| BloodBright | #C21A1F | 0.5395, 0.0103, 0.0137 |
| BloodDried | #4A0A0D | 0.0685, 0.0030, 0.0040 |
| Shield | #B8C0C4 | 0.4793, 0.5271, 0.5520 |
| Spectral | #8F86A8 | 0.2747, 0.2384, 0.3916 |
| Sunlight | #D9A441 | 0.6939, 0.3712, 0.0529 |

Fuente única en editor: **`/Game/VFX/Palette/NPC_VelmaraPalette`** (Niagara Parameter Collection, un `LinearColor` por token) y, para materiales, **`/Game/VFX/Palette/MPC_VelmaraPalette`** con los mismos nombres.

## Roles de color

| Rol | Tokens |
|---|---|
| Velmara (ataques, sangre, habilidades) | BloodBright → Blood → BloodDried, núcleo Bone |
| Impacto genérico / chispas | Bone núcleo → BloodBright breve |
| Espectral (posesión, modo espectral, palancas) | Spectral, núcleo Bone, caída a Void |
| Enemigos comunes | Ash / BoneMuted (trails), telegraphs en Spectral brillante |
| Boss / solar | Sunlight, núcleo Bone, humo Charcoal |
| Escudo / bloqueo | Shield |
| Polvo / escombros | Ash, BoneMuted |
| Orbe de vida | Bone (prueba; antes verde) |
| Orbe de sangre | BloodBright |

## Inventario (VFX que se ven en juego) y asignación

### Grupo A — Velmara / combate del jugador
| Asset | Usado por | Rol |
|---|---|---|
| /Game/Effects/NewFolder/NS_SwordSlash | MainLevel | Trail de refracción |
| /Game/VFX/SlashTrailElemental/Niagara/SlashTrail/NS_SlashTrail_Dark | AM_GreatSword_Attack_3Combo_1_R_Montage | Trail de refracción |
| /Game/VFX/Blood/NS_BloodHit | GCNS_Base | Velmara (sangre) |
| /Game/VFX/Blood/NS_Blood_Effect | DA_Hook | Velmara (sangre) |
| /Game/MechanicalDamageFX/FX/NS_Sparks_Small | GCNS_Base, DA_Hook | Impacto |
| /Game/MechanicalDamageFX/FX/NS_Sparks_Medium | DA_Pistol | Impacto |
| /Game/VFX/MuzzleFlashVFX/NS_MuzzleFlash | DA_Pistol | Velmara |
| /Game/VFX/Orbs/NS_BloodOrbRibbon (+ MI_BloodOrbs_Inst) | BP_Orb_Blood, BP_Orb_Totem | Orbe sangre |
| /Game/VFX/Orbs/NS_LifeOrbRibbon (+ MI_LifeOrbs) | BP_Orb_Life | Orbe vida |
| M_BloodDecal_Floor (decal de sangre) | NS_BloodHit | Velmara (sangre) |

### Grupo B — Espectral / enemigos
| Asset | Usado por | Rol |
|---|---|---|
| /Game/VFX/LeverGlare/NS_LeverGlareFX | BP_Lever | Espectral |
| /Game/MechanicalDamageFX/FX/NS_InternalParts_Small | GCNS_Lever | Espectral |
| /Game/BloodsenseV2/NS_SpectralVanish | SpectralObjectsSubsystem (C++) | Espectral |
| /Game/BloodsenseV2/M_SpectralDissolve, M_SpectralDecay_Overlay | SpectralObjectsSubsystem (C++) | Espectral |
| /Game/SlashTrail_SoftTofu/Niagara/Basic/NS_SlashTrail_Basic | montajes AM_Enemy* / AM_EnemyPickaxe* | Enemigo |
| /Game/VFX/ShamanCharge/NS_ShamanCharge | GA_ShamanStaffMagic, SkeletonMageBlinkAbility (C++) | Enemigo telegraph |
| /Game/VFX/ShamanCharge/NS_ShamanTell | GA_ShamanStaffMagic | Enemigo telegraph |
| /Game/SlashTrail_SoftTofu/Niagara/Dark/NS_Hit_Dark_Once | GA_SkeletonMage_Blink/Reanimate **y** BossProjectile (C++) | Espectral (ver conflicto) |

### Grupo C — Boss / solar / entorno
| Asset | Usado por | Rol |
|---|---|---|
| /Game/VFX/Aura/NS_BossAura, MI_BossAuraShield | BossEnemy (C++) | Solar |
| /Game/SlashTrail_SoftTofu/Niagara/Dark/NS_SlashTrail_Dark_Loop | BossProjectile (C++) | Solar |
| NS_Hit_Solar_Once (duplicado nuevo de NS_Hit_Dark_Once) | BossProjectile (C++) | Solar |
| /Game/VFX/NS_SunSmoke | GCNA_SunBurning | Solar |
| /Game/Effects/ParticleSystems/Dust/NS_Dust | BossLightShaft (C++) | Polvo |
| /Game/MechanicalDamageFX/FX/NS_ImpactDebris_Large, _Medium | BossLightShaft, BossCeilingBreakComponent (C++) | Escombros |
| /Game/Effects/ParticleSystems/Fire/NS_TorchFire | BP_Torch | Entorno (fuego cálido, desaturar hacia Sunlight) |

**Conflicto:** `NS_Hit_Dark_Once` lo comparten el SkeletonMage (espectral) y el proyectil del boss (solar). Se duplica a `NS_Hit_Solar_Once` y se reapunta la ruta en `BossProjectile.cpp`; el original queda espectral.

## Técnica validada (spike NS_BloodHit)

- Paleta: `MPC_VelmaraPalette` es la fuente; `NPC_VelmaraPalette` se sincroniza desde ella (`source_material_collection`). Variables Niagara: `NPC.NPC_VelmaraPalette.<Token>`. Para cambiar un token, editar el MPC.
- Enlazar color: `manage_effect.edit_niagara_system` `edit=bind_parameter`, `systemPath`, `emitterName`, `parameterName="InitializeParticle.Color"` (o el input de color que tenga el módulo), `sourceBinding="NPC.NPC_VelmaraPalette.<Token>"`, `save=true` en la última llamada.
- Intensidad/valores: `edit=set_parameter_value` con `parameters:[{parameterName:"ScaleColor.Scale RGB", parameterValue:[x,y,z]}]`.
- Materiales: nodo `CollectionParameter` (MPC_VelmaraPalette, token) multiplicando la máscara grayscale.
- Inspección: `manage_effect.get_niagara_info`; validación con `info=validate`.
- `execute_python` y otras acciones con consent piden un nonce nuevo: `describe` antes de cada `execute`. Python necesita `import unreal`.
- NPC no admite añadir parámetros desde Python (propiedad protegida): siempre vía MPC.
- `NS_Hit_Solar_Once` ya está duplicado en `/Game/VFX/Boss/` y `BossProjectile.cpp` ya apunta a él (falta compilar C++).

## Reglas de implementación

1. Editar los assets en su lugar (decisión del equipo). Solo se duplica para el conflicto de arriba.
2. Cada color de partícula/emisivo pasa a leer de `NPC_VelmaraPalette` (o `MPC_VelmaraPalette` en materiales). Si un color está horneado en una textura, desaturar con el material (grayscale × color del token) en vez de repintar texturas.
3. "Feel": ajustar intensidad HDR, escala inicial, vida y curvas de alpha para que los impactos sean más secos y legibles. No cambiar spawn/duración de gameplay ni sockets.
4. No usar `LiveCoding.Compile` (crashea el editor). Los cambios de C++ se compilan con el editor cerrado o se reportan para que los compile el equipo.
5. Guardar cada asset modificado y validarlo (`get_niagara_info` con `info=validate`).

## Trails de espada (decisión del equipo)

Nada "fancy" de color: el trail es un **swoosh de aire**. Una cinta (ribbon) casi invisible que **refracta/distorsiona** lo que hay detrás, como si el espacio por donde pasó la hoja quedara deformado un instante.

- Material translúcido con Refraction/offset de normales (sin color base, opacidad muy baja), máscara que se afina en la punta y se desvanece en ~0.1–0.2 s.
- Como mucho un borde finísimo Bone/BoneMuted de baja intensidad en el filo para que se lea en fondos planos. Sin rojos ni glow en el trail; el rojo queda para el impacto y la sangre.
- Base reutilizable ya en el proyecto: `/Game/VFX/RefractionTrail/NS_RefractionTrail`, `/Game/VFX/SwordTrailVFX/Materials/M_Trail_Refraction`.
- Aplica a los trails del jugador (`NS_SwordSlash`, `NS_SlashTrail_Dark` del GreatSword). Los trails de enemigos (`NS_SlashTrail_Basic`) siguen la misma idea, más tenues y en Ash.

## Fase 2 (aprobada) — VFX nuevos

Habilidades de Velmara que hoy **no tienen Niagara**: posesión, blood suck, enjambre de murciélagos, dash/esquiva, y trails/impactos del resto de armas del jugador. Se crean después de la fase 1 con esta paleta y el trail de refracción.
