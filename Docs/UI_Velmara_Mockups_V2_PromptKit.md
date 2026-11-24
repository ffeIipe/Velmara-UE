# VELMARA — Kit de Prompts V2 (Mockups Faltantes + Logo Remasterizado)

> **Propósito de este documento:** Generar únicamente las **vistas de referencia (Mockups 16:9)** de todas las pantallas que faltan o quedaron a mitad de camino, más la **remasterización limpia del Logo de Velmara** (y 1 lámina opcional de iconos fijos extra).
>
> **Regla de Arquitectura Híbrida (aprendida en producción):**
> 1. **PROHIBIDO pedir a la IA marcos 9-slice, cajas de filas, barras estirables, sliders o degradados.** Esas 77 texturas estructurales ya existen generadas matemáticamente en `/Game/UI/Textures/` (`Tools/ui_texture_pipeline.py`) y cualquier nueva pieza estirable la genera el agente C++/Python por código al píxel.
> 2. **NO adjuntes el Style Tile viejo como referencia de imagen.** En GPT Image 2.5 y NanoBanana funciona como *Image-to-Image* y clona la lámina de cuadraditos. Todos los prompts de abajo son **100% autocontenidos**: copiás un único bloque y lo pegás directo en Tripo sin adjuntar nada (salvo el caso opcional indicado en el Logo).
> 3. **ADN Visual Canónico:** Todos los mockups replican exactamente la estética lograda en tu captura de referencia de Ajustes: tipografía romana alta y fina (`Forum` / `Cinzel Light`) con espaciado amplio para títulos y botones, `Cardo` serif clásica para párrafos y citas en cursiva, marcos minimalistas de 4 esquinas en `L` blanco hueso (`#E8E0D0`), filas rectangulares finas con chevrons `< >` y estrella de 4 puntas inferior, hover/foco en rojo sangre (`#8E0F14`–`#C21A1F`), lema `ALGUNAS COSAS | NUNCA MUEREN` abajo a la izquierda e íconos circulares `(A) / (B)` abajo a la derecha.

---

## 0. Qué modelo y configuración usar en Tripo

| Pieza | Modelo recomendado en Tripo | Formato | ¿Adjuntar imagen? |
|---|---|---|---|
| **LOGO-01** (Remaster del logo `Velmara` con espada) | **GPT Image 2.5** | `16:9` | **Opción A:** Adjuntar SOLO `T_UI_Logo_Velmara.png` para calcar su composición exacta en limpio.<br>**Opción B:** Sin imagen adjunta (prompt puro). |
| **M-V2-01 a M-V2-10** (Mockups de pantallas completas) | **NanoBanana Pro** *(o GPT Image 2.5)* | `16:9` | **NO adjuntar imagen** (evita que clone las 3 columnas de Ajustes en pantallas que tienen otro layout). |
| **ASSET-EXTRA-01** (Solo si querés iconos fijos extra para Audio/Slots/Ajustes) | **GPT Image 2.5** | `1:1` | **NUNCA adjuntar imagen.** Blanco puro `#FFFFFF` sobre negro `#000000`. |

---

## 1. Remasterización del Logo Original (`LOGO-01`)

Conserva la identidad exacta del logo actual de tu proyecto (la palabra **Velmara** en serif gótica roja posada sobre una espada larga gótica horizontal), pero limpia los bordes pixelados/dorados sucios para que armonice con la nueva UI minimalista blanco hueso y sangre.

### `LOGO-01A` — Remaster fiel (Adjuntando `T_UI_Logo_Velmara` en GPT Image 2.5 · Formato 16:9)
> Subí como imagen de entrada únicamente el archivo del logo viejo (`Content/Images/T_UI_Logo_Velmara`) y pegá este bloque:

```text
Clean AAA high-resolution 2D vector-like remaster of the attached "Velmara" game logo on a solid pure black #000000 background. Keep the exact composition and identity of the original logo: the word "Velmara" in sharp, aristocratic gothic-serif calligraphy positioned above and slightly overlapping an ornate horizontal gothic greatsword laying beneath the letters (pommel and intricate crossguard on the right side under "ara", long straight blade pointing left under "Velm"). Refine and clean up every jagged pixel and noisy texture: render the sword in crisp bone-white and pale cold silver linework (#E8E0D0 and #A39C8E) with razor-sharp filigree, thorns and gothic guard details instead of yellow cartoon gold; render the "Velmara" lettering in deep crimson blood red (#8E0F14 to #C21A1F) with a subtle razor-thin bone-white highlight edge for legibility against dark backgrounds. Flat orthographic 2D presentation, solid #000000 background for clean alpha extraction, generous empty black margin around the logo, no background scenery, no extra text, no glow halo, no 3D bevel.
```

