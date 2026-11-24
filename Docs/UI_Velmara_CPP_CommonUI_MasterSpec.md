# VELMARA — Especificación Técnica Maestra C++ & Common UI (Prompt para el Agente Constructor)

> **Cómo usar este documento:**
> Copiá el bloque de la **Sección 1 (Prompt Disparador)** y pegáselo directamente al agente de código en este repositorio (`i:\UnrealProjects\TesisUE`). Ese prompt le ordena leer esta especificación completa y ejecutar la reconstrucción total de la UI en C++ por fases compilables, respetando `CLAUDE.md` y conectando todos los sistemas reales del proyecto.

---

## 1. Prompt Disparador (Copiar y Pegar al Agente Programador)

```text
Necesito que reconstruyas de forma integral toda la UI/UX de Velmara en C++ usando Common UI, siguiendo al pie de la letra la especificación técnica en `Docs/UI_Velmara_CPP_CommonUI_MasterSpec.md` y las referencias visuales en `Docs/UI_Velmara_Mockups_V2_PromptKit.md` (más los mockups generados en `UI_Raw/` si están presentes).

Reglas innegociables antes de tocar una sola línea:
1. Leé primero `CLAUDE.md` y `Docs/UI_Velmara_CPP_CommonUI_MasterSpec.md` completo.
2. PROHIBIDO escribir comentarios (`//` o `/* */`) dentro de cualquier archivo bajo `Source/` (regla estricta de `CLAUDE.md`).
3. NO vuelvas a parchar árboles de widgets viejos (`WBP_*`) con scripts cosméticos ni uses `UComboBoxString` desplegables en ningún menú. Todos los widgets nuevos deben construir su jerarquía y lógica en C++ (`Source/TesisUE/Public/UI/` y `Source/TesisUE/Private/UI/`) sobre `CommonUI` + `CommonInput` + `AudioModulation`, reutilizando las 77 texturas ya importadas en `/Game/UI/Textures/`, las fuentes en `/Game/Fonts/` (`Forum_Font`, `Cinzel_Font`, `CARDO-*_Font`) y los datos de input en `/Game/Blueprints/HUD/InputPrompts/BP_VelmaraInputData`.
4. Mantené compatibilidad total con los mapas y Blueprints existentes mediante subclases puente o reasignación limpia de `WidgetClass` (`HUD_Velmara`, `CustomPlayerController`, `VelmaraGameInstance`, `ABossEnemy`, `UInventoryComponent`, `ABloodAltar`, `WBP_PromptPopUp`, `BP_Shop`).
5. Ejecutá el trabajo estrictamente en las 6 fases indicadas en la Sección 9 de `Docs/UI_Velmara_CPP_CommonUI_MasterSpec.md`, compilando el módulo C++ al cierre de cada fase para garantizar cero errores de compilación y cero regresiones.
```

---

## 2. Diagnóstico del Estado Actual (Qué NO repetir)

El intento anterior (`Tools/ue_phase3_*.py`) estilizó superficialmente algunos `WBP_*` existentes sin reemplazar su lógica Blueprint, dejando estos fallos críticos que esta arquitectura C++ debe erradicar:

1. **Desplegables viejos en Ajustes (`WBP_OptionsMenu`):** Se dibujaron chevrons `< >` decorativos alrededor de filas que adentro seguían teniendo `UComboBoxString` con listas en cascada superpuestas. **Solución:** Cero `UComboBoxString`; usar únicamente `UVelmaraSettingCarouselRow` (`< VALOR >` inline) y `UVelmaraSettingSliderRow`.
2. **Sonido muerto y `USettingsSaveGame` incompleto:** `SoundTab` no tenía lógica y `USettingsSaveGame` solo guardaba gráficos, pese a que ya existen los buses en `/Game/Audio/Pato/`. **Solución:** Ampliar `USettingsSaveGame` + `UVelmaraGameInstance` y conectar `AudioModulation` (`CB_Master_Vol`, `CB_Music_Vol`, `CB_SFX_Vol`) y `SoundClasses` (`SC_Master`, `SC_Music`, `SC_SFX`, `SC_UI`).
3. **Menú Principal roto (`WBP_MainMenu`):** El `OnHover` no respondía visualmente, el botón `Ajustes` abría por error `WBP_Menu_LoadGameSlots_C`, y `Nueva Partida` / `Cargar Partida` abrían pantallas de slots con recuadros y fuentes viejas. **Solución:** `UVelmaraButtonBase` unifica Hover de mouse y Focus de Gamepad en C++, y `UVelmaraSaveSlotsWidget` reemplaza las pantallas de ranuras.
4. **Cartel de Consejos intrusivo y viejo (`/Game/Blueprints/Tutorial/WBP_PromptPopUp`):** Sigue usando un marco dorado redondeado, botón gris `DONE` y pausa la partida. **Solución:** Reemplazar por `UVelmaraTutorialToastWidget` en la capa `Toast` **sin pausar el juego** ni robar el control, anclado al lateral derecho con auto-avance/tecla rápida y soporte de iconos dinámicos + texto enriquecido.
5. **Altar de Sangre con estilo viejo (`UpgradeUIStyle.h`):** `UAttributeTreeMenuWidget` y `UWeaponUpgradeMenuWidget` ya están en C++, pero dibujan cajas redondeadas doradas. **Solución:** Migrar `UpgradeUIStyle.h` y `UUpgradeMenuWidgetBase` al tema visual de Velmara e integrarlos en Common UI.
6. **Dependencias faltantes en `TesisUE.Build.cs`:** `CommonUI`, `CommonInput` y `AudioModulation` están activados en `TesisUE.uproject`, pero faltan en `Source/TesisUE/TesisUE.Build.cs`.

---

## 3. Reglas de Compilación, Módulos y Restricciones

- **Regla #1 (`CLAUDE.md`):** **CERO comentarios en `Source/`** (ni `//`, ni `/* */`, ni bloques Doxygen en `.h` o `.cpp`).
- **Actualizar `Source/TesisUE/TesisUE.Build.cs`:**
  Agregar a `PublicDependencyModuleNames` / `PrivateDependencyModuleNames`:
  - `"CommonUI"`
  - `"CommonInput"`
  - `"AudioModulation"`
  - `"DeveloperSettings"`
  - `"ApplicationCore"`
  - `"MoviePlayer"` *(opcional si se usa en pantalla de carga asíncrona)*
