#include <M5CoreS3.h>
#include <time.h>

// Estados de la aplicación
enum class AppState {
    CLOCK,
    CALENDAR,
    POMODORO,
    SETTINGS
};

AppState currentState = AppState::CLOCK;
bool needsRedraw = true;

// Variables del Pomodoro
int shortWorkMin = 15;
int longWorkMin = 30;
int activeDurationMin = 15;
unsigned long pomodoroStartTime = 0;
bool pomodoroRunning = false;
bool pomodoroDisabledByUser = false;

// Variables de control de tiempo
unsigned long lastSecondTick = 0;

// Estructura para botones táctiles
struct TouchButton {
    int x, y, w, h;
    const char* label;
    uint16_t color;
};

// Botones de navegación inferior (320x240)
TouchButton btnBottomLeft  = {10,  195, 95, 38, "CALEND.", DARKGREY};
TouchButton btnBottomMid   = {112, 195, 95, 38, "AJUSTES", DARKGREY};
TouchButton btnBottomRight = {215, 195, 95, 38, "IMU ON",  DARKGREEN};

// Botones de Ajustes
TouchButton btnShortMinus = {60,  70,  50, 40, "-", NAVY};
TouchButton btnShortPlus  = {210, 70,  50, 40, "+", NAVY};
TouchButton btnLongMinus  = {60,  130, 50, 40, "-", NAVY};
TouchButton btnLongPlus   = {210, 130, 50, 40, "+", NAVY};

bool isTouched(TouchButton btn, int tx, int ty) {
    return (tx >= btn.x && tx <= (btn.x + btn.w) &&
            ty >= btn.y && ty <= (btn.y + btn.h));
}

void drawButton(TouchButton btn) {
    CoreS3.Display.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 6, btn.color);
    CoreS3.Display.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 6, WHITE);
    CoreS3.Display.setTextDatum(MC_DATUM);
    CoreS3.Display.setTextColor(WHITE, btn.color);
    CoreS3.Display.setTextSize(1);
    CoreS3.Display.drawString(btn.label, btn.x + btn.w / 2, btn.y + btn.h / 2);
}

void startPomodoro(int minutes) {
    activeDurationMin = minutes;
    pomodoroStartTime = millis();
    pomodoroRunning = true;
    currentState = AppState::POMODORO;
    needsRedraw = true;
}

void stopPomodoro() {
    pomodoroRunning = false;
    currentState = AppState::CLOCK;
    needsRedraw = true;
}

void checkOrientationIMU() {
    if (pomodoroDisabledByUser || currentState == AppState::SETTINGS) return;

    float ax, ay, az;
    CoreS3.Imu.getAccel(&ax, &ay, &az);

    if (ax < -0.75f && !pomodoroRunning) {
        startPomodoro(shortWorkMin);
    } else if (ax > 0.75f && !pomodoroRunning) {
        startPomodoro(longWorkMin);
    } else if (abs(ax) < 0.35f && pomodoroRunning) {
        stopPomodoro();
    }
}

void drawClockView() {
    auto dt = CoreS3.Rtc.getDateTime();
    char timeStr[16];
    char dateStr[16];

    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", dt.time.hours, dt.time.minutes, dt.time.seconds);
    snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%04d", dt.date.date, dt.date.month, dt.date.year);

    CoreS3.Display.setTextDatum(MC_DATUM);
    CoreS3.Display.setTextColor(CYAN, BLACK);
    CoreS3.Display.setTextSize(3);
    CoreS3.Display.drawString(timeStr, 160, 85);

    CoreS3.Display.setTextColor(LIGHTGREY, BLACK);
    CoreS3.Display.setTextSize(2);
    CoreS3.Display.drawString(dateStr, 160, 135);
}

// Calcula el día de la semana (0=Dom ... 6=Sáb) para el día 1 del mes
int getFirstDayOfMonth(int year, int month) {
    struct tm time_in = {0};
    time_in.tm_year = year - 1900;
    time_in.tm_mon = month - 1;
    time_in.tm_mday = 1;
    mktime(&time_in);
    return time_in.tm_wday;
}

int getDaysInMonth(int year, int month) {
    const int daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        return 29;
    }
    return daysPerMonth[month - 1];
}

void drawCalendarView() {
    auto dt = CoreS3.Rtc.getDateTime();
    CoreS3.Display.setTextDatum(MC_DATUM);
    CoreS3.Display.setTextColor(YELLOW, BLACK);
    CoreS3.Display.setTextSize(2);

    char header[24];
    snprintf(header, sizeof(header), "MES: %02d / %04d", dt.date.month, dt.date.year);
    CoreS3.Display.drawString(header, 160, 22);

    const char* days[] = {"D", "L", "M", "X", "J", "V", "S"};
    CoreS3.Display.setTextSize(1);
    for (int i = 0; i < 7; i++) {
        CoreS3.Display.setTextColor(ORANGE, BLACK);
        CoreS3.Display.drawString(days[i], 35 + i * 41, 48);
    }

    int startCol = getFirstDayOfMonth(dt.date.year, dt.date.month);
    int totalDays = getDaysInMonth(dt.date.year, dt.date.month);
    int dayCounter = 1;

    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 7; col++) {
            if (row == 0 && col < startCol) continue;
            if (dayCounter > totalDays) break;

            int x = 35 + col * 41;
            int y = 70 + row * 20;

            if (dayCounter == dt.date.date) {
                CoreS3.Display.fillCircle(x, y, 8, BLUE);
                CoreS3.Display.setTextColor(WHITE, BLUE);
            } else {
                CoreS3.Display.setTextColor(WHITE, BLACK);
            }
            CoreS3.Display.drawNumber(dayCounter, x, y);
            dayCounter++;
        }
    }
}

