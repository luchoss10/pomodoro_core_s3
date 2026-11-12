# Iniciar el compartidor USB entre Windows y WSL (usbipd)

Guía para conectar el CoreS3 a WSL y empezar a trabajar (compilar, flashear, monitor serial).

> Todos los comandos de Windows se ejecutan en **PowerShell como administrador**
> (menú Inicio → `PowerShell` → clic derecho → *Ejecutar como administrador*).
> Si `usbipd` no se reconoce, usa la ruta completa:
> `& "C:\Program Files\usbipd-win\usbipd.exe"`

Para cerrar la sesión de forma segura al terminar, ver [detener-usbipd.md](detener-usbipd.md).

---

## Inicio rápido (uso diario)

```powershell
Get-Service usbipd                       # 1. Revisar que el servicio esté en Running
Start-Service usbipd                     #    (solo si estaba Stopped; ver "Servicio no está corriendo")
usbipd list                              # 2. Busca el BUSID del CoreS3 (VID:PID 303a:1001)
usbipd bind --busid 3-5                  # 3. Compartirlo (solo si estaba "Not shared")
usbipd attach --wsl --busid 3-5          # 4. Conectarlo a WSL
```

```bash
ls /dev/ttyACM*                          # 5. En WSL: debe aparecer /dev/ttyACM0
```

> El BUSID del CoreS3 ha sido `3-5`, pero puede cambiar si lo conectas a otro puerto USB.
> Confírmalo siempre con `usbipd list`.

---

## Paso a paso

### 1. Verificar que el servicio está activo

Hazlo siempre primero:

```powershell
Get-Service usbipd
```