### `LOGO-01B` — Alternativa desde cero sin adjuntar imagen (GPT Image 2.5 · Formato 16:9)
```text
AAA dark-fantasy game title logo "Velmara" centered on a solid pure black #000000 background. The word "Velmara" is written in elegant, sharp gothic-serif lettering in deep blood red (#9E141A) with subtle razor-sharp thorn serifs. Directly beneath and interlocking horizontally with the base of the letters lies a slender, aristocratic gothic greatsword: intricate thorned crossguard and pommel on the right under the letters "ara", needle-sharp thin blade extending left under "Velm", drawn in crisp bone-white (#E8E0D0) and muted silver-ash (#A39C8E) fine linework — zero yellow gold. Clean 2D high-resolution vector-like illustration, crisp silhouette, no noisy grunge, no drop shadow, no outer glow, large empty pure black margins on all sides so it can be cleanly extracted to transparent PNG.
```

---

## 2. Prompts de Mockups de Pantallas Completas (Copiar y Pegar directo)

> **Instrucción:** En Tripo elegí **NanoBanana Pro** (o **GPT Image 2.5**), relación **16:9**, **sin imagen adjunta**, y pegá el bloque completo de la pantalla que quieras generar.

---

### `M-V2-01` — Menú Principal (Con Hover funcional, Brackets `L` y Logo con Espada)
> Guarda el resultado como `Mockup_V2_01_MainMenu.png`.

```text
16:9 in-game UI mockup at 1920x1080 for "Velmara", an AAA gothic vampire action-RPG with the restrained minimalist elegance of Phantom Blade Zero. Background key art: a dark atmospheric gothic cathedral perched on jagged cliffs under a pale eclipsed moon and drifting crimson mist at the bottom; the left third of the screen fades smoothly into deep void black (#0A0809) for effortless UI readability.

Top-left: the "Velmara" logo (crimson red gothic-serif lettering resting over a thin horizontal bone-white gothic sword), and beneath it a razor-thin horizontal bone-white hairline divider with a tiny 4-point needle star in its center. Far left edge: a delicate vertical hairline spine with a tiny white 4-point star marker at mid-height. Top-right corner: a small minimal sigil made of a crimson crescent moon above a thin vertical silver needle-dagger.

Left-side main menu navigation framed only by four crisp, thin L-shaped corner brackets in bone white (#E8E0D0), floating over translucent darkness with no solid box border. Inside the brackets, a vertical list of 6 options in tall, thin, widely spaced Roman capitals (Forum / light Cinzel style):
- CONTINUAR (muted bone #A39C8E)
- NUEVA PARTIDA (FOCUSED / HOVERED STATE: highlighted with a sleek horizontal dark blood-red gradient wash #8E0F14 fading to transparent on the right, thin crimson top/bottom hairlines, a small crimson diamond marker on the left, and bright bone-white text #E8E0D0)
- CARGAR PARTIDA (muted bone)
- ZONA DE PRÁCTICA (muted bone)
- AJUSTES (muted bone)
- SALIR (muted bone)

Bottom-left corner: small spaced Roman capitals "ALGUNAS COSAS  |  NUNCA MUEREN" divided by a thin vertical 4-point needle cross. Bottom-right corner: clean monochrome circular gamepad/input prompts "(A) SELECCIONAR    (B) SALIR" in bone white. Crisp 2D UI overlay aligned to an 8px grid, 64px safe margins, no rounded corners, no gold, no clutter.
```

---

### `M-V2-02` — Selección de Ranuras: Nueva Partida / Cargar Partida (3 Slots)
> Reemplaza los cuadros y fuentes viejos que aparecían al tocar *Nueva Partida* o *Cargar Partida*. Guarda como `Mockup_V2_02_SaveSlots.png`.

