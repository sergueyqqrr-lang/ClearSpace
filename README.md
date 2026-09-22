# ClearSpace — Fase 0 + 1 + 3 (MVP funcional, visualización con eje log)

Plugin de sidechain espectral enfocado en resolver masking entre dos
pistas (ej. kick/bajo). Ver la conversación de diseño para arquitectura
completa; este README cubre solo cómo levantar el proyecto.

**Estado: compila y enlaza limpio en Linux (VST3 + Standalone) contra
JUCE 8.0.4, verificado con una build real.** El DSP ya "talla" espacio
real entre main y sidechain, y la UI ya muestra el análisis real (no
un placeholder).

## Qué incluye

- Proyecto JUCE + CMake listo para compilar (VST3 / AU / Standalone).
- Bus de sidechain declarado y validado en `isBusesLayoutSupported`.
- Parámetros completos vía `AudioProcessorValueTreeState`: Carve, Mode
  (Dynamic/Static), Smoothness, Listen, Bypass, Output Gain.
- Pipeline de DSP funcional (Source/DSP/):
  - `BandMapper`: mapea bins FFT a ~32 bandas perceptuales (Bark).
  - `SpectralAnalyzer`: FFT (1024 pts, ventana Hann, hop 256 = 75%
    overlap) sobre main y sidechain por separado, fuera del camino de
    la señal de salida (no añade latencia).
  - `MaskingDetector`: índice de conflicto por banda (solapamiento ×
    dominancia del sidechain).
  - `SmoothingEnvelope` + `CarveCurveGenerator`: convierten el
    conflicto + Carve + Mode en una curva de reducción de ganancia
    (dB) por banda, con attack/release controlados por Smoothness.
  - `DynamicFilterBank`: banco de filtros IIR tipo campana (uno por
    banda), coeficientes recalculados a la tasa de hop — Opción A del
    enfoque técnico (bajo CPU, baja latencia).
- Modo Listen implementado (dry − processed = lo que se está quitando).
- **Visualización real (Fase 3, con eje logarítmico)**: `SpectrumDataFifo`
  (Source/DSP/) es una cola lock-free (sobre `juce::AbstractFifo`) que
  publica, cada vez que hay un nuevo frame de análisis, la energía por
  banda de main y sidechain, el mapa de conflicto, la frecuencia
  central de cada banda y la reducción de ganancia pico. El audio
  thread nunca bloquea: si la UI no ha vaciado el fifo a tiempo, el
  frame simplemente se descarta. `SpectrumDisplay` (Source/UI/) lee
  esos frames desde el `Timer` del editor (30 Hz) y dibuja main,
  sidechain (sutil) y las bandas en conflicto resaltadas, todo
  posicionado en **escala logarítmica de Hz real** (no ancho uniforme),
  con grid y etiquetas de frecuencia (50, 100, 200, 500, 1k, 2k, 5k,
  10k, 20k) acotadas al rango real de bandas para el sample rate
  activo.
- Aviso en la UI cuando no hay sidechain conectado (integrado ahora en
  el propio `SpectrumDisplay`, no como texto aparte).

## Qué NO incluye todavía (fases siguientes)

- Modo "Static" está implementado pero sin afinar por oído todavía
  (promedio de largo plazo simple; falta escuchar en mezclas reales).
- LookAndFeel custom (usa los componentes estándar de JUCE con colores
  del tema, no un diseño gráfico final).
- Profiling de CPU real y optimización (Fase 4).

## CI (GitHub Actions)

El repo incluye `.github/workflows/build.yml`, que compila el plugin en
cada push/PR a `main` en Linux, macOS y Windows (matriz), y sube los
artefactos (VST3, AU en macOS, Standalone) descargables desde la
pestaña "Actions" de GitHub. No requiere configuración adicional: usa
el mismo `CMakeLists.txt` de este repo, con JUCE descargado vía
FetchContent (cacheado entre corridas para no volver a bajarlo cada
vez).

## Cómo compilar

1. Clona JUCE como submódulo (o deja que CMake lo descargue solo vía
   FetchContent, ya configurado como fallback):
   ```bash
   git submodule add https://github.com/juce-framework/JUCE.git JUCE
   ```
2. Configura y compila:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   ```
3. El VST3/AU/Standalone se copian automáticamente a las carpetas de
   plugins del sistema (gracias a `COPY_PLUGIN_AFTER_BUILD TRUE`).

## Próximo paso sugerido (Fase 4)

1. Probar con audio real (kick + bajo, o voz + pads) y afinar por oído:
   - `kMaxReductionDb` en `CarveCurveGenerator.h` (actualmente 18 dB).
   - El `Q` de los filtros en `DynamicFilterBank.h` (actualmente 1.5).
   - Los tiempos de attack/release en `SmoothingEnvelope.h`.
2. Conectar el modo Static a un análisis más robusto (detectar la
   banda de mayor conflicto histórico en vez de un promedio simple
   por banda).
3. Profiling de CPU real en un DAW (Ableton, Logic, Reaper) y afinar
   el número de bandas / tamaño de FFT si hace falta.
