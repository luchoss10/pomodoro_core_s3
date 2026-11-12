#include "ui.h"
#include "config.h"
#include "timesync.h"
#include <M5CoreS3.h>
#include <time.h>

static M5Canvas cv(&CoreS3.Display);
static int currentRotation = -1;

struct Hit { int x, y, w, h; UiButton id; };
static Hit hits[16];
static int nhits = 0;

// ---------- Rotación ----------
static int rotationFor(Orientation o) {
    int off = 0;
    if (o == Orientation::Left)  off = ROT_OFFSET_LEFT;
    if (o == Orientation::Right) off = ROT_OFFSET_RIGHT;
    return (ROT_BASE + off) & 3;
}

void uiInit() {
    currentRotation = -1;
    uiSetOrientation(Orientation::Base);
}

bool uiSetOrientation(Orientation o) {
    const int rot = rotationFor(o);
    if (rot == currentRotation) return false;
    currentRotation = rot;
    CoreS3.Display.setRotation(rot);
    cv.deleteSprite();
    cv.setColorDepth(16);
    cv.createSprite(CoreS3.Display.width(), CoreS3.Display.height());
    CoreS3.Display.fillScreen(BLACK);
    return true;
}

UiButton uiHit(int tx, int ty) {
    for (int i = nhits - 1; i >= 0; i--) {
        const Hit& b = hits[i];
        if (tx >= b.x && tx <= b.x + b.w && ty >= b.y && ty <= b.y + b.h) return b.id;
    }
    return UiButton::None;
}

// ---------- Elementos ----------
static void drawButton(int x, int y, int w, int h, const char* label, uint16_t color, UiButton id) {
    cv.fillRoundRect(x, y, w, h, 6, color);
    cv.drawRoundRect(x, y, w, h, 6, WHITE);
    cv.setTextDatum(MC_DATUM);
    cv.setTextColor(WHITE, color);
    cv.setTextSize(1);
    cv.drawString(label, x + w / 2, y + h / 2);
    if (id != UiButton::None && nhits < 16) hits[nhits++] = {x, y, w, h, id};
}

static void drawArrow(int x, int y, bool pointLeft, uint16_t col) {
    if (pointLeft) cv.fillTriangle(x, y, x + 10, y - 7, x + 10, y + 7, col);
    else           cv.fillTriangle(x + 10, y, x, y - 7, x, y + 7, col);
}

static void drawHouse(int x, int y, uint16_t col) {   // 16x14, esquina superior izquierda
    cv.fillTriangle(x + 8, y, x, y + 7, x + 16, y + 7, col);
    cv.fillRect(x + 3, y + 7, 10, 7, col);
    cv.fillRect(x + 7, y + 10, 3, 4, BLACK);
}

// Cuadro pequeño: flecha + minutos del Pomodoro que se activa al girar hacia ese lado
static void drawHintBox(int x, int y, bool arrowLeft, int minutes) {
    const int w = 56, h = 24;
    cv.drawRoundRect(x, y, w, h, 6, DARKGREY);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", minutes);
    cv.setTextDatum(MC_DATUM);
    cv.setTextColor(WHITE, BLACK);
    cv.setTextSize(1);
    if (arrowLeft) { drawArrow(x + 6, y + h / 2, true, CYAN);        cv.drawString(buf, x + w - 18, y + h / 2); }
    else           { drawArrow(x + w - 16, y + h / 2, false, CYAN);  cv.drawString(buf, x + 18, y + h / 2); }
}

// Cuadro pequeño: flecha + casa, hacia el lado en que se gira para volver
static void drawHomeBox(int x, int y, bool arrowLeft) {
    const int w = 64, h = 26;
    cv.drawRoundRect(x, y, w, h, 6, DARKGREY);
    if (arrowLeft) { drawArrow(x + 6, y + h / 2, true, CYAN);        drawHouse(x + w - 24, y + h / 2 - 7, WHITE); }
    else           { drawArrow(x + w - 16, y + h / 2, false, CYAN);  drawHouse(x + 8, y + h / 2 - 7, WHITE); }
}