- **Geometría Estirable vs Arte Fijo:**
  - Ninguna textura con ruido o detalle orgánico puede estirarse.
  - Si hace falta generar alguna máscara geométrica nueva o procesar `T_UI_Logo_Velmara` remasterizado desde `UI_Raw/`, extender `Tools/ui_texture_pipeline.py` (PIL + NumPy 4x supersampling) e importar con `Tools/ue_phase2_import.py` (`TEXTUREGROUP_UI`, `TC_EDITOR_ICON` / `UserInterface2D`, `TMGS_NO_MIPMAPS`, `sRGB=true`, `NeverStream=true`, `TA_CLAMP`).

---

## 4. Sistema de Diseño Centralizado en C++ (`FVelmaraUITheme`)

Crear `Source/TesisUE/Public/UI/Style/VelmaraUITheme.h` y `.cpp` como fuente única de verdad para colores, tipografías, márgenes 9-slice y rutas de texturas.

### 4.1 Tokens de Color (`FLinearColor::FromSRGBColor`)

| Token | Hex sRGB | Uso |
|---|---|---|
| `Void` | `#0A0809` | Relleno de paneles (`Alpha 0.80`), overlays de pausa/modal (`Alpha 0.72`) |
| `Charcoal` | `#1C1A1C` | Fondo de filas de opciones (`Alpha 0.70`), slots, tracks de barras |
| `Ash` | `#3A3638` | Bordes inactivos (`1px`), separadores tenues, elementos bloqueados |
| `Bone` | `#E8E0D0` | Texto principal, texto en hover/focus, brackets `L`, iconos activos |
| `BoneMuted` | `#A39C8E` | Texto normal de botones no enfocados, descripciones secundarias |
| `Blood` | `#8E0F14` | Degradado horizontal de hover/focus, relleno principal de barra de vida |
| `BloodBright` | `#C21A1F` | Borde activo de fila/slot seleccionado, chevrons activos, énfasis, nombre de hablante |
| `BloodDried` | `#4A0A0D` | Barra fantasma de daño diferido, fondo sutil de fila/slot activo (`Alpha 0.45`), viñeta de muerte |
| `Shield` | `#B8C0C4` | Barra de escudo |
| `Spectral` | `#8F86A8` | Barra/pips de energía espectral y posesión, resaltado de objetos interactuables |
| `Sunlight` | `#D9A441` | Advertencia de exposición solar |

### 4.2 Tipografías Ya Importadas (`/Game/Fonts/`)

| Uso | Asset en UE | Tracking (`LetterSpacing`) | Tamaños base (1080p, `FontDPI=72`) |
|---|---|---|---|
| Display / Headers / Botones / Filas / Tabs | `/Game/Fonts/Forum_Font` *(fallback `/Game/Fonts/Cinzel_Font`)* | `140` a `260` (siempre MAYÚSCULAS en títulos/opciones) | Display `64`, H1 `42`, H2 `28`, Fila/Botón `20–22`, Micro-label `14` |
| Cuerpo / Descripciones / Subtítulos / Consejos | `/Game/Fonts/CARDO-Regular_Font` (`/Game/NewFonts/Cardo-Regular`) | `0` | Body `20`, Subtítulo `24 / 30 / 38` (según accesibilidad), Caption `16` |
| Citas de Lore en paneles derechos | `/Game/Fonts/CARDO-Italic_Font` (`/Game/NewFonts/Cardo-Italic`) | `10` | Quote `18` |

### 4.3 Catálogo Exacto de Texturas Ya Disponibles (`/Game/UI/Textures/`)

Usar estas rutas exactas (sin duplicar ni inventar nombres inexistentes):
- **Frames:** `T_UI_Frame_Brackets_Panel`, `T_UI_Frame_Brackets_Minimal`, `T_UI_Frame_OptionRow`, `T_UI_Frame_OptionRow_WithStar`, `T_UI_Frame_Panel_Gothic`, `T_UI_Frame_Sidebar_Active`, `T_UI_Btn_Hex_3Slice`, `T_UI_Slot_Square_Normal`, `T_UI_Slot_Square_Selected`, `T_UI_Slot_Square_FillMask`, `T_UI_Slot_Diamond_Normal`, `T_UI_Slot_Diamond_Selected`, `T_UI_Slot_Diamond_FillMask`.
- **Bars & Controls:** `T_UI_Bar_Health_Frame`, `T_UI_Bar_Health_FillMask`, `T_UI_Bar_Shield_Frame`, `T_UI_Bar_Shield_FillMask`, `T_UI_Bar_Boss_Frame`, `T_UI_Bar_Boss_FillMask`, `T_UI_Bar_Finial_Simple_L/R`, `T_UI_Bar_Finial_Boss_L/R`, `T_UI_Bar_PhaseMarker`, `T_UI_Pip_Diamond_Hollow`, `T_UI_Pip_Diamond_Filled`, `T_UI_Ring_Progress`, `T_UI_Slider_Track`, `T_UI_Slider_Thumb`.
- **Gradients:** `T_UI_Grad_HFade_LeftToRight`, `T_UI_Grad_HFade_RightToLeft`, `T_UI_Grad_HFade_Center`, `T_UI_Grad_VFade_BottomUp`, `T_UI_Grad_Vignette_Radial`.
- **Ornaments:** `T_UI_Orn_Chevron_L`, `T_UI_Orn_Chevron_R`, `T_UI_Orn_Chevron_Down`, `T_UI_Orn_RowStar`, `T_UI_Orn_Sep_Diamond`, `T_UI_Orn_Sep_Star`, `T_UI_Orn_SidebarLine_V`, `T_UI_Orn_TabActiveLine`, `T_UI_Orn_TabDivider`, `T_UI_Orn_Crest_Velmara`, `T_UI_Orn_Crest_Boss`, `T_UI_Marker_Diamond_Hollow`, `T_UI_Marker_Diamond_Solid`, `T_UI_Reticle_LockOn`, `T_UI_Reticle_Possession`, `T_UI_Spinner_Moon`.
- **Masks & Backgrounds:** `T_UI_Mask_BloodStroke_Long_A/B`, `T_UI_Mask_BloodStroke_Short_A/B`, `T_UI_Bg_MainMenu`, más `/Game/Images/T_UI_Logo_Velmara` (o el nuevo `T_UI_Logo_Velmara_V2` si está en `UI_Raw/`).
- **Icons:** Todos los `T_UI_Icon_*` bajo `/Game/UI/Textures/Icons/`.
- **CommonInput:** `/Game/Blueprints/HUD/InputPrompts/BP_VelmaraInputData` y `/Game/Blueprints/HUD/InputPrompts/DT_VelmaraInputActions`.

