#include <Arduino.h>
#include <math.h>

#include "loadsave.cpp"
#include "pinouts.hpp"

void WaitForEncoderPress() {
    while (digitalRead(ENCODER_SW) == LOW) {
        delay(100); // Wait for the encoder button to be released
    }
    while (digitalRead(ENCODER_SW) == HIGH) {
        delay(100); // Wait for the encoder button to be pressed
    }
}

void CalibrateADC(Adafruit_SSD1306 &display, float offsetScale[2][2]) {
    display.clearDisplay();

    int calibrationValues[2][2]; // [channel][0: 0V, 1: 5V]
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Calibrating inputs...");
    display.display();

    display.setCursor(0, 10);
    display.print("Ch. 1");
    display.print(" - 0V: ");
    display.display();

    WaitForEncoderPress(); // Wait for user to press the encoder
    calibrationValues[0][0] = analogRead(CV_1_IN_PIN);
    display.setCursor(80, 10);
    display.print(calibrationValues[0][0]);
    display.display();

    display.setCursor(0, 19);
    display.print("Ch. 2");
    display.print(" - 0V: ");
    display.display();

    WaitForEncoderPress(); // Wait for user to press the encoder

    calibrationValues[1][0] = analogRead(CV_2_IN_PIN);
    display.setCursor(80, 19);
    display.print(calibrationValues[1][0]);
    display.display();

    display.setCursor(0, 27);
    display.print("Ch. 1");
    display.print(" - 5V: ");
    display.display();

    WaitForEncoderPress(); // Wait for user to press the encoder

    calibrationValues[0][1] = analogRead(CV_1_IN_PIN);
    display.setCursor(80, 27);
    display.print(calibrationValues[0][1]);
    display.display();

    display.setCursor(0, 36);
    display.print("Ch. 2");
    display.print(" - 5V: ");
    display.display();

    WaitForEncoderPress(); // Wait for user to press the encoder

    calibrationValues[1][1] = analogRead(CV_2_IN_PIN);
    display.setCursor(80, 36);
    display.print(calibrationValues[1][1]);

    display.setCursor(0, 50);
    display.print("Press to continue...");
    display.display();
    WaitForEncoderPress(); // Wait for user to press the encoder

    for (int ch = 0; ch < 2; ch++) {
        float offset = calibrationValues[ch][0];
        float scale = 5.0 / (calibrationValues[ch][1] - calibrationValues[ch][0]);
        offsetScale[ch][0] = offset;
        offsetScale[ch][1] = scale;
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Calibration Done");
    // Print the calibration values
    display.setCursor(0, 10);
    display.print("Ch1 - Offset: ");
    display.print(offsetScale[0][0]);
    display.setCursor(0, 20);
    display.print("Ch1 - Scale: ");
    display.print(offsetScale[0][1], 6);
    display.setCursor(0, 30);
    display.print("Ch2 - Offset: ");
    display.print(offsetScale[1][0]);
    display.setCursor(0, 40);
    display.print("Ch2 - Scale: ");
    display.print(offsetScale[1][1], 6);
    display.setCursor(0, 50);
    display.print("Press encoder to save");
    display.display();

    // Print the calibration values to the serial monitor
    Serial.println("Saved calibration values:");
    Serial.print("Ch1 - Offset: ");
    Serial.println(offsetScale[0][0]);
    Serial.print("Ch1 - Scale: ");
    Serial.println(offsetScale[0][1], 6);
    Serial.print("Ch2 - Offset: ");
    Serial.println(offsetScale[1][0]);
    Serial.print("Ch2 - Scale: ");
    Serial.println(offsetScale[1][1], 6);
    WaitForEncoderPress(); // Wait for user to press the encoder
    delay(1000);

    SaveCalibration(offsetScale);
}