```text
16:9 in-game UI mockup at 1920x1080 for "Velmara", AAA gothic vampire minimalist UI. Dark atmospheric cathedral background dimmed with an 80% void-black vignette (#0A0809).

Top-left header: title "SELECCIONAR PARTIDA" (or subtitle "CARGAR PARTIDA" in small crimson capitals above it) in tall, thin, widely spaced bone-white Roman capitals (Forum style), underlined by a razor-thin horizontal line with a sharp 4-point star ornament on the left. Top-right corner: small crimson crescent moon over a vertical silver needle sigil.

Left two-thirds: three wide, sleek horizontal save-slot cards stacked vertically with generous spacing:
1) RANURA I — OCCUPIED & FOCUSED: thin crimson hairline rectangular border (#C21A1F) with subtle dark blood-red translucent fill (#4A0A0D at 50%), sharp chevron markers "<" and ">" at the left and right edges, and a tiny white 4-point star on the bottom edge. Inside: left diamond emblem, slot header "RANURA I  —  CATEDRAL DEL ECLIPSE" in bright bone-white Roman capitals, secondary row in classic Cardo serif muted bone showing "Último guardado: 23 Nov · Nivel de Sangre 14", and on the right side blood currency icon with "2.450".
2) RANURA II — OCCUPIED (UNFOCUSED): thin ash-grey hairline border (#3A3638) over translucent void black (#0A0809 at 75%), muted bone text "RANURA II  —  CRIPTA INTERIOR", "Último guardado: 21 Nov · Nivel de Sangre 6", "840".
3) RANURA III — EMPTY SLOT: thin ash-grey hairline border (#3A3638), centered muted text "— RANURA VACÍA —" in spaced Roman capitals with a subtle hollow diamond icon.

Right third: a summary panel framed ONLY by four thin bone-white L-shaped corner brackets. Inside: selected zone title "CATEDRAL DEL ECLIPSE", thin divider, lore/progress summary in classic Cardo serif ("Continúa la cacería desde el último Altar de Sangre consagrado. Armas desbloqueadas: 3 · Atributos mejorados: 8."), italic quote at the bottom, and tiny "VELMARA" needle mark at bottom-right.

Bottom-left: "ALGUNAS COSAS | NUNCA MUEREN" with vertical needle star. Bottom-right: circular monochrome input prompts "(A) INICIAR    (X) BORRAR RANURA    (B) ATRÁS". Flat crisp 2D UI, zero rounded corners, zero gold.
```

---

### `M-V2-03` — Menú de Pausa (Independiente, SIN la barra superior de pestañas fantasma)
> Guarda como `Mockup_V2_03_PauseMenu.png`.

```text
16:9 in-game UI mockup at 1920x1080 for "Velmara", AAA gothic dark-fantasy pause screen. Behind the UI: a paused third-person gothic combat scene heavily blurred (gaussian background blur) and darkened by a 70% void-black overlay (#0A0809). IMPORTANT: NO top horizontal navigation bar (do not show Equipo/Mapa/Diario tabs at the top).

Left-center vertical layout: header "P A U S A" in tall, thin, widely spaced bone-white Roman capitals (Forum style), with a razor-thin horizontal hairline beneath it featuring a 4-point needle star on the left and a vertical thin spine running down the left margin. Top-right corner of screen: small red crescent moon over a vertical silver needle emblem.

Below the header, a clean vertical list of 7 menu rows inside thin rectangular hairline frames (#3A3638) with subtle "<" and ">" chevrons at their ends:
- REANUDAR (FOCUSED: crimson hairline border #C21A1F, dark blood-red horizontal gradient wash #8E0F14, small red diamond marker on the left, bright bone-white text #E8E0D0)
- INVENTARIO
- AJUSTES
- GUARDAR PARTIDA
- CARGAR PARTIDA
- MENÚ PRINCIPAL
- SALIR AL ESCRITORIO

Right side: a quiet, minimal status box framed ONLY by four thin bone-white L-shaped corner brackets, displaying current zone "CATEDRAL DEL ECLIPSE", current objective in classic Cardo serif ("Busca el mecanismo oculto tras el vitral consagrado."), current Blood currency "1.250" with a tiny blood-drop icon, and an atmospheric italic line at the bottom.

Bottom-left: "ALGUNAS COSAS | NUNCA MUEREN" split by a vertical needle star. Bottom-right: circular monochrome prompts "(A) SELECCIONAR    (B) REANUDAR". Ultra-clean, sharp geometry, no rounded corners, no gold frames.
```

---