---

## 5. Arquitectura de Capas Common UI (`UVelmaraPrimaryGameLayout`)

Crear un widget raíz `UVelmaraPrimaryGameLayout` (`UCommonUserWidget`) administrado por `APlayerMainHUD` (y disponible también en el nivel `MainMenu`), con **4 capas ordenadas por ZOrder**:

1. **`HUDLayer` (`ZOrder = 0`, `ESlateVisibility::SelfHitTestInvisible`):**
   Contiene `UVelmaraGameplayHUDWidget` y `UVelmaraBossBarWidget` (solo en mapas de gameplay).
2. **`ToastLayer` (`ZOrder = 10`, `ESlateVisibility::SelfHitTestInvisible`):**
   Contiene `UVelmaraTutorialToastWidget` (consejos no intrusivos) y notificaciones breves ("Partida guardada", "Objeto obtenido", "Objetivo actualizado"). **Jamás captura el mouse ni pausa el juego.**
3. **`MenuStack` (`ZOrder = 20`, `UCommonActivatableWidgetStack`):**
   Pila principal de pantallas activables (`MainMenu`, `SaveSlots`, `PauseMenu`, `SettingsMenu`, `Inventory`, `Shop`, `BloodAltarMenu`, `DeathScreen`). Configura automáticamente `FUIInputConfig` (`ECommonInputMode::Menu` con mouse visible cuando hay un menú activo; restaura `ECommonInputMode::Game` al vaciarse la pila).
4. **`ModalStack` (`ZOrder = 30`, `UCommonActivatableWidgetStack`):**
   Pila superior exclusiva para `UVelmaraConfirmModalWidget`.

---

## 6. Controles Primitivos Reutilizables en C++ (`Source/TesisUE/Public/UI/Widgets/Common/`)

Construir primero estos 5 bloques base para que todas las pantallas compartan exactamente la misma respuesta de mouse, teclado y gamepad:

### 6.1 `UVelmaraButtonBase` (`UCommonButtonBase`)
- Construye programáticamente en `RebuildWidget()`:
  - `UImage* BgGradient` (`T_UI_Grad_HFade_LeftToRight`, tintado `Blood`, oculto en normal, `Alpha 0.75` en Hover/Focus, `0.95` en Pressed).
  - `UImage* FrameBorder` (`T_UI_Frame_OptionRow` o `T_UI_Btn_Hex_3Slice` o sin borde según `EVelmaraButtonVariant`: `MinimalList`, `FramedRow`, `SidebarTab`, `HexAction`).
  - `UImage* LeftMarker` (`T_UI_Marker_Diamond_Hollow` en normal si es `SidebarTab`, cambia a `T_UI_Marker_Diamond_Solid` tintado `BloodBright` en Hover/Focus/Selected).
  - `UCommonTextBlock* LabelText` (`Forum_Font`, `BoneMuted` en normal -> `Bone` en Hover/Focus, `Ash 40%` en Disabled).
- **Garantía crítica de Hover + Focus:** Sobrescribir `NativeOnHovered`, `NativeOnUnhovered`, `NativeOnAddedToFocusPath`, `NativeOnRemovedFromFocusPath`, `NativeOnPressed`, `NativeOnReleased` y `NativeOnSelectedChange` llamando a un único método `UpdateVisualState()`. Si `IsHovered() || HasAnyUserFocus() || HasKeyboardFocus()`, se activa el estado visual rojo sangre (y además en `NativeOnHovered` llamar a `SetKeyboardFocus()` cuando hay un menú activo para que mover el mouse sincronice la selección con el teclado/gamepad igual que en juegos AAA).

### 6.2 `UVelmaraSettingCarouselRow` (`UCommonButtonBase`)
Reemplazo definitivo de `UComboBoxString`:
- **Estructura visual (calca el mockup de Ajustes):**
  - Borde exterior: `T_UI_Frame_OptionRow_WithStar` (`Ash` en reposo -> `BloodBright` en foco) + relleno `Charcoal` (`Alpha 0.70`) + wash `BloodDried` en foco.
  - Mitad izquierda (`HBox` / `GridPanel` columna `0.52`): icono decorativo `<` (`T_UI_Orn_Chevron_L`, `12x12`) + `LabelText` en mayúsculas (`Forum_Font 20`).
  - Mitad derecha (columna `0.48`): botón/zona clicable izquierda `<` (`PrevArrow` con `T_UI_Orn_Chevron_L`), `ValueText` centrado (`Forum_Font 20`, `Bone`), botón/zona clicable derecha `>` (`NextArrow` con `T_UI_Orn_Chevron_R`).
- **Interacción:**
  - Clic en `<` llama a `StepOption(-1)`; clic en `>` (o clic en el centro del valor) llama a `StepOption(+1)`.
  - Cuando la fila tiene foco de teclado/gamepad, `NativeOnKeyDown` intercepta `EKeys::Left`, `EKeys::A`, `EKeys::Gamepad_DPad_Left`, `EKeys::Gamepad_LeftStick_Left` -> `StepOption(-1)` y sus equivalentes derechos -> `StepOption(+1)` retornando `FReply::Handled()`. Arriba/Abajo navega normalmente a la fila vecina.
  - Soporta `bWrapAround` (ej. `false` para Calidades Baja..Épica, `true` para Sí/No o Modo Ventana).
  - Dispara `OnSelectionChanged(int32 NewIndex)` y `OnRowHighlighted(const FVelmaraSettingMetadata& Meta)` para actualizar el panel derecho de descripción.

### 6.3 `UVelmaraSettingSliderRow` (`UCommonButtonBase`)
- Misma mitad izquierda y marco que `UVelmaraSettingCarouselRow`.
- Mitad derecha: `USlider` estilizado en C++ (`NormalBarImage = T_UI_Slider_Track` tintado `Ash`, barra de relleno superpuesta tintada `BloodBright`, `NormalThumbImage / HoveredThumbImage = T_UI_Slider_Thumb` diamante en `Bone` / `BloodBright`) + `ValueText` a la derecha (`56px` fijo, ej. `80%`, `2.2`, `1.25`).
- Con foco en Gamepad/Teclado, Izquierda/Derecha ajusta `Value ± StepSize` (ej. `0.05f` en volúmenes, `0.05f` en gamma, `0.05f` en sensibilidad).

