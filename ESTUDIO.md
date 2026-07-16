# Pico Launcher — Mapa de estudio

> Generado el 15-jul-2026 a partir de un análisis del código fuente (commit `d31a15c`, rama `develop`).
> Objetivo: entender cómo está construido el launcher y dónde encajan las mejoras del proyecto "Pico Enhanced".

## 1. Cómo compilar

**Requisito previo obligatorio** (el repo no compila sin esto — `libs/libtwl` está vacío):

```bash
git submodule update --init
```

**Opción A — Docker (idéntico a la CI, recomendado para el primer build):**

```bash
docker run --rm -v "$PWD":/work -w /work skylyrac/blocksds:slim-v1.16.0 make
```

La CI (`.github/workflows/nightly.yml`) usa exactamente esa imagen en cada push, así que BlocksDS **v1.16.0** es la versión "conocida-buena". El README solo dice "instala BlocksDS" sin versión.

**Opción B — nativo en macOS (Apple Silicon):**

1. Instalar [wonderful-toolchain](https://wonderful.asie.pl) (`wf-bootstrap` en `/opt/wonderful`), luego con `wf-pacman` el paquete `thirdparty-blocksds-toolchain`. Solo hay soporte aarch64 nativo (Intel Mac excluido) y es soporte reciente: puede tener asperezas.
2. Exportar como **variables de entorno reales** (no basta pasarlas a make — `arm7/dldi_ds_arm7.specs` lee `BLOCKSDS` vía `%:getenv` en tiempo de link):
   ```bash
   export WONDERFUL_TOOLCHAIN=/opt/wonderful
   export BLOCKSDS=/opt/wonderful/thirdparty/blocksds/core   # ¡no /opt/blocksds/core!
   ```
3. `make` (el make 3.81 de Apple funciona; los Makefiles son BSD-compatibles).

**Artefacto:** `LAUNCHER.nds` en la raíz. Para DSpico: renombrar a `_picoboot.nds` en la raíz de la SD + copiar la carpeta `_pico/` del repo. **Ojo:** el launcher solo no sirve — los binarios de Pico Loader (`aplist.bin`, `savelist.bin`, `picoLoader7.bin`, `picoLoader9.bin`, de los releases de [pico-loader](https://github.com/LNH-team/pico-loader)) deben estar en `/_pico/`.

**Trampas del build:**
- **Icono del cartucho** (`GAME_ICON`, banner de `ndstool -b`): acepta BMP/GIF/**PNG**, máximo 16 colores en total. La transparencia solo funciona vía **canal alpha** (umbral 128) — un BMP plano nunca la logra (por eso el `icon.bmp` original renderiza con fondo rosa opaco). ndstool ordena los colores por valor BGR ascendente y reserva el índice 0 para lo transparente. Usar `tools/png2icon.py`: produce un PNG 32×32 con ≤15 colores opacos + alpha, verificado con ndstool v1.16.0.
- Cada PNG en `arm9/gfx/` necesita un `.grit` pareado; sin él la regla nunca existe y falla el header generado.
- El target `sdimage` (imagen FAT para no$gba) espera un directorio `sdroot/` que no existe en el repo.
- La regla de audio de `Makefile.arm9:77` usa regex de GNU find (`\|`) — rompería en macOS, pero está dormida (`AUDIODIRS` vacío).

## 2. Arquitectura en una página

```
ARM9 (67 MHz, hace TODO)              ARM7 (sirviente mínimo)
├─ UI completa (App)                  ├─ Muestreo touch/botones → memoria compartida
├─ FatFs (montado en ARM9)            ├─ Sectores SD crudos (driver DLDI subido a 0x037F8000)
├─ Decodificación BGM (BCSTM)         ├─ RTC (solo lectura, canal IPC 20)
├─ Heap TLSF (~4 MB DS / 16 MB DSi)   └─ Escritura de registros de sonido (canal 19)
└─ IPC: RPC síncrono por FIFO, structs alignas(32), coherencia de caché MANUAL
```

- **Procesos**: `IProcess` con `Run()` (loop bloqueante) y `Exit()`. `gProcessManager.Goto<T>()` cambia de proceso (teardown completo + fade). Hay 3: `App` (navegador), `SettingsProcess` (selector de temas), `PicoLoaderProcess` (terminal — **nunca vuelve**).
- **DI**: Boost.DI en `arm9/source/services/process/ProcessFactory.thumb.cpp` — ahí se registran procesos (`REGISTER_PROCESS`) y singletons (`JsonAppSettingsService` → `/_pico/settings.json`, `BgmService`).
- **Flujo de lanzamiento** (un solo cuello de botella, útil para hooks):
  `RomBrowserItemViewModel::Activate()` → `RomBrowserController::LaunchFile` → FSM `Launching` → `HandleLaunchTrigger` (`RomBrowserController.cpp:208`, en hilo IO): escribe `lastUsedFilePath` en settings.json → `FileType::TrySetLaunchParameters` → **único** `Goto<PicoLoaderProcess>()` (línea 252) → `pload_start()` copia `picoLoader9/7.bin` a VRAM y salta. El "volver al launcher" es un **reinicio en frío** del .nds del launcher: no se conserva ningún estado en memoria.
- **GUI**: árbol de vistas retenido con render inmediato por frame. 3 mecanismos: capas BG 2D (fondos), sprites OAM (arena por frame), quads 3D (solo pantalla táctil — el motor principal está mapeado abajo). Texto NFT2 rasterizado por CPU. `RecyclerView` estilo Android con adapters. **No existe widget de teclado.** No hay sistema de layout: los padres posicionan hijos en `Update()`.
- **Temas**: `/_pico/themes/<carpeta>/theme.json` + assets `.bin` que son volcados crudos en formato de hardware NDS (15bpp, A3I5/A5I3 — se crean con NitroPaint; no hay conversor en el repo). Todo tema (incluso custom) deriva su croma de un **port completo de material-color-utilities de Google** corriendo CAM16/HCT en el ARM9. Cambiar de tema = reinicio del proceso App. `topBackgroundType` está parseado pero ignorado (extensión anticipada y nunca terminada).
- **Settings**: un solo `/_pico/settings.json` con ArduinoJson 6.20.1, pool fijo de **2048 bytes** (¡desborde = truncado silencioso!), sin ints de 64 bits, reescrito completo en cada `Save()`. Claves: `language`, `theme`, `lastUsedFilePath`, `romBrowserLayout`, `romBrowserSortMode`, `fileAssociations`.
- **Asociaciones de archivos**: mapa extensión→appPath en settings.json, materializado como `CustomFileType` al escanear; el ROM elegido viaja por `argv` al emulador. **Las extensiones sin asociación son invisibles** en el navegador (se filtran como Unknown) — incluso `.gba`.
- **Arte por juego** (ya soportado, solo convención de archivos):
  - Carátulas: `/_pico/covers/{nds,gba}/<GAMECODE>.bmp` o `/_pico/covers/user/<nombre-completo>.bmp` — BMP 128×96, 8bpp, sin comprimir (se muestra 106×96).
  - Iconos: `/_pico/icons/...` — BMP 32×32, 4bpp, color 0 = transparente.
  - Banners: `/_pico/banners/...` — `.bnr` estándar NDS(i), soporta iconos animados DSi.
  - Carpetas: `icon.bmp` / `banner.bnr` dentro de la carpeta misma.
- **BGM**: `.bcstm` (DSP-ADPCM, con loop) en `<tema>/bgm/`, uno al azar por arranque, streaming desde SD con decoder en ensamblador. `IBgmService::StartBgm(path)` existe pero **nadie lo llama** — gancho listo para música por carpeta. No hay efectos de sonido de UI.
- **RTC**: `rtc_readDateTime()` (`arm9/source/rtcIpc.cpp`) funciona y es thread-safe; hoy solo siembra el RNG. Los timestamps de FAT están **deshabilitados** (`FF_FS_NORTC=1` → todo archivo se estampa 2018): cualquier "jugado recientemente" debe persistir sus propias fechas.

## 3. El launcher ya dejó migas de pan

Hallazgos que confirman que varias de "nuestras" ideas ya estaban planeadas upstream:

- `heartIcon` y `recentIcon` **ya están compilados** en el binario; los botones `APP_BAR_BUTTON_FAVORITE` / `APP_BAR_BUTTON_RECENT` existen escritos pero **comentados** en `romBrowser/views/RomBrowserAppBarView.{h,cpp}`.
- `NdsGameDetailsBottomSheetView` (ficha de juego con chip "Favorite") existe como código muerto — `App.cpp:297-300` lo tiene comentado y abre la hoja de cheats en su lugar.
- `DisplaySettingsBottomSheetView.cpp:37-38` reserva `FILTERS_LABEL_X/Y` para una fila de filtros que nunca se construyó.
- `RomBrowserSortMode::LastModified` se persiste y parsea, pero `SdFolder::FilterAndSort` solo implementa orden por nombre — **cae silenciosamente a nombre** (FileInfo ni siquiera guarda timestamp).
- `RomBrowserLayout::FileList` está en el enum y el serializador, pero la fábrica devuelve `nullptr` → un settings.json editado a mano con "FileList" crashea.
- Bug probable en `arm9/source/bgm/AudioStreamPlayer.cpp:138-139`: `DC_FlushRange(&blockPtrL, …)` flushea la dirección de la variable puntero, no el buffer de audio. Candidato a primer PR upstream.

## 4. Puntos de extensión, ordenados por dificultad

| Feature | Dónde | Dificultad |
|---|---|---|
| Nuevo setting global escalar | `AppSettings.h` + `JsonAppSettingsSerializer.thumb.cpp` (patrón `json[KEY] \| default`) | Trivial |
| Línea de texto de estado ("412 juegos") | Copiar patrón de `settings/views/ThemeListTopView.cpp:13-20` en `RomBrowserTopScreenView` (hay que pasarle el font repository por ctor) | Fácil |
| "Sorpréndeme" (juego aleatorio) | `gRandomGenerator` listo; input en `RomBrowserBottomScreenView::HandleInput` → juntar items con clasificación Game → `LaunchFile` | Fácil |
| Fondo según hora del día (al arrancar) | `rtc_readDateTime` + elegir `topbg_day/night.bin` en `CustomSubBackground::LoadResources` | Fácil |
| Música por carpeta | Inyectar `IBgmService` en `RomBrowserController`, llamar `StartBgm(path)` desde `HandleNavigateTrigger` | Fácil–Medio |
| Volumen / fade de BGM | Volumen hardcodeado a 127; nuevo comando IPC en `SoundIpcService` | Fácil |
| Registro de lanzamientos (path + fecha + contador) | Hook en `HandleLaunchTrigger`/`UpdateLastUsedFilepath` (`RomBrowserController.cpp:231-242`) — ya escribe settings.json ahí, mismo hilo IO, RTC disponible | Fácil–Medio |
| Favoritos persistentes | Persistencia por gamecode + toggle (X libre) + badge en vistas de tema + botón ya comentado en app bar | Medio |
| Nueva hoja inferior (bottom sheet) | Subclase de `BottomSheetView` + `DialogPresenter::ShowDialog` (modelo: `CheatsBottomSheetView`) | Medio |
| Dashboard como proceso nuevo | Copiar `SettingsProcess` (~330 líneas de setup duplicado; no hay clase base) | Medio–Difícil |
| Carpeta virtual "Recientes" | Difícil: todo el pipeline asume cwd de FatFs = carpeta navegada; `FileInfo` no guarda rutas completas (`FastFileRef` = sector+cluster) | Difícil |
| Búsqueda con texto libre | No existe widget de teclado en todo el codebase — habría que construirlo | Difícil |
| Tiempo total jugado | **Imposible medir duración sin tocar Pico Loader/firmware**: el launcher muere al lanzar y renace en frío. Solo se pueden registrar timestamps de lanzamiento y aproximar con "delta al siguiente arranque" | Difícil/limitado |

**Regla de oro para metadatos por juego** (favoritos, contador, última vez): NO crecer `settings.json` (pool de 2048 bytes trunca en silencio). Crear un archivo aparte (p. ej. `/_pico/gamedata.json` o binario de registros fijos), con clave = gamecode de 4 chars (misma convención que `/_pico/icons/nds/`) y fallback a nombre de archivo. Sin epoch de 64 bits: guardar `{y,m,d,h,min,s}`.

## 5. Disciplinas del codebase (para no romper nada)

- IPC entre CPUs: structs `alignas(32)` con padding, `DC_FlushRange`/`DC_InvalidateRange` manuales, un solo request pendiente por canal.
- VRAM usa asignadores de pila (bump): no se libera individualmente, solo por snapshot/restore — el orden de construcción de vistas importa.
- Los glifos de las etiquetas se dibujan como gradiente fondo→texto (no alpha): el color de fondo de la etiqueta debe coincidir con lo que hay detrás.
- Todo IO de archivos va en `_ioTaskQueue` (nunca en el hilo de UI); el destructor de TaskQueue hace join, garantizando que la escritura pre-lanzamiento termina.
- Código frío se compila como Thumb con `#pragma GCC optimize("Os")` (convención `.thumb.cpp`); hot paths pueden optar por ARM renombrando a `.arm.cpp`.
- `ProcessManager::MainLoop` reconstruye el mismo proceso para siempre si `Run()` retorna sin llamar `Goto` antes.

## 6. Ecosistema (verificado en la web, jul-2026)

- Hardware + firmware DSpico: 100 % open source (KiCad, Zlib/CC-BY-SA) — cierto.
- RP2040 con firmware propio, detecta DS/DSi/3DS, actualizable por USB (UF2) — cierto.
- Firmware / Pico Loader / Pico Launcher: 3 componentes con versiones y updates independientes — cierto.
- GBARunner3: activo y funciona en DSpico, pero la comparación va al revés de lo que suele decirse: en general tiene **mejor** audio/compatibilidad que GBARunner2, aunque sigue en beta y algunos juegos van mejor en el 2 — tener ambos sigue siendo el consejo correcto.
- Editor web de temas: existe ("DS Pico Theme Creator") + biblioteca comunitaria en themes.flashcarts.net/pico.
- Cheats: sí (Loader API v3). **Soft reset e in-game reset: NO existen aún** (las guías lo dicen explícitamente) — ojo con esa expectativa.
