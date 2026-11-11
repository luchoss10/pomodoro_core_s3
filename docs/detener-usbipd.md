# Detener el compartidor USB entre Windows y WSL (usbipd)

Guía para dejar de compartir el CoreS3 (u otros dispositivos USB) con WSL y reducir la superficie de riesgo de `usbipd-win`.

> Todos los comandos de Windows se ejecutan en **PowerShell como administrador**
> (menú Inicio → `PowerShell` → clic derecho → *Ejecutar como administrador*).
> Si `usbipd` no se reconoce, usa la ruta completa:
> `& "C:\Program Files\usbipd-win\usbipd.exe"`

En esta guía, el CoreS3 tiene BUSID `3-5` (VID:PID `303a:1001`). Verifica el tuyo con `usbipd list`, porque puede cambiar si lo conectas a otro puerto USB.

---

## Nivel 1: Terminar la sesión de trabajo (uso diario)

Hazlo cada vez que termines de flashear o depurar.

1. **Cierra el monitor serial** en WSL (`Ctrl+C` si usas `pio device monitor`).
2. **Desconecta el dispositivo de WSL** (vuelve a verlo Windows como `COM4`):

   ```powershell
   usbipd detach --busid 3-5
   ```

3. **Deja de compartirlo** (quita el `bind`):

   ```powershell
   usbipd unbind --busid 3-5
   ```

4. **Verifica** que el estado sea `Not shared`:

   ```powershell
   usbipd list
   ```

> `detach` solo desconecta de WSL, pero el dispositivo sigue marcado como compartido.
> `unbind` elimina esa marca. Para máxima seguridad, haz ambos.

Para dejar **todo** limpio de una vez:

```powershell
usbipd detach --all
usbipd unbind --all
```

---

## Nivel 2: Detener el servicio

Con el servicio detenido, `usbipd` no escucha conexiones (puerto TCP 3240).

```powershell
Stop-Service usbipd
Set-Service usbipd -StartupType Disabled   # que no arranque con Windows
```

Verifica:

```powershell
Get-Service usbipd      # Status debe ser Stopped
```

Para volver a usarlo:

```powershell
Set-Service usbipd -StartupType Automatic
Start-Service usbipd
```

---

## Nivel 3: Desinstalar por completo

Si no lo vas a usar más:

```powershell
usbipd unbind --all
winget uninstall dorssel.usbipd-win
```

Verifica:

```powershell
Get-Service usbipd      # debe dar error: no existe
winget list usbipd      # no debe aparecer
```

Reinstalar más adelante:

```powershell
winget install --exact dorssel.usbipd-win
```

---

## Comprobaciones de seguridad

| Comprobación | Comando | Resultado esperado |
|---|---|---|
| Dispositivos compartidos | `usbipd list` | Todos en `Not shared` |
| Estado del servicio | `Get-Service usbipd` | `Stopped` (o inexistente) |
| Puerto 3240 escuchando | `Get-NetTCPConnection -LocalPort 3240 -ErrorAction SilentlyContinue` | Sin resultados |
| Regla de firewall | `Get-NetFirewallRule -DisplayName "*usbipd*"` | Revisa que solo permita WSL |

---

## Buenas prácticas

- Comparte **solo** el CoreS3. Nunca teclados, discos, cámaras o lectores de huella.
- Haz `detach` y `unbind` al terminar cada sesión.
- No uses `--force` ni abras el puerto 3240 a otras redes.
- Mantén `usbipd-win` actualizado: `winget upgrade dorssel.usbipd-win`.
- En un equipo corporativo, confirma con TI que el uso esté permitido.

---

## Resumen rápido

```powershell
usbipd detach --all
usbipd unbind --all
Stop-Service usbipd
Set-Service usbipd -StartupType Disabled
```
