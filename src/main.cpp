#include <M5CoreS3.h>

void setup() {
    auto cfg = M5.config();
    CoreS3.begin(cfg);
    Serial.begin(115200);

    CoreS3.Display.setRotation(1);
    CoreS3.Display.setTextSize(2);
    CoreS3.Display.setTextColor(WHITE, BLACK);
    CoreS3.Display.fillScreen(BLACK);
    
    CoreS3.Display.setCursor(10, 10);
    CoreS3.Display.println("Calibracion IMU");
}

void loop() {
    CoreS3.update();

    float ax, ay, az;
    CoreS3.Imu.getAccel(&ax, &ay, &az);
    Serial.printf("ax=%.2f ay=%.2f az=%.2f\n", ax, ay, az);

    // Posicionamiento individual para mantener el margen X=10
    CoreS3.Display.setCursor(10, 50);
    CoreS3.Display.printf("X: %5.2f  ", ax);
    
    CoreS3.Display.setCursor(10, 75);
    CoreS3.Display.printf("Y: %5.2f  ", ay);
    
    CoreS3.Display.setCursor(10, 100);
    CoreS3.Display.printf("Z: %5.2f  ", az);

    // Indicador visual de orientacion detectada
    CoreS3.Display.setCursor(10, 145);
    if (ax < -0.75f) {
        CoreS3.Display.setTextColor(CYAN, BLACK);
        CoreS3.Display.print("MODO: 15 MIN (IZQ)  ");
    } else if (ax > 0.75f) {
        CoreS3.Display.setTextColor(GREENYELLOW, BLACK);
        CoreS3.Display.print("MODO: 30 MIN (DER)  ");
    } else {
        CoreS3.Display.setTextColor(LIGHTGREY, BLACK);
        CoreS3.Display.print("MODO: RELOJ NORMAL  ");
    }
    CoreS3.Display.setTextColor(WHITE, BLACK);

    delay(150);
}
