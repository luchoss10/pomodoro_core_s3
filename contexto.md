# Proyectos y Desarrollo en M5Stack CoreS3

## 1. Proyectos con M5Stack CoreS3

**Usuario:**
> m5stack core s3 projects

**Asistente:**
El **M5Stack CoreS3** es un potente kit de desarrollo IoT todo en uno basado en el procesador de doble núcleo **ESP32-S3**. Cuenta con una pantalla táctil capacitiva de 2 pulgadas, cámara integrada, micrófonos duales, altavoz, una IMU de 6 ejes y puertos de expansión Grove, lo que lo hace ideal para *edge computing*, visión artificial y proyectos interactivos.

Proyectos populares y creativos construidos con el M5Stack CoreS3 incluyen:

### Asistentes de IA y Robots de Chat
* **CoCube AI Chat Robot:** Uso del CoreS3 como cerebro para comunicarse directamente con Modelos de Lenguaje Grande (LLMs) a través de Wi-Fi.
* **Asistente de IA por Voz y Visión:** Aprovechamiento de la cámara integrada, micrófonos y altavoz para ejecutar compañeros de IA interactivos locales o en la nube (como Stack-chan o interfaces de chatbot personalizadas).

### Hogar Inteligente y Paneles Táctiles
* **Homepoint / Paneles Táctiles:** Controlador táctil MQTT y HomeKit de escritorio o pared para gestionar luces inteligentes, interruptores y dispositivos de climatización.
* **Centro Ambiental y de Sensores:** Conexión de unidades externas I2C / Grove (sensores ENV, lectores RFID o monitores de energía AC) para rastrear métricas y mostrar datos en vivo en la pantalla IPS de 320x242.

### Robótica y Compañeros de Escritorio
* **Robot de Escritorio Stack-chan:** Uso del CoreS3 como cabeza animada y controlador de IA para robots de escritorio impresos en 3D.
* **Cerebro Robótico Multisensor:** Emparejamiento del CoreS3 con controladores secundarios (como Arduino Mega) vía I2C para manejar servos de alto torque y matrices complejas de sensores.

### Plataformas de Programación
* **UIFlow:** Herramienta visual basada en bloques de M5Stack para prototipado rápido.
* **Arduino IDE:** Instalando el paquete de placas ESP32 y la librería oficial M5CoreS3.
* **MicroPython:** Para control ligero del dispositivo basado en scripts.
* **M5Burner:** Utilidad oficial de escritorio para flashear aplicaciones precompiladas e imágenes del sistema.

---

## 2. Requerimientos de Memoria Externa (MicroSD)

**Usuario:**
> Necesito comprar una memoria externa para la misma? una micro sd?

**Asistente:**
**No, no es estrictamente obligatorio** comprar una tarjeta MicroSD para que el M5Stack CoreS3 funcione. El dispositivo ya viene equipado de fábrica con **16 MB de memoria Flash interna** y **8 MB de PSRAM**, capacidad más que suficiente para almacenar códigos, interfaces de UIFlow, scripts de MicroPython y pequeños *assets* gráficos.

Sin embargo, **sí necesitarás comprar una MicroSD** si tu proyecto entra en alguna de estas categorías:
* **Proyectos con la cámara integrada:** Captura de fotos, video, o almacenamiento de datos para visión artificial.
* **Audio y multimedia:** Almacenamiento de archivos de audio (`.mp3` o `.wav`) para reproducirlos en el parlante.
* **Data Logging masivo:** Registro continuo de datos de sensores ambientales por largos periodos.

### Requisitos y limitaciones de la MicroSD
* **Capacidad máxima recomendada:** Oficialmente hasta **16 GB** (la comunidad reporta compatibilidad hasta **32 GB**). Evita tarjetas de 64 GB o superiores (SDXC).
* **Formateo obligatorio:** Sistema de archivos **FAT32**.
* **Marcas sugeridas:** Optar por tarjetas **SanDisk (como SanDisk Ultra)**, ya que el lector SPI del CoreS3 suele dar fallos de tiempos de respuesta con marcas como PNY o Verbatim.

---

## 3. Diseño de App: Reloj, Calendario y Pomodoro Adaptable

**Usuario:**
> Bueno primero me gustaria probar realizar una pequeña app de reloj, calendario y pomodoro. Dependiendo de la posicon del dispositivo se incia el pomodoro para sesines 15 u 30 minutos, de maner adaptable puedo. desactivar el pomodoro, mopdifica los tiempos de los mismos, ver solo la hora o el calendario. y desactivar el pomodoro o salirme

**Asistente:**
Para esta aplicación en específico (reloj, calendario y Pomodoro táctil), **no necesitas una tarjeta MicroSD**. La memoria flash interna de 16 MB es suficiente.

La arquitectura recomendada se divide en tres componentes clave:

### 1. Interfaz Gráfica (UI) y Estados
* **UIFlow 2.0:** Diseño visual de las pantallas y detección de toques mediante eventos.
* **Arduino / C++ (LVGL o M5GFX):** Creación de widgets o dibujo directo de primitivas y relojes mediante código.