### 6.4 `UVelmaraBracketPanel` (`UCommonUserWidget`)
- Contenedor reutilizable con relleno `Void` (`Alpha 0.80`) y borde `T_UI_Frame_Brackets_Panel` (`DrawAs = Border`, margen 9-slice `0.22f`, tintado `Bone`), `NamedSlot` o `UVerticalBox` interno con padding `32px`, y firma opcional abajo a la derecha (`VELMARA` + `T_UI_Orn_Crest_Velmara`).

### 6.5 `UVelmaraFooterBar` (`UCommonUserWidget`)
- Abajo a la izquierda: lema `"ALGUNAS COSAS"` + `T_UI_Orn_Sep_Star` + `"NUNCA MUEREN"` en `Forum_Font 14` (`BoneMuted`).
- Abajo a la derecha: fila dinámica de acciones (`[A] Seleccionar`, `[B] Atrás`, y acciones contextuales como `[Y] Aplicar cambios` o `[X] Borrar ranura`) usando `UCommonActionWidget` alimentado por `BP_VelmaraInputData` (cambia los glifos en tiempo real al alternar entre Teclado/Mouse y Gamepad suscribiéndose a `UCommonInputSubsystem::OnInputMethodChangedNative`).

---

## 7. Ampliaciones de Backend C++ Requeridas

### 7.1 `USettingsSaveGame` (`Source/TesisUE/Public/Features/SaveSystem/Core/SettingsSaveGame.h`)
Mantener intactos los campos existentes para no invalidar guardados previos y agregar con valores por defecto seguros:
- **Audio:** `float MasterVolume = 1.0f;`, `float MusicVolume = 0.8f;`, `float SFXVolume = 1.0f;`, `float UIVolume = 1.0f;`
- **Gráficos adicionales:** `bool bLumenEnabled = true;`, `int32 OverallQualityPreset = 2;` *(0=Baja, 1=Media, 2=Alta, 3=Épica, 4=Personalizada)*
- **Juego y Controles:** `float MouseSensitivity = 1.0f;`, `float GamepadSensitivity = 1.0f;`, `bool bInvertY = false;`, `bool bVibrationEnabled = true;`, `bool bCameraAssist = false;`
- **Accesibilidad e Idioma:** `bool bSubtitlesEnabled = true;`, `int32 SubtitleSizeIndex = 1;` *(0=Pequeño, 1=Normal, 2=Grande)*, `FString LanguageCode = TEXT("es");`

### 7.2 `UVelmaraGameInstance` (`VelmaraGameInstance.h/.cpp`)
1. **Audio real conectado a `/Game/Audio/Pato/`:**
   - Cargar en `Init()` las referencias suaves/duras a:
     - Control Buses: `/Game/Audio/Pato/Modulation/ControlBusses/CB_Master_Vol`, `CB_Music_Vol`, `CB_SFX_Vol` y Mix `/Game/Audio/Pato/Modulation/ControlBusMixes/CBM_MenuSettings` (`UAudioModulationStatics::ActivateBusMix` + `UAudioModulationStatics::UpdateMixByFilter` o `SetGlobalBusMixValue`).
     - SoundClasses de respaldo directo: `/Game/Audio/Pato/SoundClasses/SC_Master`, `SC_Music`, `SC_SFX`, `SC_UI` ajustando `Properties.Volume` para garantizar que tanto los MetaSounds modulados como cualquier `SoundWave` enrutado solo por `SoundClass` cambien su volumen al instante al mover el slider.
   - Exponer `SetMasterVolume(float)`, `SetMusicVolume(float)`, `SetSFXVolume(float)`, `SetUIVolume(float)` y `ApplyAudioSettings()`.
2. **Separación entre ajustes en vivo y ajustes gráficos diferidos:**
   - **En vivo (aplican y guardan al instante):** Volúmenes, Gamma (`ApplyGamma`), Sensibilidad/Invertir Y/Vibración (leídos por `ACustomPlayerController` en input de cámara y `ClientPlayForceFeedback`), Subtítulos e Idioma (`FInternationalization::Get().SetCurrentCulture(LanguageCode)`).
   - **Diferidos con botón `Aplicar cambios`:** `WindowMode`, `ScreenResolution`, `OverallQualityPreset`, las 8 calidades de `UGameUserSettings`, `bLumenEnabled` (CVars `r.DynamicGlobalIlluminationMethod` y `r.ReflectionMethod`: `1` = Lumen, `0/2` = ScreenSpace/None) y `FrameRateLimit`.
   - Agregar `ResetAllSettingsToDefaults()`.

### 7.3 `USaveGameSubsystem` (`SaveGameSubsystem.h/.cpp`)
Agregar dos métodos limpios:
- `UFUNCTION(BlueprintCallable) bool DeleteSaveSlot(int32 SlotIndex);` -> Borra el archivo del slot con `UGameplayStatics::DeleteGameInSlot`, limpia `ActiveSaveSlotIndex` si coincidía y emite `OnGameSaved`.
- `UFUNCTION(BlueprintPure) int32 GetMostRecentSaveSlot() const;` -> Revisa los slots `0..2` existentes con `GetSaveSlotInfo`, compara `Timestamp` y devuelve el índice más reciente (o `INDEX_NONE` si no hay ninguna partida guardada).

---

## 8. Especificación Detallada de las 10 Pantallas / Módulos UI

### Módulo 1 — Menú Principal (`UVelmaraMainMenuWidget` : `UCommonActivatableWidget`)
- **Referencia visual:** `M-V2-01` y captura 4 corregida.
- **Layout:**
  - Fondo 16:9 `T_UI_Bg_MainMenu` (`ScaleBox` `ScaleToFill`) + viñeta lateral izquierda `T_UI_Grad_HFade_LeftToRight` en `Void` para contraste perfecto.
  - Arriba-izquierda: Logo `Velmara` remasterizado + debajo separador `T_UI_Orn_Sep_Star`. Arriba-derecha: `T_UI_Orn_Crest_Velmara`. Izquierda exterior: línea vertical `T_UI_Orn_SidebarLine_V`.
  - Centro-izquierda: `UVelmaraBracketPanel` transparente (`T_UI_Frame_Brackets_Minimal`) conteniendo `UVerticalBox` de `UVelmaraButtonBase`:
    1. `CONTINUAR` *(se muestra solo si `SaveSubsystem->GetMostRecentSaveSlot() != INDEX_NONE`; carga ese slot directamente).*
    2. `NUEVA PARTIDA` -> Empuja `UVelmaraSaveSlotsWidget` en modo `ESaveSlotsMode::NewGame`.
    3. `CARGAR PARTIDA` -> Empuja `UVelmaraSaveSlotsWidget` en modo `ESaveSlotsMode::LoadGame`.
    4. `ZONA DE PRÁCTICA` -> Llama `ShowLoadingScreen()` y `UGameplayStatics::OpenLevel(this, TEXT("Felipe"))`.
    5. `AJUSTES` -> Empuja `UVelmaraSettingsMenuWidget` *(corrige el bug que abría LoadGameSlots)*.
    6. `SALIR` -> Empuja `UVelmaraConfirmModalWidget` ("¿SALIR DE VELMARA?") -> `UKismetSystemLibrary::QuitGame`.
  - Pie de pantalla: `UVelmaraFooterBar` (`[A] Seleccionar`, `[B] Salir`).
  - `NativeGetDesiredFocusTarget()` devuelve `Continuar` (si visible) o `Nueva Partida`.