static void drawNavigationBar(AppState state, const Settings& s) {
    const int w = cv.width(), h = cv.height();
    const int gap = 7, bh = 38;
    const int y = h - 45;

    if (state == AppState::POMODORO) {
        const int bw = (w - 2 * 10 - gap) / 2;
        drawButton(10, y, bw, bh, "REINICIAR", NAVY, UiButton::Restart);
        drawButton(10 + bw + gap, y, bw, bh, "DETENER", RED, UiButton::Stop);
        return;
    }

    const int bw = (w - 2 * 10 - 2 * gap) / 3;
    drawButton(10, y, bw, bh, state == AppState::CALENDAR ? "RELOJ" : "CALEND.", DARKGREY, UiButton::Calendar);
    drawButton(10 + bw + gap, y, bw, bh, state == AppState::SETTINGS ? "VOLVER" : "AJUSTES", DARKGREY, UiButton::Settings);
    if (s.imuEnabled) drawButton(10 + 2 * (bw + gap), y, bw, bh, "IMU ON", DARKGREEN, UiButton::Imu);
    else              drawButton(10 + 2 * (bw + gap), y, bw, bh, "IMU OFF", DARKGREY, UiButton::Imu);

    // Sobre el botón IMU (forma de "L" acostada): acceso al modo ahorro, solo en el reloj
    if (state == AppState::CLOCK) drawButton(10 + 2 * (bw + gap), y - 6 - 34, bw, 34, "MODO AHORRO", NAVY, UiButton::LowPower);
}

// Indicador discreto (abajo a la izquierda) de que hay un Pomodoro pausado
static void drawPausedChip(const Pomodoro& p) {
    const uint32_t rem = p.remainingSec();
    char buf[32];
    snprintf(buf, sizeof(buf), "POMO %d PAUSADO %02u:%02u", p.minutes(), (unsigned)(rem / 60), (unsigned)(rem % 60));
    cv.setTextDatum(ML_DATUM);
    cv.setTextColor(ORANGE, BLACK);
    cv.setTextSize(1);
    cv.drawString(buf, 10, 186);   // abajo a la izquierda, sobre la barra de botones
}

// Pantalla de aviso (a pantalla completa y con diseño relativo, para que
// funcione rotada): no se puede iniciar otro Pomodoro hasta terminar o reiniciar el pausado.
static void drawBlockedView(const Pomodoro& p, Orientation o) {
    const int w = cv.width(), h = cv.height();
    const int bx = 12, bw = w - 24, bh = 118, by = (h - bh) / 2 - 10;
    char buf[40];

    cv.drawRoundRect(bx, by, bw, bh, 8, RED);
    cv.setTextDatum(MC_DATUM);
    cv.setTextColor(RED, BLACK);
    cv.setTextSize(2);
    cv.drawString("NO SE PUEDE", w / 2, by + 20);
    cv.drawString("INICIAR", w / 2, by + 42);

    cv.setTextColor(WHITE, BLACK);
    cv.setTextSize(1);
    snprintf(buf, sizeof(buf), "Hay un Pomodoro de %d min", p.minutes());
    cv.drawString(buf, w / 2, by + 70);
    cv.drawString("en pausa. Terminalo o", w / 2, by + 86);
    cv.drawString("reinicialo primero.", w / 2, by + 100);

    // Indicador de volver a la base (mismo criterio que en la pantalla del Pomodoro)
    const bool arrowLeft = (o == Orientation::Left);
    drawHomeBox(arrowLeft ? 4 : w - 68, h - 34, arrowLeft);
}

// Icono de altavoz con 4 barras (el nivel 0 se ve tachado)
static void drawVolumeIcon(int cx, int y, int level) {
    const int x = cx - 18;                       // ancho total ~36 px, alto ~12 px
    cv.fillRect(x, y + 4, 4, 5, WHITE);
    cv.fillTriangle(x + 4, y + 4, x + 4, y + 8, x + 10, y + 12, WHITE);
    cv.fillTriangle(x + 4, y + 4, x + 10, y, x + 10, y + 12, WHITE);
    if (level <= 0) {
        cv.drawLine(x + 15, y + 2, x + 27, y + 10, RED);
        cv.drawLine(x + 15, y + 10, x + 27, y + 2, RED);
        return;
    }
    for (int i = 0; i < SOUND_LEVELS - 1; i++) {
        const int bh = 3 + 3 * i;
        const uint16_t col = (i < level) ? CYAN : DARKGREY;
        cv.fillRect(x + 15 + i * 5, y + 12 - bh, 3, bh, col);
    }
}

// ---- Batería ----
static int  batteryPct = -1;
static bool batteryCharging = false;
static uint32_t lastBatteryRead = 0;

static void readBattery() {
    if (batteryPct >= 0 && millis() - lastBatteryRead < BATTERY_READ_MS) return;
    lastBatteryRead = millis();
    batteryPct = M5.Power.getBatteryLevel();
    batteryCharging = (M5.Power.isCharging() == m5::Power_Class::is_charging);
}