- **`Running`:** sigue con el paso 2.
- **`Stopped`:** actívalo (ver también [Servicio no está corriendo](#servicio-no-está-corriendo)):

  ```powershell
  Set-Service usbipd -StartupType Automatic
  Start-Service usbipd
  Get-Service usbipd          # Status debe ser Running
  ```

- **Error porque no existe:** lo desinstalaste. Reinstálalo y reinicia Windows:

```powershell
winget install --exact dorssel.usbipd-win
```

### 2. Conectar el CoreS3

Conéctalo con un cable USB **de datos**. Luego:

```powershell
usbipd list
```

Busca la línea del dispositivo (`303a:1001`, *USB Serial Device (COM4), USB JTAG/serial debug unit*) y anota su BUSID. Ejemplo:

```
BUSID  VID:PID    DEVICE                                         STATE
3-5    303a:1001  USB Serial Device (COM4), USB JTAG/serial ...  Not shared
```

> Comparte **solo** el CoreS3. No toques los audífonos, la huella, la cámara ni el Bluetooth.

### 3. Compartir y conectar a WSL

Reemplaza `3-5` por tu BUSID (sin `<>`):

```powershell
usbipd bind --busid 3-5              # solo hace falta si el estado era "Not shared"
usbipd attach --wsl --busid 3-5
```

Comprueba que el estado sea `Attached`:

```powershell
usbipd list
```

Si WSL no está corriendo, abre antes una terminal de WSL y repite el `attach`.

### 4. Verificar en WSL

```bash
ls /dev/ttyACM*
```

Debe aparecer `/dev/ttyACM0`. Mientras esté conectado a WSL, Windows no ve el `COM4`; es normal.

> En zsh, no uses `ls /dev/ttyACM* /dev/ttyUSB*`: si un patrón no coincide, zsh cancela todo el comando.
> Usa `ls /dev/ttyACM*` o `ls /dev/tty{ACM,USB}* 2>/dev/null`.

### 5. Trabajar

```bash
cd ~/repos/pomodoro_core_s3
pio run -t upload          # compilar y flashear
pio device monitor         # monitor serial (Ctrl+C para salir)
```

---

## Cuándo repetir el `attach`

El `bind` queda guardado, pero el `attach` se pierde en estos casos y hay que repetirlo:

- Reiniciaste Windows o WSL (`wsl --shutdown`).
- Desconectaste y volviste a conectar el cable.
- El dispositivo se re-enumeró (por ejemplo, al entrar en modo descarga).

---

## Solución de problemas

| Problema | Qué hacer |
|---|---|
| `usbipd` no se reconoce | Cierra y abre PowerShell, usa la ruta completa o reinicia Windows. |
| `usbipd: warning: The service is currently not running; a reboot should fix that.` | El servicio está detenido. Ver la sección [Servicio no está corriendo](#servicio-no-está-corriendo). |
| `Acceso denegado` | La ventana de PowerShell no es de administrador. |
| No aparece el CoreS3 en `usbipd list` | Prueba otro cable (de datos) u otro puerto USB. |
| No aparece `/dev/ttyACM0` | Revisa que el estado sea `Attached`. Repite el `attach` y mira `dmesg \| tail`. |
| Error de conexión al flashear | Entra en modo descarga: mantén pulsado reset ~2 s (el LED verde parpadea) y repite el `attach`. |
| `Permission denied` en `/dev/ttyACM0` | Revisa que estés en el grupo `dialout` (`groups`) y reinicia WSL con `wsl --shutdown`. |
| Monitor serial vacío | Pulsa reset después de abrirlo. |

### Servicio no está corriendo

Si `usbipd` muestra:

```
usbipd: warning: The service is currently not running; a reboot should fix that.
```

es un aviso, no un error, pero `bind` y `attach` no funcionarán hasta que el servicio esté activo.

**Arréglalo sin reiniciar** (PowerShell como administrador):

```powershell
Get-Service usbipd
Set-Service usbipd -StartupType Automatic
Start-Service usbipd
Get-Service usbipd          # Status debe ser Running
```

Luego repite:

```powershell
usbipd list
usbipd attach --wsl --busid 3-5
```

**Causas probables**

- Lo detuviste con la guía de detener (`Stop-Service` y `Disabled`). Los comandos de arriba lo corrigen.
- Se reinstaló o actualizó y el servicio aún no arrancó. En ese caso lo más fiable es **reiniciar Windows**.

**Si `Start-Service` falla**

| Error | Causa y solución |
|---|---|
| `Acceso denegado` | La ventana de PowerShell no es de administrador. |
| `No se puede abrir el servicio` | El servicio no existe. Reinstala con `winget install --exact dorssel.usbipd-win` y reinicia Windows. |
| `ERROR 1058` ("No se puede iniciar el servicio, porque está deshabilitado...") | El servicio quedó en `StartType: Disabled`, normalmente por la guía de detener. Ver [Servicio deshabilitado (error 1058)](#servicio-deshabilitado-error-1058). |
| Sigue sin arrancar siendo administrador | Puede haber una política de TI bloqueándolo. Revisa los eventos con `Get-WinEvent -LogName System -MaxEvents 20 \| Where-Object Message -match usbipd`. |

### Servicio deshabilitado (error 1058)

Si `Start-Service usbipd` falla de forma genérica ("No se puede iniciar el servicio usbipd en el equipo '.'") y `sc.exe start usbipd` devuelve:

```
[SC] StartService ERROR 1058:
No se puede iniciar el servicio, porque está deshabilitado o porque no tiene dispositivos habilitados asociados a él.
```

el servicio está **deshabilitado**. Es lo que deja la guía de detener ([detener-usbipd.md](detener-usbipd.md), nivel 2).

**Confirmarlo**

```powershell
Get-Service usbipd | Select-Object Name, Status, StartType
```

Si `StartType` es `Disabled`, es esta causa.

**Arreglarlo** (PowerShell como administrador)

```powershell
Set-Service usbipd -StartupType Automatic
Start-Service usbipd
Get-Service usbipd | Select-Object Name, Status, StartType
```

Debe mostrar `Running` y `Automatic`.

Alternativa con `sc.exe` (el espacio después de `start=` es obligatorio):

```powershell
sc.exe config usbipd start= auto
sc.exe start usbipd
```

### Aviso del historial de PowerShell

Mensaje:

```
Error al leer o escribir el archivo de historial ".../ConsoleHost_history.txt": El proceso no puede obtener acceso al archivo ... porque está siendo utilizado en otro proceso.
```

Es inofensivo y no afecta a `usbipd`. Ocurre porque hay otra ventana de PowerShell abierta usando el mismo archivo de historial. Cierra las otras ventanas o ignóralo.

---

## Seguridad

- Comparte únicamente el CoreS3 y no uses `--force`.
- Al terminar, haz `detach` y `unbind` (ver [detener-usbipd.md](detener-usbipd.md)).
- En un equipo corporativo, confirma con TI que el uso esté permitido.