### `M-V2-04` — Menú de Ajustes Definitivo (Sin barra Hub superior · Carruseles `< VALOR >` + Sliders de Diamante)
> Refina tu captura 2 eliminando las pestañas inexistentes de arriba y mostrando juntos los **Carruseles inline** (sin desplegables) y los **Sliders de audio/gamma** + el botón `Aplicar cambios`. Guarda como `Mockup_V2_04_Settings.png`.

```text
16:9 in-game UI mockup at 1920x1080 for "Velmara", AAA gothic vampire Settings screen. Dark blurred gothic castle background under a 75% void-black overlay (#0A0809). IMPORTANT: NO top horizontal menu bar across the top of the screen — the screen starts directly with the "A J U S T E S" header on the left and the small crimson crescent-and-needle sigil in the top-right corner.

Three-column layout aligned to a strict 8px grid:
1) LEFT COLUMN (Categories): Title "A J U S T E S" in tall widely spaced Roman capitals (Forum), thin horizontal divider with a 4-point star on the left, and a vertical hairline down the far-left edge. Vertical category list with small diamond icons:
   - ◇ PANTALLA (ACTIVE: dark blood-red horizontal wash #8E0F14, crimson hairline box, red diamond icon, bright bone-white text)
   - ◇ SONIDO
   - ◇ JUEGO
   - ◇ CONTROLES
   - ◇ ACCESIBILIDAD
   - ◇ IDIOMA
   - ◇ CRÉDITOS

2) CENTER COLUMN ("PANTALLA" options — NO dropdown menus anywhere, strictly inline carousels and diamond sliders): 8 sleek horizontal rectangular rows (#1C1A1C at 70% alpha, hairline border #3A3638) stacked vertically with a tiny white 4-point star pip on the center seam between rows:
   - MODO DE VENTANA          <   PANTALLA COMPLETA   >
   - RESOLUCIÓN               <     1920 × 1080       >
   - CALIDAD GLOBAL           <         ALTA          >   (FOCUSED ROW: crimson hairline border #C21A1F, dark blood-red wash, red "<" and ">" carousel arrows around "ALTA", white star marker on left margin)
   - CALIDAD DE TEXTURAS      <         ÉPICA         >
   - CALIDAD DE SOMBRAS       <         ALTA          >
   - ILUMINACIÓN LUMEN        <       ACTIVADA        >
   - FPS MÁXIMOS              <          120          >
   - GAMMA                    |──────────◇────────| 2.2   (razor-thin horizontal slider track with a small hollow/solid bone-white diamond thumb and numeric value on the right)

3) RIGHT COLUMN (Description Panel): Framed ONLY by four thin bone-white L-shaped corner brackets. Title "CALIDAD GLOBAL" in spaced Roman capitals, body explanation in classic Cardo serif ("Ajusta de forma conjunta el nivel de detalle de texturas, sombras, posprocesado y efectos visuales. Los cambios gráficos requieren confirmación."), subtle warning note in muted crimson "Cambios pendientes de aplicar", italic lore quote at bottom, and tiny "VELMARA" needle mark at bottom-right.

Bottom bar: left "ALGUNAS COSAS | NUNCA MUEREN" with vertical needle cross; right monochrome circular prompts "(A) CAMBIAR    (Y) APLICAR CAMBIOS    (X) RESTABLECER    (B) ATRÁS". Crisp flat 2D UI, zero dropdown cascades, zero gold.
```

---

### `M-V2-05` — Cartel No Intrusivo de Consejos / Tutoriales en Gameplay (Reemplaza la captura 3)
> **Sin pausar el juego**, anclado a un costado durante el gameplay, con soporte para teclas dinámicas, palabras resaltadas y paginado. Guarda como `Mockup_V2_05_TutorialToast.png`.

