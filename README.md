# Pomodoro Core S3

Reloj, calendario y Pomodoro adaptable para el **M5Stack CoreS3** (ESP32-S3).
La posición del dispositivo, leída con el IMU, decide qué se muestra y rota la interfaz para que siempre quede legible.

## Comportamiento

La **posición base** es la de la cámara arriba (pantalla horizontal, 320x240).

| Situación | Qué hace |
|---|---|
| Base | Reloj con segundos y fecha. Arriba, dos cuadros pequeños indican qué Pomodoro se activa al girar a cada lado (`◄ 30` a la izquierda y `15 ►` a la derecha, con los minutos configurados). |
| Girado a la izquierda | Inicia el Pomodoro largo (30 min por defecto) y **rota la interfaz** para que quede legible. |
| Girado a la derecha | Inicia el Pomodoro corto (15 min por defecto) y rota la interfaz. |
| Volver a la base | **Pausa** el Pomodoro y conserva el progreso. Regresa al reloj. |
| Girar otra vez al mismo lado | **Continúa donde quedó.** |
| Girar al lado contrario con un Pomodoro pausado | No inicia: la pantalla rota hacia ese lado y muestra el aviso *NO SE PUEDE INICIAR* (con el icono de casa para volver a la base), hasta que el pausado se termine, se reinicie o se detenga. |

En la pantalla del Pomodoro hay un cuadro pequeño con una flecha y un icono de casa, en el lado hacia el que se debe girar para volver a la base. La interfaz solo rota en esa pantalla y en el aviso de bloqueo; el reloj, el calendario y los ajustes siempre se ven en la base.

### Sesión de Pomodoro

Hay **una sola sesión a la vez**, asociada al lado que la inició.

- Mientras está pausada, el reloj muestra `POMO 15 PAUSADO 08:12` y, sobre el botón CALEND., un botón **DETENER POMO** que descarta la sesión y libera el otro lado sin tener que girar de vuelta.
- Si el tiempo llega a 00:00, la sesión queda como cumplida. Al volver a la base se cierra y ya se puede iniciar el otro lado.
- Si se gira de un lado a otro sin pasar por la base, la sesión del primer lado también se pausa.
- El progreso vive en RAM: se pierde al reiniciar el equipo.

### Sonidos

| Evento | Sonido |
|---|---|
| Inicia una sesión nueva, o se completa sin detenerla | Ascendente (Do, Mi, Sol) |
| Se pulsa REINICIAR o DETENER | Descendente (La, Fa, Do) |

Pausar (volver a la base) y reanudar (girar otra vez al mismo lado) no suenan. El volumen tiene 5 niveles (0 = silencio, 64, 128, 192 y 255) y se elige en **Ajustes → SONIDO**; al cambiarlo suena una muestra. El nivel actual se ve como un icono de altavoz con barras en la barra superior de indicadores.

### Indicadores (barra superior)

En el reloj, el calendario y la pantalla del Pomodoro, arriba al centro, hay dos indicadores:

- **Volumen:** altavoz con barras (tachado si está en silencio).
- **Batería:** icono con el porcentaje. Verde si es de 40 % o más, naranja de 20 a 39 %, rojo por debajo, y un rayo amarillo mientras carga. Es una estimación a partir del voltaje; se lee cada 5 s.

### Modo ahorro

Botón **MODO AHORRO**, sobre el botón IMU, solo en la pantalla del reloj (forma de "L" acostada con la barra inferior). Muestra únicamente **HH:MM** en grande, sin segundos, con el indicador de batería arriba al centro, y:

- Brillo igual al nivel 20 % de Ajustes (el mínimo real, 2.5 V), para maximizar la autonomía. El color del texto cambia según la hora, siempre en gris (sin blanco):
  - **Día (06:00-18:59):** gris claro, para leerlo de día.
  - **Tarde (19:00-21:59):** gris medio.
  - **Noche (22:00-05:59):** gris oscuro.
  Las franjas, el brillo y los colores están en `config.h` (`LP_*`). CPU a 80 MHz (en modo normal trabaja a 160 MHz; ver `NORMAL_CPU_MHZ` en `config.h`).
- Se redibuja una vez por minuto (la batería se actualiza con él) y, entre minutos, la CPU **duerme** (sueño ligero).
- Sin sonido, sin IMU (no se inicia ningún Pomodoro) y sin Wi-Fi (si había una sincronización en curso se cancela; se retoma al salir).
- El botón **SALIR** (esquina inferior derecha) vuelve al reloj y restaura el brillo elegido en Ajustes.