### Módulo 2 — Selección de Ranuras: Nueva / Cargar Partida (`UVelmaraSaveSlotsWidget` : `UCommonActivatableWidget`)
- **Referencia visual:** `M-V2-02`.
- **Layout:**
  - Header izquierdo: `"NUEVA PARTIDA"` o `"CARGAR PARTIDA"` + subtítulo `"SELECCIONAR RANURA"`.
  - Columna izquierda (`65%`): 3 tarjetas `UVelmaraSaveSlotCardWidget` para los slots `0, 1, 2`, consultando `SaveSubsystem->DoesSaveGameExist(i)` y `GetSaveSlotInfo(i, LevelName, Timestamp, Label)` + `VelmaraGameInstance->GetSaveGameInfo(i)`:
    - **Ocupado:** Muestra `"RANURA I — <Nombre amigable del nivel>"`, fecha formateada (`dd/MM/yyyy HH:mm`), cantidad de atributos/combos desbloqueados.
    - **Vacío:** Muestra `"— RANURA VACÍA —"`.
  - Columna derecha (`35%`): `UVelmaraBracketPanel` con detalles de la ranura enfocada.
  - **Reglas de acción:**
    - En modo `NewGame`: si el slot está vacío, llama directo a `VelmaraGameInstance->CreateNewGame(SlotIndex, TEXT("MainLevel"))`; si está **ocupado**, abre primero `UVelmaraConfirmModalWidget` ("¿SOBRESCRIBIR RANURA?").
    - En modo `LoadGame`: los slots vacíos aparecen atenuados y no ejecutan carga al presionar `[A]`; un slot ocupado llama a `VelmaraGameInstance->LoadGame(SlotIndex)`.
    - Acción secundaria `[X / Supr]` sobre cualquier slot ocupado: abre `UVelmaraConfirmModalWidget` ("¿ELIMINAR PARTIDA GUARDADA?") -> `SaveSubsystem->DeleteSaveSlot(SlotIndex)` y refresca las 3 tarjetas.

### Módulo 3 — Menú de Pausa (`UVelmaraPauseMenuWidget` : `UCommonActivatableWidget`)
- **Referencia visual:** `M-V2-03` (**independiente y clásico, SIN barra superior de pestañas**).
- **Layout & Lógica:**
  - Fondo: `UBackgroundBlur` (`BlurStrength = 14.0f`) + imagen `Void` (`Alpha 0.70`).
  - Al activarse (`NativeOnActivated`): pausa el juego, activa `PauseSoundMix` / `CBM_Pause`. Al desactivarse (`NativeOnDeactivated`): si no queda otro menú en `MenuStack`, quita la pausa y restaura el mix de audio.
  - Columna izquierda: Título `P A U S A` + separador + botones `UVelmaraButtonBase` (`FramedRow`):
    1. `REANUDAR` -> `DeactivateWidget()`
    2. `INVENTARIO` -> Empuja `UVelmaraInventoryWidget`
    3. `AJUSTES` -> Empuja `UVelmaraSettingsMenuWidget`
    4. `GUARDAR PARTIDA` -> `VelmaraGameInstance->SaveGame(ActiveSaveSlotIndex)` y dispara notificación breve en `ToastLayer` (`"PARTIDA GUARDADA"`)
    5. `CARGAR PARTIDA` -> Empuja `UVelmaraSaveSlotsWidget(LoadGame)`
    6. `MENÚ PRINCIPAL` -> `UVelmaraConfirmModalWidget` ("¿VOLVER AL MENÚ PRINCIPAL? Se perderá el progreso no guardado.") -> `OpenLevel("MainMenu")`
    7. `SALIR AL ESCRITORIO` -> `UVelmaraConfirmModalWidget` -> `QuitGame`
  - Columna derecha: `UVelmaraBracketPanel` mostrando zona actual, moneda de Sangre (`UProgressionComponent::GetAvailableCurrency()`) y resumen de estado.