```text
16:9 in-game screenshot mockup at 1920x1080 of "Velmara" during live third-person exploration in a dark gothic stone corridor next to a hidden iron lever on a pillar. The game is NOT paused: no full-screen blur, center of screen completely unobstructed so the player keeps moving and fighting freely.

Anchored cleanly on the middle-right side of the screen (64px from the right edge, about 460px wide): a sleek, non-intrusive gothic tutorial toast panel. Background is translucent void black (#0A0809 at 82% alpha) with a soft fade on its inner edge, framed ONLY by four razor-thin bone-white L-shaped corner brackets (#E8E0D0) — zero gold borders, zero rounded corners, zero grey buttons.

Inside the toast panel:
- Top micro-header: small crimson diamond icon + "CONSEJO  ·  1 / 2" in tiny crimson Roman capitals (#C21A1F) on the left, and tiny muted page dots on the right.
- Title: "INTERACCIÓN Y SENTIDO VAMPÍRICO" in tall, thin spaced bone-white Roman capitals (Forum style), followed by a hairline separator with a tiny center diamond.
- Body text in crisp classic Cardo serif (#E8E0D0), featuring an inline minimal monochrome keycap icon "[E]" (sharp dark square with thin bone outline) and rich-text colored keywords: "Presiona [E] para accionar PALANCAS (in pale spectral violet #8F86A8). Permanecen ocultas en la penumbra si el SENTIDO VAMPÍRICO (in bright blood red #C21A1F) no está activo."
- Bottom of the toast: a razor-thin horizontal auto-dismiss timer hairline partially depleted in muted crimson, and on the bottom-right a tiny subtle inline hint "[TAB] Siguiente" in muted bone (#A39C8E) — NO bulky clickable button.

Top-left of screen shows the quiet minimal player health/shield/energy bars for context. Crisp AAA readability, Phantom Blade Zero restraint.
```

---

### `M-V2-06` — Modal de Confirmación Universal (Sobrescribir / Salir / Restablecer)
> Guarda como `Mockup_V2_06_ConfirmModal.png`.

```text
16:9 in-game UI mockup at 1920x1080 for "Velmara", showing a modal confirmation popup over a blurred, darkened menu screen (75% void-black overlay #0A0809).

Centered modal dialog (approx 640x300 px) with a translucent void-black interior (#0A0809 at 90%) framed ONLY by four crisp, razor-thin bone-white L-shaped corner brackets (#E8E0D0) and a tiny 4-point needle star centered on the top invisible edge.

Inside the modal, centered composition:
- Header: "¿SOBRESCRIBIR RANURA I?" in tall, widely spaced bone-white Roman capitals (Forum style).
- Thin horizontal hairline separator tapering at both ends with a small crimson diamond in the middle.
- Body message in classic Cardo serif (#A39C8E): "Ya existe una partida guardada en Catedral del Eclipse. Si continúas, todo el progreso anterior se perderá de forma permanente."
- Bottom row with two sleek elongated hexagonal sharp-pointed buttons side by side:
  1) "CANCELAR" (FOCUSED BY DEFAULT: thin crimson hairline outline #C21A1F, soft dark blood-red horizontal gradient fill #8E0F14, small diamond marker, bright bone-white text).
  2) "CONFIRMAR" (UNFOCUSED: thin ash hairline outline #3A3638, muted bone text #A39C8E).

Bottom-right of screen: monochrome circular prompts "(A) ACEPTAR    (B) CANCELAR". Sharp geometry, zero rounded corners, zero gold.
```

---

### `M-V2-07` — Altar de Sangre: Árbol de Atributos y Mejoras de Armas
> Reemplaza el estilo dorado redondeado actual de `UpgradeUIStyle.h` por la estética minimalista gótica. Guarda como `Mockup_V2_07_BloodAltar_Upgrades.png`.

```text
16:9 in-game UI mockup at 1920x1080 for "Velmara", showing the Blood Altar progression menu over a darkened, blurred gothic altar chamber with candles and crimson mist.

Top header bar (ONLY the two real Altar tabs): centered "ATRIBUTOS    ✦    MEJORAS DE ARMAS" in tall spaced Roman capitals (Forum); "ATRIBUTOS" is active in bright crimson (#C21A1F) with a thin red underline and tiny 4-point star. Top-right: Blood currency counter showing a sharp monochrome blood-drop icon + "3.420 SANGRE" in bone-white Roman capitals, next to the small crimson crescent-and-needle sigil.

Left two-thirds: a clean vertical list of 6 attribute upgrade rows (Salud Máxima, Escudo Espectral, Energía Máxima, Daño Base, Armadura, Velocidad de Ataque). Each row is a sleek thin hairline rectangle (#3A3638) over translucent void black:
- Left: sharp monochrome white stat icon + attribute name in Roman capitals + current value "120  →  135" (the upgraded value in crimson #C21A1F).
- Center-right: a row of 5 small diamond level pips (◆ ◆ ◆ ◇ ◇ — purchased pips filled blood red, remaining pips hollow ash grey).
- Far right: upgrade cost with tiny blood drop "450" and a sharp chevron ">".
- The third row ("ENERGÍA MÁXIMA") is FOCUSED with a crimson hairline frame, dark blood-red wash, and white 4-point star marker on the left spine.
(Above the list or shown subtly as secondary sub-pill note: clean sharp geometry compatible with switching to Weapon tabs: Espada, Dobles Hojas, Lanza, Guadaña, Pistola).

Right third: details panel framed ONLY by four thin bone-white L-shaped corner brackets: attribute title "ENERGÍA MÁXIMA", thin divider, lore and mechanical description in Cardo serif, requirement/cost breakdown ("Costo del nivel 4: 450 Sangre · Disponible: 3.420"), and a sharp hexagonal action prompt at the bottom of the panel.

Bottom-left: "ALGUNAS COSAS | NUNCA MUEREN". Bottom-right: "(LB/RB) CAMBIAR PESTAÑA    (A) MEJORAR    (B) SALIR DEL ALTAR". Zero gold, zero rounded boxes.
```