// Icono de batería 24x12 + porcentaje (ancho total ~55 px). x,y = esquina superior izquierda.
static void drawBattery(int x, int y) {
    readBattery();
    const bool known = batteryPct >= 0 && batteryPct <= 100;
    const uint16_t col = !known ? DARKGREY : (batteryPct >= 40 ? GREEN : (batteryPct >= 20 ? ORANGE : RED));
    cv.drawRect(x, y, 24, 12, WHITE);
    cv.fillRect(x + 24, y + 3, 3, 6, WHITE);
    if (known) cv.fillRect(x + 2, y + 2, (20 * batteryPct) / 100, 8, col);
    if (batteryCharging) {   // rayo
        cv.drawLine(x + 13, y + 1, x + 8, y + 7, YELLOW);
        cv.drawLine(x + 8, y + 7, x + 15, y + 7, YELLOW);
        cv.drawLine(x + 15, y + 7, x + 10, y + 11, YELLOW);
    }
    char buf[8];
    if (known) snprintf(buf, sizeof(buf), "%d%%", batteryPct);
    else       snprintf(buf, sizeof(buf), "--");
    cv.setTextDatum(ML_DATUM);
    cv.setTextColor(WHITE, BLACK);
    cv.setTextSize(1);
    cv.drawString(buf, x + 31, y + 6);
}

// Barra superior de indicadores (volumen + batería), centrada en cy
static void drawIndicators(int cy, const Settings& s) {
    const int total = 36 + 14 + 55;
    const int x0 = cv.width() / 2 - total / 2;
    drawVolumeIcon(x0 + 18, cy - 6, s.volumeLevel);
    drawBattery(x0 + 36 + 14, cy - 6);
}

// ---------- Vistas ----------
static void drawHints(const Settings& s) {
    if (!s.imuEnabled) return;
    const int w = cv.width();
    // Giro físico a la izquierda activa el Pomodoro largo; a la derecha, el corto.
    drawHintBox(4, 4, true, s.longMin);
    drawHintBox(w - 60, 4, false, s.shortMin);
}

static void drawClockView(const Settings& s) {
    auto dt = CoreS3.Rtc.getDateTime();
    char timeStr[16], dateStr[16];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", dt.time.hours, dt.time.minutes, dt.time.seconds);
    snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%04d", dt.date.date, dt.date.month, dt.date.year);

    cv.setTextDatum(MC_DATUM);
    cv.setTextColor(CYAN, BLACK);
    cv.setTextSize(3);
    cv.drawString(timeStr, 160, 85);
    cv.setTextColor(LIGHTGREY, BLACK);
    cv.setTextSize(2);
    cv.drawString(dateStr, 160, 135);
    // Estado de la sincronización de hora (útil para diagnosticar el Wi-Fi)
    cv.setTextSize(1);
    cv.setTextColor(DARKGREY, BLACK);
    cv.drawString(timeSyncStatus(), 160, 149);
    drawHints(s);
}

static int getFirstDayOfMonth(int year, int month) {   // 0=Dom ... 6=Sáb
    struct tm t = {0};
    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = 1;
    mktime(&t);
    return t.tm_wday;
}

static int getDaysInMonth(int year, int month) {
    static const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) return 29;
    return d[month - 1];
}

static void drawCalendarView(const Settings& s) {
    auto dt = CoreS3.Rtc.getDateTime();
    char header[24];
    snprintf(header, sizeof(header), "MES: %02d / %04d", dt.date.month, dt.date.year);
    cv.setTextDatum(MC_DATUM);
    cv.setTextColor(YELLOW, BLACK);
    cv.setTextSize(2);
    cv.drawString(header, 160, 37);

    static const char* days[] = {"D", "L", "M", "X", "J", "V", "S"};
    cv.setTextSize(1);
    cv.setTextColor(ORANGE, BLACK);
    for (int i = 0; i < 7; i++) cv.drawString(days[i], 35 + i * 41, 52);

    const int startCol  = getFirstDayOfMonth(dt.date.year, dt.date.month);
    const int totalDays = getDaysInMonth(dt.date.year, dt.date.month);
    int day = 1;
    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 7; col++) {
            if (row == 0 && col < startCol) continue;
            if (day > totalDays) break;
            const int x = 35 + col * 41, y = 74 + row * 20;
            if (day == dt.date.date) {
                cv.fillCircle(x, y, 8, BLUE);
                cv.setTextColor(WHITE, BLUE);
            } else {
                cv.setTextColor(WHITE, BLACK);
            }
            cv.drawNumber(day, x, y);
            day++;
        }
    }
    drawHints(s);
}

static void drawSettingsRow(const char* label, int row, const char* value,
                            UiButton minus, UiButton plus) {
    const int btnY = 38 + row * 40;
    cv.setTextDatum(MC_DATUM);
    cv.setTextColor(WHITE, BLACK);
    cv.setTextSize(1);
    cv.drawString(label, 160, btnY - 8);
    cv.setTextSize(2);
    cv.drawString(value, 160, btnY + 14);
    drawButton(60, btnY, 50, 28, "-", NAVY, minus);
    drawButton(210, btnY, 50, 28, "+", NAVY, plus);
}