### Módulo 4 — Menú de Ajustes Completo (`UVelmaraSettingsMenuWidget` : `UCommonActivatableWidget`)
- **Referencia visual:** `M-V2-04` y tu captura de referencia (sin barra superior Hub).
- **Columna Izquierda (7 Categorías reales con `UVelmaraButtonBase` variante `SidebarTab` + atajo `Q/E` o `LB/RB`):**
  1. **`PANTALLA`** (diferido con `Aplicar cambios`, excepto Gamma que es en vivo):
     - `Modo de Ventana` (Carrusel: `Pantalla Completa`, `Ventana sin Bordes`, `Ventana`)
     - `Resolución` (Carrusel poblado dinámicamente con `UKismetSystemLibrary::GetSupportedFullscreenResolutions()`, fallback `1280×720`, `1600×900`, `1920×1080`, `2560×1440`, `3840×2160`)
     - `Calidad Global` (Carrusel: `Baja`, `Media`, `Alta`, `Épica`, `Personalizada` — cambiarla sincroniza las 8 filas de abajo; tocar una individual pasa Calidad Global a `Personalizada`)
     - `Distancia de Visión`, `Antialiasing`, `Posprocesado`, `Calidad de Sombras`, `Iluminación Global`, `Efectos Visuales`, `Calidad de Texturas`, `Sombreado` (8 Carruseles: `Baja`, `Media`, `Alta`, `Épica`)
     - `Iluminación Lumen` (Carrusel: `Desactivada`, `Activada`)
     - `FPS Máximos` (Carrusel: `30`, `60`, `90`, `120`, `Sin límite`)
     - `Brillo / Gamma` (Slider `1.4` a `3.0`, paso `0.05`, aplica en vivo)
  2. **`SONIDO`** (4 Sliders `0%` a `100%`, paso `5%`, aplican y guardan en vivo):
     - `Volumen General` (`CB_Master_Vol` + `SC_Master`)
     - `Música` (`CB_Music_Vol` + `SC_Music`)
     - `Efectos de Sonido` (`CB_SFX_Vol` + `SC_SFX`)
     - `Interfaz` (`SC_UI`)
  3. **`JUEGO`** (en vivo):
     - `Vibración del Mando` (Carrusel: `Desactivada`, `Activada`)
     - `Asistencia de Cámara` (Carrusel: `Desactivada`, `Activada`)
  4. **`CONTROLES`** (en vivo + tabla informativa limpia):
     - `Sensibilidad de Ratón` (Slider `0.2` a `3.0`)
     - `Sensibilidad de Mando` (Slider `0.2` a `3.0`)
     - `Invertir Eje Vertical` (Carrusel: `No`, `Sí`)
     - Debajo, grilla de lectura limpia con los bindings reales de `EnhancedInput` (Ataque, Habilidades 1–4, Cambiar Arma, Poseer, Esquivar, Fijar Objetivo, Interactuar) mostrando el icono dinámico de Teclado y de Gamepad lado a lado.
  5. **`ACCESIBILIDAD`** (en vivo):
     - `Subtítulos` (Carrusel: `Desactivados`, `Activados`)
     - `Tamaño de Subtítulos` (Carrusel: `Pequeño`, `Normal`, `Grande`)
  6. **`IDIOMA`** (en vivo):
     - `Idioma de Textos` (Carrusel: `Español`, `English`)
  7. **`CRÉDITOS`**:
     - Panel central limpio presentando el proyecto **Velmara (Tesis UE)**, integrantes/roles y herramientas.
- **Columna Derecha:** `UVelmaraBracketPanel` dinámico que muestra título, explicación y cita en cursiva de la fila enfocada, más aviso en rojo si hay cambios gráficos sin aplicar (`bHasPendingGraphicsChanges`).
- **Acciones inferiores:** `[A] Cambiar`, `[Y / F] Aplicar cambios` (habilitado/destacado cuando `bHasPendingGraphicsChanges == true`; si el usuario pulsa `Atrás` con cambios pendientes, preguntar con `UVelmaraConfirmModalWidget` "¿Aplicar o descartar cambios gráficos?"), `[X / R] Restablecer valores`, `[B / Esc] Atrás`.

### Módulo 5 — Cartel No Intrusivo de Consejos / Tutoriales (`UVelmaraTutorialToastWidget` : `UCommonUserWidget`)
- **Referencia visual:** `M-V2-05` (reemplaza por completo la captura 3 `WBP_PromptPopUp`).
- **Comportamiento obligatorio:**
  - **NUNCA llama a `SetGamePaused(true)`** ni cambia el `InputMode` a UI. El jugador sigue caminando, girando la cámara y combatiendo con normalidad.
  - Se aloja en `ToastLayer`, anclado a `Middle-Right` (`Anchors(1.0, 0.5)`, `Alignment(1.0, 0.5)`, margen derecho `64px`, ancho `460px`), con animación suave de entrada/salida (`16px` slide + fade `180ms`).
  - Soporta cola/array de páginas (`TArray<FVelmaraTutorialPage>`: `Title`, `RichBody`, `DurationSeconds = 7.5f`).
  - Header pequeño: diamante rojo + `"CONSEJO"` (y `"1 / N"` si hay más de una página).
  - Cuerpo: `UCommonRichTextBlock` conectado a `DT_RichTextStyleSet` (asegurar filas de estilo `Default` en Cardo `Bone`, `Emphasis` en `BloodBright`, `Spectral` en `Spectral #8F86A8`, `Key` en `Bone` bold) + parser C++ que convierte tokens de acción (ej. `{Interact}` o el texto legacy `"Press E"` / `[E]`) en el icono actual del dispositivo activo (`BP_VelmaraInputData`).
  - Barra inferior: línea fina de `2px` que se vacía suavemente durante `DurationSeconds`; al terminar pasa sola a la siguiente página o se oculta. Además, escuchar de forma no bloqueante una tecla secundaria (ej. `Tab` o `Gamepad_Special_Left`) para avanzar/cerrar antes si el jugador quiere.
  - **Puente de compatibilidad con `WBP_PromptPopUp`:** Reemplazar la lógica interna de `/Game/Blueprints/Tutorial/WBP_PromptPopUp` (o reparentarlo a una clase puente C++ `UVelmaraLegacyPromptBridgeWidget`) para que cuando cualquier trigger existente de los mapas cree/empuje `WBP_PromptPopUp` con su `Array<S_TutorialPromptData>`, envíe automáticamente esas páginas a `UVelmaraTutorialToastWidget` en `ToastLayer` y se autodestruya al instante sin pausar el juego.

### Módulo 6 — Modal de Confirmación Universal (`UVelmaraConfirmModalWidget` : `UCommonActivatableWidget`)
- **Referencia visual:** `M-V2-06`.
- Método estático helper: `UVelmaraConfirmModalWidget::ShowConfirmModal(UObject* WorldContext, FText Title, FText Message, FText ConfirmLabel, FText CancelLabel, TFunction<void()> OnConfirm, TFunction<void()> OnCancel = nullptr)`.
- Empuja al `ModalStack` sobre fondo `Void 75%` + `BackgroundBlur`, `UVelmaraBracketPanel` centrado (`640x300`), dos `UVelmaraButtonBase` (`HexAction`): **`CANCELAR` recibe el foco inicial siempre** por seguridad UX, y `B / Esc` equivale a Cancelar (`bIsBackHandler = true`).