---

### `M-V2-08` — Inventario y Tienda (`WBP_Inventory` / `WBP_ShopMenu`)
> Guarda como `Mockup_V2_08_Inventory_Shop.png`.

```text
16:9 in-game UI mockup at 1920x1080 for "Velmara", AAA gothic vampire Inventory & Equipment screen over a blurred dark background (80% void black #0A0809).

Top-left header: "I N V E N T A R I O" in tall widely spaced Roman capitals (Forum) with a thin horizontal divider and 4-point needle star. Top-right: Blood currency counter ("1.850" with monochrome blood-drop icon) and the small crimson crescent-and-needle sigil.

Left column (Equipped Loadout): vertical arrangement of 3 large square weapon/relic slots framed by razor-thin bone-white hairlines with tiny 45-degree chamfered corners and corner diamond dots, displaying crisp white monochrome weapon silhouettes (Longsword equipped as Primary, Flintlock Pistol as Secondary, Crimson Vial).

Center column (Item Grid): a 5x4 grid of sharp square slots (#1C1A1C translucent fill, #3A3638 hairline border with tiny corner notches). Several slots hold clean flat white monochrome gothic icons (dual blades, scythe, spear, blood vials, ancient keys, relics). One slot ("GUADAÑA DEL ECLIPSE") is SELECTED: highlighted with a crisp double crimson hairline border, four tiny inner red L-brackets, and a subtle dark blood fill (#4A0A0D) — zero outer neon glow.

Right column (Item Details Panel): framed ONLY by four thin bone-white L-shaped corner brackets. Inside: weapon title "GUADAÑA DEL ECLIPSE" in Roman capitals, category "ARMA DE ASTA  ·  EQUIPABLE" in small crimson capitals, thin star divider, lore description in Cardo serif, clean stat rows ("Daño Base: 68 (+12)", "Velocidad de Ataque: 0.9x") with sharp crimson upward indicator, and italic quote at bottom.

Bottom-left: "ALGUNAS COSAS | NUNCA MUEREN". Bottom-right: "(A) EQUIPAR    (X) SOLTAR    (B) CERRAR".
```

---

### `M-V2-09` — Pantalla de Muerte y Pantalla de Carga (2 Mockups rápidos)

#### `M-V2-09A` — Pantalla de Muerte (`Mockup_V2_09A_DeathScreen.png`)
```text
16:9 in-game UI mockup at 1920x1080 for "Velmara" Death Screen. Gameplay behind has faded nearly to pure void black (#0A0809) with a heavy, moody dried-blood vignette around the screen edges (#4A0A0D). Center of screen: a wide, organic horizontal dry-brush stroke of deep dark crimson blood (#8E0F14) with rough bristle edges, and centered over it the words "H A S   C A Í D O" in tall, razor-thin, widely spaced bone-white Roman capitals (Forum style), framed above and below by two ultra-faint tapering hairlines with a tiny central 4-point needle star. Lower center, quiet and minimal: two stacked sharp options in Roman capitals — "REINTENTAR" (focused with a soft crimson horizontal fade strip and tiny red diamond marker on the left) and "MENÚ PRINCIPAL" (muted bone). Solemn, cinematic, zero clutter.
```