El sueño ligero no pierde el táctil: en el CoreS3 la señal del panel táctil llega al ESP32 por GPIO21 y lo despierta, así que SALIR responde al primer toque.

### Botones táctiles

**En la base (reloj y calendario):**
- **CALEND. / RELOJ:** alterna entre reloj y calendario.
- **AJUSTES / VOLVER:** abre o cierra los ajustes: minutos de cada Pomodoro (pasos de 5; izquierda 10 a 90, derecha 5 a 60), volumen del sonido (5 niveles) y **brillo** de la pantalla (5 niveles, de 20 % a 100 %; por defecto 60 %; se aplica al instante). Arriba a la derecha muestra el voltaje y el porcentaje de la batería, para pruebas de descarga. Una sesión pausada conserva los minutos con los que empezó.
- **IMU ON / IMU OFF:** activa o desactiva el inicio automático por giro.
- **MODO AHORRO:** solo en el reloj; ver la sección anterior.
- **DETENER POMO:** solo en el reloj y solo si hay un Pomodoro pausado; descarta la sesión.

**En la pantalla del Pomodoro:**
- **REINICIAR:** vuelve al tiempo completo y arranca solo.
- **DETENER:** descarta la sesión, vuelve al reloj y libera el otro lado.

## Estructura del proyecto

```
pomodoro_core_s3/
├── platformio.ini        # Placa, framework y librerías
├── src/
│   ├── main.cpp          # setup() y loop(): coordina los módulos
│   ├── config.h          # Constantes: umbrales, minutos por defecto, rotaciones
│   ├── settings.h/.cpp   # Ajustes (minutos, volumen, brillo) y su guardado en NVS
│   ├── orientation.h/.cpp# Lectura del IMU y orientación estable
│   ├── pomodoro.h/.cpp   # Lógica del temporizador
│   ├── sound.h/.cpp      # Sonidos del altavoz
│   ├── timesync.h/.cpp   # Hora: RTC + sincronización NTP (Bogotá, UTC-5)
│   ├── power.h/.cpp      # Energía: sueño ligero, salida de 5 V
│   ├── secrets.example.h # Plantilla de credenciales Wi-Fi (secrets.h no se sube)
│   └── ui.h/.cpp         # Vistas, botones táctiles, rotación e indicadores
├── docs/
│   ├── iniciar-usbipd.md # Conectar el CoreS3 a WSL
│   └── detener-usbipd.md # Desconectarlo y cerrar el servicio de forma segura
└── contexto.md           # Conversación de contexto inicial del proyecto
```

## Módulos

Cada módulo tiene una sola responsabilidad.

### `config.h`
- `TILT_ENTER` / `TILT_EXIT`: umbrales del eje X (0.75 g y 0.35 g, como en la v2) y `ORIENT_STABLE_MS`.
- `POMODORO_LEFT_MIN` / `POMODORO_RIGHT_MIN`: minutos por defecto.
- `ROT_BASE` y `ROT_OFFSET_LEFT/RIGHT`: rotación de pantalla (0 a 3).

### `settings.h`
Estructura `Settings` con los minutos de cada lado, el volumen, el brillo y si el IMU está activo.

**Persistencia (NVS):** los minutos de cada Pomodoro, el nivel de volumen y el de brillo se guardan en la memoria no volátil cada vez que se cambian en Ajustes, y se cargan al arrancar (`settingsLoad`). Cada valor se valida al cargarlo: si está fuera de rango, se usa el valor por defecto. El estado del IMU (ON/OFF) no se guarda: siempre arranca en ON.

### `orientation`
- `classify(ax, actual)`: devuelve `Base`, `Left` o `Right` a partir del eje X, con histéresis (entre los dos umbrales conserva el estado anterior).
- `updateOrientation()`: devuelve la orientación **estable** (debe mantenerse `ORIENT_STABLE_MS`). Imprime por serial `ax/ay/az` cada segundo, para calibrar.

### `pomodoro`
Clase `Pomodoro`, sin dibujar ni leer sensores. Máquina de estados `PomoState`: `Idle`, `Running`, `Paused` y `Finished`.
- `start(minutos, lado)`, `pause()`, `resume()`, `restart()` y `stop()`.
- `state()`, `side()` y `minutes()`.
- `remainingSec()` y `remainingRatio()`.
- `update()`: pasa a `Finished` al llegar a cero.

Guarda el tiempo acumulado en tramos anteriores, y por eso `pause()` y `resume()` conservan el progreso.

### `sound`
`soundPlay(Sfx::Chime | Sfx::Cancel)` encola una melodía corta de tres notas y `soundUpdate()` la avanza en cada vuelta del `loop`, sin bloquear. Cinco niveles de volumen (`SOUND_VOLUMES` en `config.h`); el nivel 0 silencia.