void drawPomodoroView() {
    unsigned long elapsedSec = (millis() - pomodoroStartTime) / 1000;
    long totalSec = activeDurationMin * 60L;
    long remainingSec = totalSec - elapsedSec;

    if (remainingSec <= 0) {
        remainingSec = 0;
    }

    int mins = remainingSec / 60;
    int secs = remainingSec % 60;

    char pomoStr[10];
    snprintf(pomoStr, sizeof(pomoStr), "%02d:%02d", mins, secs);

    CoreS3.Display.setTextDatum(MC_DATUM);
    CoreS3.Display.setTextColor(GREENYELLOW, BLACK);
    CoreS3.Display.setTextSize(2);
    CoreS3.Display.drawString("SESION EN CURSO", 160, 45);

    CoreS3.Display.setTextColor(WHITE, BLACK);
    CoreS3.Display.setTextSize(4);
    CoreS3.Display.drawString(pomoStr, 160, 105);

    int barWidth = map(remainingSec, 0, totalSec, 0, 256);
    CoreS3.Display.drawRect(30, 150, 260, 16, WHITE);
    CoreS3.Display.fillRect(32, 152, barWidth, 12, GREEN);
    CoreS3.Display.fillRect(32 + barWidth, 152, 256 - barWidth, 12, BLACK);
}

void drawSettingsView() {
    CoreS3.Display.setTextDatum(MC_DATUM);
    CoreS3.Display.setTextColor(WHITE, BLACK);
    CoreS3.Display.setTextSize(2);
    CoreS3.Display.drawString("AJUSTAR TIEMPOS", 160, 25);

    CoreS3.Display.setTextSize(1);
    CoreS3.Display.drawString("IZQ (MIN)", 160, 58);
    CoreS3.Display.setTextSize(2);
    CoreS3.Display.drawNumber(shortWorkMin, 160, 90);
    drawButton(btnShortMinus);
    drawButton(btnShortPlus);

    CoreS3.Display.setTextSize(1);
    CoreS3.Display.drawString("DER (MIN)", 160, 118);
    CoreS3.Display.setTextSize(2);
    CoreS3.Display.drawNumber(longWorkMin, 160, 150);
    drawButton(btnLongMinus);
    drawButton(btnLongPlus);
}

void drawNavigationBar() {
    btnBottomLeft.label  = (currentState == AppState::CALENDAR) ? "RELOJ" : "CALEND.";
    btnBottomMid.label   = (currentState == AppState::SETTINGS) ? "VOLVER" : "AJUSTES";
    
    if (pomodoroRunning) {
        btnBottomRight.label = "SALIR POMO";
        btnBottomRight.color = RED;
    } else {
        btnBottomRight.label = pomodoroDisabledByUser ? "IMU OFF" : "IMU ON";
        btnBottomRight.color = pomodoroDisabledByUser ? DARKGREY : DARKGREEN;
    }

    drawButton(btnBottomLeft);
    drawButton(btnBottomMid);
    drawButton(btnBottomRight);
}

void handleTouch() {
    auto touch = CoreS3.Touch.getDetail();
    if (!touch.wasPressed()) return;

    int tx = touch.x;
    int ty = touch.y;

    if (isTouched(btnBottomLeft, tx, ty)) {
        if (pomodoroRunning) stopPomodoro();
        currentState = (currentState == AppState::CLOCK) ? AppState::CALENDAR : AppState::CLOCK;
        needsRedraw = true;
    } else if (isTouched(btnBottomMid, tx, ty)) {
        if (pomodoroRunning) stopPomodoro();
        currentState = (currentState == AppState::SETTINGS) ? AppState::CLOCK : AppState::SETTINGS;
        needsRedraw = true;
    } else if (isTouched(btnBottomRight, tx, ty)) {
        if (pomodoroRunning) {
            stopPomodoro();
            pomodoroDisabledByUser = true;
        } else {
            pomodoroDisabledByUser = !pomodoroDisabledByUser;
        }
        needsRedraw = true;
    }

    if (currentState == AppState::SETTINGS) {
        if (isTouched(btnShortMinus, tx, ty) && shortWorkMin > 5) {
            shortWorkMin -= 5;
            needsRedraw = true;
        } else if (isTouched(btnShortPlus, tx, ty) && shortWorkMin < 60) {
            shortWorkMin += 5;
            needsRedraw = true;
        } else if (isTouched(btnLongMinus, tx, ty) && longWorkMin > 10) {
            longWorkMin -= 5;
            needsRedraw = true;
        } else if (isTouched(btnLongPlus, tx, ty) && longWorkMin < 90) {
            longWorkMin += 5;
            needsRedraw = true;
        }
    }
}

void setup() {
    auto cfg = M5.config();
    CoreS3.begin(cfg);
    Serial.begin(115200);

    CoreS3.Display.setRotation(0);
    CoreS3.Display.fillScreen(BLACK);

    // Fecha y hora inicial: 10 de noviembre de 2026, 22:06:00
    CoreS3.Rtc.setDateTime({{2026, 11, 10}, {22, 16, 0}});
}

void loop() {
    CoreS3.update();

    handleTouch();
    checkOrientationIMU();

    if (needsRedraw) {
        CoreS3.Display.fillScreen(BLACK);
        drawNavigationBar();
        needsRedraw = false;
        lastSecondTick = 0;
    }

    if (millis() - lastSecondTick >= 1000 || lastSecondTick == 0) {
        lastSecondTick = millis();

        switch (currentState) {
            case AppState::CLOCK:
                drawClockView();
                break;
            case AppState::CALENDAR:
                drawCalendarView();
                break;
            case AppState::POMODORO:
                drawPomodoroView();
                break;
            case AppState::SETTINGS:
                drawSettingsView();
                break;
        }
    }

    delay(15);
}