### Módulo 7 — Altar de Sangre: Atributos y Mejoras de Armas (`UpgradeUIStyle.h` + `UVelmaraBloodAltarMenuWidget`)
- **Referencia visual:** `M-V2-07`.
- Actualizar `Source/TesisUE/Public/Features/UpgradeSystem/UI/UpgradeUIStyle.h`, `UpgradeMenuWidgetBase`, `AttributeTreeMenuWidget`, `AttributeTreeRowWidget`, `WeaponUpgradeMenuWidget`, `WeaponComboRowWidget` y `WeaponUpgradeTabWidget`:
  - Reemplazar todos los colores dorados/marrones y bordes redondeados (`CornerRadius > 0`) por los tokens de `FVelmaraUITheme` (`CornerRadius = 0`, marcos `T_UI_Frame_OptionRow` y `T_UI_Frame_Brackets_Panel`, tipografías `Forum_Font` y `Cardo`, pips de nivel con `T_UI_Pip_Diamond_Filled` en `BloodBright` y `T_UI_Pip_Diamond_Hollow` en `Ash`).
  - Envolver o adaptar `UUpgradeMenuWidgetBase` en un `UCommonActivatableWidget` (`UVelmaraBloodAltarMenuWidget`) empujado a `MenuStack` cuando se interactúa con `ABloodAltar` o se abre `UAttributeTreeComponent` / `UWeaponUpgradeComponent`, permitiendo alternar con `LB / RB` (`Q / E`) entre las dos pestañas reales: **`ATRIBUTOS`** y **`MEJORAS DE ARMAS`**, con navegación completa por Gamepad y contador de Sangre arriba a la derecha.

### Módulo 8 — Inventario (`UVelmaraInventoryWidget`) y Tienda (`UVelmaraShopMenuWidget`)
- **Referencia visual:** `M-V2-08`.
- **`UVelmaraInventoryWidget`:** Hereda de `UInventory` (o `UInventory` pasa a heredar de `UCommonActivatableWidget` manteniendo intacta la firma `RefreshInventoryUI` y las llamadas desde `UInventoryComponent::ShowInventory()` / `HideInventory()`).
  - Izquierda: slots de arma equipada (`UInventoryComponent::GetCurrentWeapon()`, `EquippedSlotIndex`).
  - Centro: grilla `5x4` de `UVelmaraItemSlotButton` usando `T_UI_Slot_Square_Normal` y overlay `T_UI_Slot_Square_Selected` en `BloodBright`.
  - Derecha: `UVelmaraBracketPanel` mostrando nombre, icono, descripción y stats del arma seleccionada.
  - Acciones: `[A] Equipar` (`EquipItemFromSlot`), `[X] Soltar` (`DropWeaponFromSlot`), `[B] Cerrar`.
- **`UVelmaraShopMenuWidget`:** Conectado a `/Game/Blueprints/DataTables/DT_ShopItemData` (`FShopItemData`) y al `UProgressionComponent` del jugador (`GetAvailableCurrency()`, `TryBuyUpgrade(Item.GrantedTag, Item.BaseCost)`, verificación de `Item.RequiredTag` y si ya posee `GrantedTag`). Vincular con `/Game/Blueprints/SpectralMode/BP_Shop` / `WBP_ShopMenu` para que abrir cualquier tienda del mapa abra esta versión C++.

### Módulo 9 — Pantalla de Muerte (`UVelmaraDeathScreenWidget`) y Pantalla de Carga (`UVelmaraLoadingScreenWidget`)
- **Referencia visual:** `M-V2-09A` y `M-V2-09B`.
- **Muerte (`UVelmaraDeathScreenWidget` : `UCommonActivatableWidget`):**
  - Fondo `Void` con fade-in de `1.2s` + `T_UI_Grad_Vignette_Radial` en `BloodDried`.
  - Centro: `T_UI_Mask_BloodStroke_Long_A` tintada `Blood` + texto `"H A S   C A Í D O"` en `Forum_Font 64` (`Bone`) con `T_UI_Orn_Sep_Star` arriba y abajo.
  - Botones centrados debajo: `REINTENTAR` (llama a `SaveGameSubsystem->LoadGame(ActiveSaveSlotIndex)` si existe slot activo, o recarga el nivel actual) y `MENÚ PRINCIPAL` (`OpenLevel("MainMenu")`). Conectar en el flujo de muerte actual donde se usaba `WBP_DeathScreen`.
- **Carga (`UVelmaraLoadingScreenWidget` : `UUserWidget`):**
  - Asignar a `UVelmaraGameInstance::LoadingScreenWidgetClass` y actualizar `WBP_LoadingScreen` / `WBP_VelmaraBootLoading`.
  - Fondo oscuro 16:9 + degradado inferior `T_UI_Grad_VFade_BottomUp`, abajo-izquierda etiqueta `"C O N S E J O"` (`BloodBright`) + `T_UI_Orn_Sep_Star` + consejo aleatorio de lore/mecánicas en `Cardo`, abajo-derecha `T_UI_Spinner_Moon` rotando suavemente en `NativeTick` (`30 deg/s`).

### Módulo 10 — HUD de Gameplay, Barra de Jefe y Prompts de Mundo
- **Referencia visual:** `M-V2-10`.
- **`UVelmaraGameplayHUDWidget` (reemplaza/moderniza `UPlayerMainWidget` en `APlayerMainHUD`):**
  - **Arriba-izquierda (Vitales conectados por delegados GAS a `UVelmaraAttributeSet`):**
    - Barra fina superior de Escudo (`Shield / MaxShield`, `T_UI_Bar_Shield_Frame` + `FillMask` tintado `Shield #B8C0C4`).
    - Barra principal de Vida (`Health / MaxHealth`, cuerpo estirable `T_UI_Bar_Health_Frame` + `FillMask` tintado `Blood` + barra intermedia diferida `BloodDried` con interpolación suave tipo `UDramaticBar` + puntas fijas `T_UI_Bar_Finial_Simple_L/R` sin deformación).
    - Debajo: barra/pips de Energía Espectral (`Energy / MaxEnergy`, tintado `Spectral #8F86A8` + 5 pips `T_UI_Pip_Diamond_Filled/Hollow`).
    - Aviso condicional debajo: icono `T_UI_Icon_Status_Sunlight` + `"EXPOSICIÓN SOLAR"` en `Sunlight #D9A441` cuando `USunlightExposureSubsystem::IsExposed(PlayerPawn)` sea `true`, e indicador sutil cuando `ACustomPlayerController::IsPossessing()` está activo.
  - **Abajo-izquierda:** 4 slots en diamante (`T_UI_Slot_Diamond_Normal`) para `Skill1..Skill4` + slot de arma activa (`UInventoryComponent::OnItemEquipped`).
  - **Abajo-derecha:** icono `T_UI_Icon_Stat_Blood` + contador animado de Sangre (`UProgressionComponent::OnCurrencyChanged`).