### `timesync`
Mantiene la hora del reloj. `timeSyncInit()` solo corrige el RTC si no tiene hora válida (usa la última hora guardada en NVS o la fecha de compilación). `timeSyncUpdate()` se llama en cada vuelta del `loop` y, **sin bloquear**, conecta el Wi-Fi, pide la hora por NTP, la guarda en el RTC y apaga el Wi-Fi. Se repite cada 12 h. Si falla, reintenta con espera creciente (a los 30 s, 1 min y 2 min); si el último intento también falla, no toca la hora (se conserva la guardada) y espera al siguiente ciclo. Mientras espera, el estado bajo la fecha muestra una **cuenta regresiva** hasta el próximo intento (por ejemplo `Wi-Fi: no conecta (cod 6) - reintento en 27s`, o `Sin NTP: hora guardada - prox. en 11h 59m` si se agotaron los reintentos).

### `ui`
Todo el dibujo, en un buffer (`M5Canvas`) que se envía de una vez, sin parpadeo.
- `uiSetOrientation(o)`: aplica la rotación de pantalla.
- `uiDraw(...)`: dibuja reloj, calendario, ajustes o Pomodoro, la barra inferior, el indicador de pausa y el aviso de bloqueo.
- `uiHit(x, y)`: devuelve el botón tocado según el último dibujo.

### `main.cpp`
Máquina de estados de pantalla (`CLOCK`, `CALENDAR`, `POMODORO`, `SETTINGS`), táctil y `loop()`. Decide qué hacer con la sesión cuando cambia la orientación (iniciar, pausar, reanudar o bloquear). Redibuja solo cuando cambia lo que se ve (una vez por segundo en el reloj y el Pomodoro; en el calendario y los ajustes, solo al tocar o cada 30 s) y, con batería, la CPU duerme 150 ms entre vueltas (`LOOP_SLEEP_MS`). Ver la sección Energía.

```
IMU ──► orientation ──► main ──► pomodoro
touch ─────────────────► │
                         └────► ui (rota, dibuja y resuelve botones)
```

## Requisitos