### 2. Control de Tiempos (Reloj y Calendario)
* El CoreS3 cuenta con un chip **RTC (Real-Time Clock) interno (BM8563)**. Una vez configurada la hora (manualmente o vía NTP por Wi-Fi), el reloj la mantiene de forma independiente.

### 3. Lógica del Pomodoro Adaptable (Uso del IMU)
Lectura de datos del acelerómetro integrado **BMI270**:
* **Base horizontal:** Estado normal (Reloj/Calendario).
* **Giro de 90° a la izquierda:** Gravedad en eje X negativo -> Activa sesión de **15 minutos**.
* **Giro de 90° a la derecha:** Gravedad en eje X positivo -> Activa sesión de **30 minutos**.
* **Regreso a base plana:** Cambio de gravedad en eje Z -> Desactiva el Pomodoro y vuelve al reloj.

---

## 4. Desarrollo en Código Puro (VS Code / Neovim + PlatformIO)

**Usuario:**
> Soy mas de codigo puro. Aunque puede que se me dificulte la interfaz grafica. La idea seria usar el ide de arduino. O si puedo hacerlo en vscode o nvim mejor.

**Asistente:**
**Escribir código puro en VS Code o Neovim es completamente viable** y mucho más cómodo usando **PlatformIO**.

### Configuración de `platformio.ini`
```ini
[env:m5stack-cores3]
platform = espressif32
board = m5stack-cores3
framework = arduino
monitor_speed = 115200
lib_deps =
    m5stack/M5CoreS3 @ ^1.0.0
```
* **Neovim:** Uso de `mason.nvim` con `platformio` CLI y el LSP `clangd` para autocompletado nativo.
* **VS Code:** Extensión oficial de PlatformIO.

### Interfaz Gráfica con `M5GFX`
* Evita redibujar toda la pantalla en cada ciclo; actualiza únicamente las regiones que cambian.
* Define zonas táctiles mediante coordenadas lógicas evaluando `M5.Touch.getDetail()`.

### Esqueleto de Máquina de Estados en C++
```cpp
#include <M5CoreS3.h>

enum class Mode { ClockOnly, CalendarOnly, PomodoroActive, Settings };
Mode currentMode = Mode::ClockOnly;

void setup() {
    auto cfg = M5.config();
    CoreS3.begin(cfg);
    CoreS3.Display.setTextColor(WHITE);
    CoreS3.Display.setTextSize(2);
}

void loop() {
    CoreS3.update(); // Actualiza sensores, táctil e IMU
    
    // 1. Leer posición del IMU para cambiar el estado del Pomodoro
    // 2. Comprobar zonas de toque en pantalla para salir o modificar tiempos
    
    switch (currentMode) {
        case Mode::ClockOnly:
            // Lógica y renderizado del reloj
            break;
        case Mode::CalendarOnly:
            // Lógica y renderizado del calendario
            break;
        case Mode::PomodoroActive:
            // Descontar segundos y renderizar progreso
            break;
        case Mode::Settings:
            // Interfaz de configuración de minutos
            break;
    }
    delay(10);
}
```

---

## 5. Programación del M5Stack CoreS3 en Rust

**Usuario:**
> Oye puedo programar para esto en rust?

**Asistente:**
**Sí, puedes programar el M5Stack CoreS3 completamente en Rust.** El ESP32-S3 tiene soporte oficial de Espressif mediante el proyecto `esp-rs` y *crates* de la comunidad.

### Rutas en el ecosistema Rust (`esp-rs`)
Instalando el toolchain Xtensa mediante `espup`, existen dos enfoques:
1. **`no_std` (Bare-metal con `esp-hal`):** Sin sistema operativo detrás, muy ligero y rápido. Ideal junto con `embassy` para concurrencia asíncrona.
2. **`std` (Sobre `esp-idf-svc`):** Envuelve ESP-IDF y FreeRTOS. Permite usar la librería estándar de Rust (`Vec`, `String`, hilos, red Wi-Fi completa de forma sencilla).

### Crates clave para el CoreS3
* **Soporte de placa (BSP):** Crates como [`core-s3`](https://docs.rs), [`core-s3-rs`](https://github.com) y [`m5stack-core`](https://crates.io) abstraen pines, I2C, la gestión de energía (**AXP2101**) y el expansor GPIO (**AW9523B**) que enciende la pantalla.
* **Interfaz Gráfica:** **`embedded-graphics`** para dibujar textos, formas y barras de progreso sobre el controlador de pantalla **ILI9342C**.
* **Sensores:** Drivers independientes en `crates.io` para el acelerómetro **BMI270**, el panel táctil **FT6336/FT5336** y el reloj **BM8563**.

### Flujo de trabajo en terminal
* **Generar plantilla:** `cargo generate esp-rs/esp-template` (`no_std`) o `esp-idf-template` (`std`).
* **Compilar y flashear:** `cargo run --release` usando [`espflash`](https://github.com) como *runner*.

*Nota de hardware:* El CoreS3 requiere inicializar por I2C los chips **AXP2101** y **AW9523B** para alimentar la pantalla; usar un BSP como `core-s3` evita tener que escribir esta secuencia desde cero.