#### `M-V2-09B` — Pantalla de Carga (`Mockup_V2_09B_LoadingScreen.png`)
```text
16:9 in-game UI mockup at 1920x1080 for "Velmara" Loading Screen. Atmospheric dark painterly gothic key art of moonlit cathedral spires shrouded in mist, smoothly darkening into pure void black (#0A0809) across the bottom third. Bottom-left (64px margin): small crimson label "C O N S E J O" in widely spaced Roman capitals, a thin tapering bone-white hairline with a 4-point needle star, and two lines of classic Cardo serif text in bone white ("La luz del sol debilita tu forma inmortal. Permanece bajo las sombras de las bóvedas durante el combate."). Bottom-right (64px margin): a delicate rotating geometric loading sigil in thin bone-white hairlines (concentric fine rings cradling a tiny crescent moon and 4 needle points) next to tiny spaced text "CARGANDO". Nothing else on screen.
```

---

### `M-V2-10` — HUD en Partida + Barra de Jefe + Prompts de Mundo
> Guarda como `Mockup_V2_10_GameplayHUD_Boss.png`.

```text
16:9 in-game UI mockup at 1920x1080 for "Velmara" during a third-person gothic boss encounter in a ruined cathedral with shafts of pale sunlight. All HUD elements float cleanly without opaque background boxes:

1) TOP-LEFT (Player Vitals):
   - Thin hairline silver Shield bar (#B8C0C4) at 40% above the main bar.
   - Long razor-thin Health bar (approx 420x10 px) with needle-sharp blade-like tapered ends: deep blood-red fill (#8E0F14) at 65%, plus a darker dried-blood delayed damage trail (#4A0A0D) receding from 80% to 65%.
   - Directly below: a thin Spectral Energy bar in pale ghostly violet (#8F86A8) accompanied by 5 tiny diamond pips.
   - Subtle status warning below vitals when standing near light: tiny sharp sun icon + "EXPOSICIÓN SOLAR" in muted amber (#D9A441).

2) BOTTOM-LEFT (Combat Loadout): 4 sharp rotated-diamond skill slots with thin bone-white outlines, crisp white monochrome ability glyphs inside, and tiny monochrome key hints below; next to them, active weapon slot framed by tiny corner brackets showing a white Longsword silhouette.

3) BOTTOM-RIGHT: minimal Blood counter — monochrome blood-drop icon + "1.250" in spaced Roman capitals.

4) CENTER / WORLD PROMPTS: on a distant pillar totem, a minimal floating world prompt made of a rotated thin bone-white diamond holding key "E" with a half-filled thin circular hold ring around it and small text "POSEER". On the boss chest, a razor-thin lock-on reticle made of four inward-pointing needle spikes around a tiny red diamond.

5) BOTTOM-CENTER (Boss Health Bar): wide (1050px) razor-thin blade-ended boss health bar with ornate needle finials at both ends, silver shield hairline on top, blood-red HP fill with delayed damage trail, and two tiny white diamond phase markers at 33% and 66%. Centered above the bar: a minimal gothic thorned-halo crest ornament and "EL PALADÍN CONSAGRADO" in widely spaced bone-white Roman capitals.
```

---

## 3. (Opcional) Lámina única de Iconos Fijos Extra (`ASSET-EXTRA-01`)

Ya tenés **18 iconos importados** en `/Game/UI/Textures/Icons/`. Si el agente programador necesita iconos adicionales para **Sonido / Mute, Ranura de Guardado, Controles, Accesibilidad, Idioma y Consejo**, generá **solo esta lámina** en **GPT Image 2.5**, formato **1:1**, **SIN imagen adjunta**:

```text
Flat orthographic 2D game UI icon sheet: 6 minimalist gothic monochrome white (#FFFFFF) glyphs arranged in a clean 3x2 grid with huge empty space between them on a solid pure black (#000000) background. Strict 1px–2px thin hairline vector-like strokes and sharp clean silhouettes, zero boxes or frames around the icons, zero 3D, zero bevel, zero grey shading, zero noise, zero polygonal origami facets, zero text or letters:
(1) Audio / Sound: a slender gothic church bell emitting two thin curved sound arcs;
(2) Save Slot: an ornate vertical wax-seal ribbon bookmark with a tiny 4-point star;
(3) Controls: a clean minimal symmetrical gamepad silhouette in thin white outline;
(4) Display / Graphics: a sharp gothic eye silhouette enclosing a tiny solid diamond pupil;
(5) Language: a minimal armillary sphere / meridian globe made of thin intersecting rings;
(6) Tutorial / Lore Tip: an upright slender feather quill with a tiny diamond nib tip.
Pure #FFFFFF on pure #000000 only, generous margins around every cell.
```