- M5Stack CoreS3 con cable USB de datos.
- [PlatformIO](https://platformio.org/) (extensión de VS Code o CLI).
- En WSL, pasar el USB con `usbipd` (ver `docs/`).

## Compilar y flashear

```bash
pio run                  # compilar
pio run -t upload        # compilar y flashear
pio device monitor       # monitor serial
```

Si `pio` no está en el PATH: `export PATH="$HOME/.platformio/penv/bin:$PATH"`.

## Energía

La app apaga lo que no usa y duerme la CPU cuando no hay nada que hacer:

| Medida | Dónde | Efecto |
|---|---|---|
| Sueño ligero entre vueltas del `loop` (150 ms) y hasta el siguiente minuto en modo ahorro | `power.cpp`, `main.cpp` | La CPU pasa de ~20-24 mA activa a < 1 mA dormida. El táctil la despierta. |
| Salida de 5 V (Grove/bus) apagada si hay batería | `powerUpdate()` | El elevador SY7088 deja de funcionar sin carga. |
| Amplificador del altavoz solo mientras suena | `sound.cpp` (`Speaker.end()`) | El AW88298 no queda encendido entre sonidos. |
| Micrófono sin inicializar | `setup()` (`cfg.internal_mic = false`) | El ES7210 no se configura. |
| CPU a 160 MHz (normal) / 80 MHz (ahorro) | `config.h` | Menos consumo con la CPU activa. |

**Excepciones:** con USB conectado no hay sueño ligero (corta la conexión USB y dificultaría flashear y usar el monitor serial), ni mientras suena una melodía o hay una sincronización Wi-Fi en curso.

**Brillo:** la librería controla la luz de fondo con el voltaje de DLDO1, de 2.5 V (valores 1 a 22) a 3.3 V (255). Los 5 niveles de Ajustes reparten ese rango (`BRIGHTNESS_VALUES`) y la interfaz los muestra como 20 % a 100 %. El color de los píxeles no cambia el consumo en una LCD con retroiluminación: la paleta gris del modo ahorro es para la vista de noche; lo que ahorra es bajar la luz de fondo.

**Cómo medir la autonomía:** el CoreS3 no mide su propia corriente. Prueba de descarga: carga al 100 %, desconecta el USB, deja el equipo 60 min en un modo y compara el voltaje y el porcentaje de Ajustes al inicio y al final.

### Mediciones

Pruebas de descarga de 60 min con batería (sin USB), batería de 500 mAh:

| Modo | Firmware | Resultado en 60 min | Consumo medio aprox. | Autonomía estimada |
|---|---|---|---|---|
| Modo ahorro | Antes de las mejoras de energía | — (uso real: duró 2 a 3 h) | ~150 a 200 mA | 2 a 3 h (observado) |
| Modo ahorro | Con sueño ligero, 5 V apagada, amplificador bajo demanda | 100 % → 93 % (4060 mV) | ~35 mA | ~10 a 14 h |
| Normal, brillo 40 % (pantalla del reloj) | Con las mejoras de energía | 93 % → 89 % | ~20 a 35 mA (preliminar) | ~12 a 20 h (preliminar) |

La autonomía se extrapola de una sola hora y el porcentaje de la batería no baja de forma lineal: cae más rápido cerca del 100 % y más despacio en la zona media. Por eso las dos pruebas **no son comparables entre sí**: el modo ahorro empezó en 100 % (resultado pesimista) y el normal en 93 % (resultado optimista). Lo más probable es que ambos modos estén en el orden de 12 a 16 h. Para una cifra fiable, hay que hacer una prueba larga en cada modo desde el mismo punto de partida (por ejemplo 90 % a 60 %) y comparar el voltaje.

## Configuración de la hora

El RTC del CoreS3 conserva la hora entre arranques mientras la batería tenga carga, así que no hace falta guardarla en memoria. **Deja el interruptor físico de la parte inferior en ON:** en OFF la batería queda desconectada, el equipo solo funciona con USB, el indicador marca 0-1 %, no carga y el RTC pierde la hora al desconectarlo. Para que se ajuste sola a la hora de Bogotá (UTC-5, sin horario de verano):

1. Copia `src/secrets.example.h` como `src/secrets.h` (ya existe vacío tras clonar el proyecto; está en `.gitignore`, no se sube).
2. Completa `WIFI_SSID` y `WIFI_PASSWORD`. Debe ser una red de **2.4 GHz con WPA o WPA2 personal**; no funcionan WPA2-Enterprise ni portales cautivos (un hotspot del teléfono sirve).
3. Flashea. Al arrancar se sincroniza en unos segundos, y luego cada 12 h.

Con el SSID vacío no se usa Wi-Fi y el reloj solo depende del RTC. Las credenciales se usan solo en memoria (`WiFi.persistent(false)`): el equipo no las guarda en su flash, aunque sí quedan dentro del firmware compilado, así que no compartas `firmware.bin`.

**Respaldo sin internet:** cada 30 min y tras cada sincronización se guarda la última hora conocida en la memoria no volátil (NVS). Si el RTC pierde la hora (por ejemplo, batería agotada), el equipo arranca con la mayor entre esa hora guardada y la fecha de compilación. Esa hora va atrasada lo que estuvo apagado el equipo, hasta que haya Wi-Fi para corregirla. El ESP32 por sí solo no cuenta el tiempo apagado; solo el RTC lo hace, y solo con batería. El monitor serial indica `[hora] sincronizada por NTP`.

## Calibración

- **Interfaz de lado o al revés durante el Pomodoro:** intercambia `ROT_OFFSET_LEFT` y `ROT_OFFSET_RIGHT` en `config.h` (1 y 3). Al hacerlo, invierte también la condición `arrowLeft = (o == Orientation::Left)` en `drawPomodoroView()` (`ui.cpp`), o el icono de casa quedará en el lado contrario.
- **El giro a la izquierda activa el lado equivocado:** el signo del eje X es el contrario; invierte los signos en `classify()` (`orientation.cpp`).
- El monitor serial imprime `ax=.. ay=.. az=.. orient=N` cada segundo (0 base, 1 izquierda, 2 derecha).

## Limitaciones conocidas y mejoras opcionales

El proyecto cubre todo lo planeado. Lo que sigue no está hecho y es opcional:

- **El progreso de un Pomodoro pausado vive en RAM:** si se reinicia el equipo, la sesión se pierde. Los ajustes (minutos, volumen y brillo) sí se guardan en NVS. Guardar el progreso exigiría decidir qué hacer con el tiempo que estuvo apagado.
- **Un Pomodoro pausado solo se puede detener, no reiniciar, desde la base.** Para reiniciarlo hay que girar de vuelta a su lado y pulsar REINICIAR.
- **El estado IMU ON/OFF no se guarda:** siempre arranca en ON.

## Respaldo del firmware de fábrica

Flashear sobrescribe la demo original de M5Stack. Los respaldos (`*.bin`) están en `.gitignore` y **no deben subirse al repo**, porque pueden contener datos del dispositivo.
