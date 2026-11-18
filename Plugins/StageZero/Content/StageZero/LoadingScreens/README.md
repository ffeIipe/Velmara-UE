# Stage Zero Loading Screens

Generated with the built-in `imagegen` workflow for Stage Zero boot and map-travel loading backgrounds.

## Assets

| File | Use |
| --- | --- |
| `T_StageZero_Loading_ShaderWarmup_4K.png` | Shader/material warmup boot slide |
| `T_StageZero_Loading_WorldPartition_4K.png` | World Partition and level streaming boot slide |
| `T_StageZero_Loading_AssetCache_4K.png` | Asset dependency and texture streaming boot slide |
| `T_StageZero_Loading_TransitionGate_4K.png` | Map-travel or final handoff background |
| `T_StageZero_Loading_ShaderWarmup_Ultrawide.png` | Ultrawide shader/material warmup boot slide |
| `T_StageZero_Loading_WorldPartition_Ultrawide.png` | Ultrawide World Partition and level streaming boot slide |
| `T_StageZero_Loading_AssetCache_Ultrawide.png` | Ultrawide asset dependency and texture streaming boot slide |
| `T_StageZero_Loading_TransitionGate_Ultrawide.png` | Ultrawide map-travel or final handoff background |

Each PNG is `3840x2160` and intentionally contains no baked text, logo, progress bar, or loading label. The lower third is kept dark enough for Stage Zero's widget text, progress bar, and tip overlay.

The ultrawide PNGs are `3840x1600` and are composed to be center-crop safe for standard `1920x1080` screens. If the loading widget uses scale-to-fill on a 16:9 display, the side extensions crop away while the primary subject, horizon, and lower-third overlay area remain usable.

## Unreal Import

Import the PNGs as textures under `/StageZero/StageZero/LoadingScreens`, then assign them in Project Settings > Plugins > Stage Zero:

- Boot Screen > Background Slides
- Loading Screen > Images > Travel Background Image

Recommended texture settings after import:

- Compression Settings: `UserInterface2D (RGBA)` for UI-only loading screens, or `Default` if memory is a concern.
- Texture Group: `UI`
- sRGB: enabled
- Mip Gen Settings: `NoMipmaps` for fixed full-screen UI, or default mipmaps if the texture is reused in 3D.

## Prompt Set

The four prompts were built around these production themes:

- Premium shader and material warmup engine bay with abstract shader ribbons.
- Open-world landscape with subtle world-partition grid and warm route traces.
- Refined asset cache vault with suspended material, terrain, and mesh samples.
- Final transition gate opening into a ready gameplay world.