static void drawSettingsView(const Settings& s) {
    cv.setTextDatum(MC_DATUM);
    cv.setTextColor(WHITE, BLACK);
    cv.setTextSize(2);
    cv.drawString("AJUSTES", 160, 12);

    char buf[8];
    // Giro físico a la izquierda = Pomodoro largo; a la derecha = corto.
    snprintf(buf, sizeof(buf), "%d", s.longMin);
    drawSettingsRow("IZQ (MIN)", 0, buf, UiButton::LongMinus, UiButton::LongPlus);
    snprintf(buf, sizeof(buf), "%d", s.shortMin);
    drawSettingsRow("DER (MIN)", 1, buf, UiButton::ShortMinus, UiButton::ShortPlus);
    if (s.volumeLevel == 0) snprintf(buf, sizeof(buf), "OFF");
    else                    snprintf(buf, sizeof(buf), "%d/%d", s.volumeLevel, SOUND_LEVELS - 1);
    drawSettingsRow("SONIDO", 2, buf, UiButton::VolMinus, UiButton::VolPlus);
    snprintf(buf, sizeof(buf), "%d%%", BRIGHTNESS_VALUES[s.brightnessLevel] * 100 / 255);
    drawSettingsRow("BRILLO", 3, buf, UiButton::BrightMinus, UiButton::BrightPlus);
}

// Modo ahorro: solo HH:MM grande (sin segundos), se redibuja una vez por minuto
static void drawLowPowerView() {
    const int w = cv.width(), h = cv.height();
    auto dt = CoreS3.Rtc.getDateTime();
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", dt.time.hours, dt.time.minutes);
    drawBattery(w / 2 - 27, 10);   // arriba al centro; se actualiza con el redibujo por minuto
    cv.setTextDatum(MC_DATUM);
    cv.setTextColor(WHITE, BLACK);
    cv.setFont(&fonts::Font8);
    cv.setTextSize(1);
    cv.drawString(buf, w / 2, h / 2 - 10);
    cv.setFont(&fonts::Font0);   // restaurar la fuente por defecto para el resto de vistas
    drawButton(w - 70, h - 34, 62, 26, "SALIR", DARKGREY, UiButton::ExitLowPower);
}

// Diseño relativo al tamaño: sirve en horizontal (320x240) y vertical (240x320)
static void drawPomodoroView(const Pomodoro& p, Orientation o) {
    const int w = cv.width(), h = cv.height();
    const uint32_t rem = p.remainingSec();
    char buf[16];

    cv.setTextDatum(MC_DATUM);
    cv.setTextColor((p.state() == PomoState::Finished) ? GREEN : GREENYELLOW, BLACK);
    cv.setTextSize(2);
    cv.drawString((p.state() == PomoState::Finished) ? "TIEMPO CUMPLIDO" : "SESION EN CURSO", w / 2, h * 19 / 100);

    snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)(rem / 60), (unsigned)(rem % 60));
    cv.setTextColor(WHITE, BLACK);
    cv.setTextSize(4);
    cv.drawString(buf, w / 2, h * 42 / 100);

    // Barra de progreso (se vacía con el tiempo)
    const int bx = 30, bw = w - 60, by = h * 62 / 100, bh = 16;
    const int fill = (int)((bw - 4) * p.remainingRatio());
    cv.drawRect(bx, by, bw, bh, WHITE);
    cv.fillRect(bx + 2, by + 2, fill, bh - 4, GREEN);

    // Indicador de volver a inicio: se gira en sentido contrario al que activó el Pomodoro
    const bool arrowLeft = (o == Orientation::Left);
    drawHomeBox(arrowLeft ? 4 : w - 68, h - 45 - 34, arrowLeft);
}

void uiDraw(AppState state, const Settings& s, const Pomodoro& p, Orientation o, bool blocked) {
    nhits = 0;
    cv.fillSprite(BLACK);

    if (state == AppState::LOW_POWER) {
        drawLowPowerView();
        cv.pushSprite(0, 0);
        return;
    }

    if (blocked) {
        drawBlockedView(p, o);
        cv.pushSprite(0, 0);
        return;
    }

    switch (state) {
        case AppState::CLOCK:    drawClockView(s);       break;
        case AppState::CALENDAR: drawCalendarView(s);    break;
        case AppState::SETTINGS: drawSettingsView(s);    break;
        case AppState::POMODORO: drawPomodoroView(p, o); break;
        default: break;
    }
    if (state != AppState::POMODORO && state != AppState::SETTINGS && p.state() == PomoState::Paused) drawPausedChip(p);
    if (state == AppState::CLOCK || state == AppState::CALENDAR || state == AppState::POMODORO) drawIndicators(16, s);
    drawNavigationBar(state, s);
    cv.pushSprite(0, 0);
}