- **`UVelmaraBossBarWidget` (hereda de `UPaladinBossHealthBar` para compatibilidad directa con `ABossEnemy::HealthBarClass`):**
  - Sobrescribe `ReceivePercentagesFromBoss(float HealthHP, float ShieldHP)` en C++ (`BlueprintNativeEvent` o implementación directa): barra ancha inferior (`1050px`), puntas fijas `T_UI_Bar_Finial_Boss_L/R`, daño diferido `BloodDried`, escudo `Shield`, diamantes de fase `T_UI_Bar_PhaseMarker`, ornamento superior `T_UI_Orn_Crest_Boss` y nombre del jefe en `Forum_Font 26` con tracking amplio.
- **Prompts Flotantes de Mundo y Retículas (`UInputPromptWidget`, `UPromptWidgetComponent`, `WBP_PossessableEntity`, `WBP_TargetEntity`):**
  - Actualizar `UInputPromptWidget` y `UPromptWidgetComponent` para suscribirse a `UCommonInputSubsystem::OnInputMethodChangedNative` y cambiar en vivo entre `KeyboardIcon` y `GamepadIcon` sin requerir re-entrar al trigger, enmarcando la tecla dentro de un diamante fino (`T_UI_Slot_Diamond_Normal`) con anillo radial `T_UI_Ring_Progress` (usado también para carga de tótems `UTotemChargeComponent::GetChargePercent()`).
  - Estilizar `WBP_TargetEntity` con `T_UI_Reticle_LockOn` y `WBP_PossessableEntity` con `T_UI_Reticle_Possession`.

---

## 9. Plan de Ejecución en 6 Fases (Orden Obligatorio para el Agente)

1. **Fase 1 — Infraestructura, Build y Backend de Datos:**
   - Actualizar `TesisUE.Build.cs` (`CommonUI`, `CommonInput`, `AudioModulation`, etc.).
   - Implementar `FVelmaraUITheme`, ampliar `USettingsSaveGame`, conectar Audio Modulation/SoundClasses y staging gráfico en `UVelmaraGameInstance`, y agregar `DeleteSaveSlot` + `GetMostRecentSaveSlot` en `USaveGameSubsystem`.
   - **Compilar y verificar 0 errores (y 0 comentarios en `Source/`).**
2. **Fase 2 — Layout Raíz y Controles Primitivos Common UI:**
   - Implementar `UVelmaraPrimaryGameLayout` (4 capas: `HUD`, `Toast`, `MenuStack`, `ModalStack`), `UVelmaraButtonBase` (Hover == Focus), `UVelmaraSettingCarouselRow`, `UVelmaraSettingSliderRow`, `UVelmaraBracketPanel`, `UVelmaraFooterBar` y `UVelmaraConfirmModalWidget`.
   - **Compilar y verificar.**
3. **Fase 3 — Flujo Completo de Menús y Ajustes:**
   - Implementar `UVelmaraMainMenuWidget`, `UVelmaraSaveSlotsWidget`, `UVelmaraPauseMenuWidget` y `UVelmaraSettingsMenuWidget` (las 7 pestañas con carruseles, sliders y audio real).
   - Conectar `ACustomPlayerController::TogglePauseMenu` y `APlayerMainHUD` / nivel `MainMenu` al `MenuStack`.
   - **Compilar y verificar.**
4. **Fase 4 — Gameplay HUD, Jefe, Prompts, Muerte y Carga:**
   - Implementar `UVelmaraGameplayHUDWidget`, `UVelmaraBossBarWidget`, `UVelmaraTutorialToastWidget` (con el puente no intrusivo para `WBP_PromptPopUp`), `UVelmaraDeathScreenWidget`, `UVelmaraLoadingScreenWidget` y la actualización dinámica de `UInputPromptWidget`.
   - **Compilar y verificar.**
5. **Fase 5 — Sistemas de Progresión (Altar de Sangre, Inventario y Tienda):**
   - Renovar `UpgradeUIStyle.h` + `UVelmaraBloodAltarMenuWidget` (`AttributeTree` + `WeaponUpgrade`), e implementar `UVelmaraInventoryWidget` y `UVelmaraShopMenuWidget`.
   - **Compilar y verificar.**
6. **Fase 6 — Wiring de Assets/Blueprints Puente y Checklist de QA:**
   - Si hay nuevo logo en `UI_Raw/`, procesarlo con `Tools/ui_texture_pipeline.py` e importarlo.
   - Ejecutar un script Python mínimo en Unreal Editor **únicamente** para reparentar o apuntar las propiedades `WidgetClass` de `HUD_Velmara`, `BP_CustomPlayerController`, `BP_VelmaraGameInstance`, `ABossEnemy`, `WBP_TitleScreen`, `WBP_PromptPopUp` y `BP_Shop` hacia las nuevas clases C++ (nunca para parchear nodos internos de viejos `WBP_*`).
   - Verificar punto por punto la lista de control de la Sección 10.

---

## 10. Checklist de Aceptación Final

- [ ] **Cero `UComboBoxString`:** Todas las opciones discretas de Ajustes usan `UVelmaraSettingCarouselRow` (`< VALOR >`) con clic en flechas e Izquierda/Derecha en teclado/gamepad.
- [ ] **Audio funcional de punta a punta:** Mover `Volumen General`, `Música`, `Efectos` e `Interfaz` modifica el sonido en tiempo real y persiste al reiniciar el juego.
- [ ] **Hover y Gamepad idénticos en Menú Principal, Pausa, Slots y Ajustes:** Pasar el mouse por encima o mover el stick/cruceta muestra siempre la franja roja sangre degradada, el diamante marcador y el texto blanco hueso.
- [ ] **Botón `Ajustes` del Menú Principal corregido:** Abre `UVelmaraSettingsMenuWidget` (no `LoadGameSlots`), y **no** muestra la barra superior fantasma `Equipo/Mapa/Diario`.
- [ ] **Nueva Partida y Cargar Partida renovados:** 3 ranuras minimalistas conectadas a `USaveGameSubsystem`, con confirmación al sobrescribir y opción de borrar ranura.
- [ ] **Consejos (`WBP_PromptPopUp`) no intrusivos:** Ningún trigger de tutorial pausa el juego ni muestra el marco dorado con el botón gris `Done`; aparecen en `ToastLayer` a la derecha con brackets `L`, palabras resaltadas e íconos dinámicos.
- [ ] **Altar de Sangre, Inventario, Tienda, Muerte, Carga, Barra de Jefe y HUD:** Todos comparten la misma paleta, tipografías (`Forum` + `Cardo`) y marcos sin deformación de 9-slice.
- [ ] **Código limpio:** Compila sin errores en UE 5.6 y sin ningún comentario en `Source/`.
