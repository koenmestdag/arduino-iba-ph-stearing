/**
 * Control the acidity from your IBA (Indivuele Behandeling van Afvalwater) by addding NaOH
 * 
 * Hardware:
 *  - Peristaltic pump 12V DC DIY Liquid Dosing Pump for Aquarium Lab Analytical
 *        https://www.aliexpress.com/item/32770860268.html
 *  - pH measurement with Liquid PH 0-14 Value Detection Regulator Sensor Module Monitoring Control Meter Tester + BNC PH Electrode Probe For Arduino
 *        https://www.aliexpress.com/item/32964738891.html
 *  - 4-port lcd
 *  - Relais
 *  - 12V power supply
 *  - Pushbutton & 10K ohm resistor
 * 
 * Pump
 * ----
 * The pump functions with with Arduino 5V power supply (directly connect one pole to an digital outpin, the other to GND), but it hasn't enough power to run smoothly.
 * The pump runs better with 12V. Attach a 12V power supply to a relais, stear the relais with the Arduino.
 * 
 * pH meter
 * --------
 * Code for the pH meter is based on: https://scidle.com/how-to-use-a-ph-sensor-with-arduino/
 * 
 * Before starting up, always first calibrate the pH with buffer fluids. Measure the voltage at p0, using the Arduino int value.
 *          y = ax + b // y = ph, x = voltage, a en b are to be determined
 *          
 *          4,01 = a * 4,40 + b // first buffering solution has pH 4,01, f.e. measured voltage is pHsensor A: 4,40 pHsensor B: 4,35
 *          6,86 = a * 3,82 + b // second buffering solution has pH 6,86, f.e. measured voltage is for pHsensor A: 3,82 pHsensor B: 3,55
            
 *          b = 4,01 - a * 4,40
 *          6,86 = a * 3,82 + (4,01 - a * 4,40) // substitute value of b of the first equation, into the second in order to calculate a
 *          6,86 = a * 3,82 + 4,01 - a * 4,40
 *          6,86 = 3,82a + 4,01 - 4,40a
 *          6,86 - 4,01 = a (3,82 - 4,40)
 *          2,85 = - 0,58 * a
 *          a = -4,91
 *          b = 4,01 - (-4,91 * 4,40) = 25,61 // fill in the value of a into the first equation in order to calculate b
 *          
 *          => 6,86 = -4,91 * 3,82 + 25,61 =  // dubbelcheck
 *          
 *          (You can use arduino_ph_berekening.xls to compute the values, or use the built-in calibration mode ;-) )
 * 
 * To connect with Arduino we will need an analog input (A0), power (5V) and two GND that actually in the sensor circuit are separated but we can use the same.
 * 
 * Pinout
 *    To  Temperature     > not used
 *    Do  Limit pH Signal > not used
 *    Po  Analog pH value > A0
 *    G   Analog GND      > GND
 *    G   Supply GND      > GND
 *    V+  Supply (5V)     > 5V
 * 
 * Lcd
 * ---
 *  - Vcc > +5V
 *  - Grnd > ground
 *  - SCL > A5
 *  - SDA > A4
 *  
 *  Button
 *  ------
 *  - Pushbutton from +5V attached to pin 3 and via 10K ohm resistor to ground (https://www.arduino.cc/en/tutorial/button)
 */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

enum eProgramModes {
  TEST_PUMP = 0,  // program is running in test: pH values are random, without pumping
  TEST_PH = 1,    // program is running in test: pH values measured, without pumping
  RUN = 2,        // program is running!
  CALIBRATE = 3   // calibration mode: use buffers to calibrate the pH sensor
};

enum eProgramModes programMode = RUN;
unsigned long startTime = millis();

// Pump
int pumpPin = 2;                  // the pin we will use as output
float alarmMin = 6.4;             // sound alarm when pH level is lower
float alarmMax = 7.5;             // sound alarm when pH level is higher
float lowPh = 6.5;                // start adding pH+ (NaOH 1M)
int pumpTime = 2000;              // the amount of time tot pump NaOH into the barrel, whithout measuring
int measurementDelayTime = 5000;  // the amount of time to wait after pumping in order to stabilize pH
int timesPumped = 0;

// pH
const int analogInPin = A0; 
int sensorValue = 0; 
unsigned long int avgValue; 
float a = -7.2; // -4.91;        //@see "calibration"
float b = 33.16; // 25.61;        //@see "calibration"
int buf[10], temp;
const float PH686 = 6.88;
const float PH401 = 4.00;

// button
const int buttonModusPin = 3;     // we use the button to change modus TEST > RUN > RUN VERBOSE / CALIBRATE
int buttonState;

/**
 * Initialize
 */
void setup() {
  Serial.begin(57600);

  // config lcd
  lcd.begin();  // Turn on the blacklight
  lcd.setBacklight((uint8_t)1);
  lcd.println(0, "pH program");
  lcd.println(1, "2020");
  
  pinMode(pumpPin, OUTPUT);   // initiate pump pin as output
  digitalWrite(pumpPin, LOW); // stop pump

  // initialize the pushbutton pin as an input:
  pinMode(buttonModusPin, INPUT);

  delay(1000);
}

/**
 * Run program
 */
void loop() {
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonModusPin);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    Serial.println("BUTTON PUSHED!");
    String sModus;
    if (programMode < CALIBRATE) {
      programMode = eProgramModes((int)programMode + 1); // increment mode
    } else {
      programMode = RUN; //return to the run programMode, ingnore test programs
    }
    switch (programMode) {
      case TEST_PUMP:
        sModus = "TESTING PUMP";
        break;
      case TEST_PH:
        sModus = "TESTING PH";
        break;
      case RUN:
        sModus = "NORMAL OPERATION";
        break;
      case CALIBRATE:
        sModus = "CALIBRATION MODE";
        break;
      default:
        sModus = "** ERROR MODE **";
        break;
      printLcd(0, "New mode");
      printLcd(1, sModus);
      delay(2000);
    }
  } else {
      Serial.println("No button pushed");
      switch (programMode) {
        case TEST_PH:
          Serial.println("TEST_PH");
          runTestPump();
          break;
        case TEST_PUMP:
          Serial.println("TEST_PUMP");
          runTestPump();
          break;
        case RUN:
          Serial.println("RUN");
          runMeasurement();
          break;
        case CALIBRATE:
          Serial.println("CALIBRATE");
          runCalibration();
          programMode = RUN; // Do not keep calibrating: change back to normal mode
          break;
        default:
          printLcd(1, "ERROR MODE :-(");
          delay(1000);
          break;
    }
  }
}

void runTestPump() {
  float phValue = getTestpH();
  printLcd(1, "Random pH=" + String(phValue));
  doseIfNeeded(phValue);
}

void runTestPH() {
  float phValue = getpH(getSensorVoltage());
  printLcd(1, "pH=" + String(phValue));    
  // No dosing!
}

void runMeasurement() {
  float pMeasuredVoltage = getSensorVoltage();
  float phValue = getpH(pMeasuredVoltage);
  printLcd(0, "pH=" + String(phValue) + " [" + String(pMeasuredVoltage) + "V]");
  doseIfNeeded(phValue);  
}

void doseIfNeeded(float phValue) {
  // when pH lower then 6.7: add NaOH 1 M
  if(phValue < 6.7) {
    // DOSE!
    printLcd(0, "pH " + String(phValue));
    printLcd(1, "Dose " + String(timesPumped++) + "/" + String(startTime / 60000) + "'");
    runPump(pumpTime);
    printLcd(1, "delaying " + String(measurementDelayTime) + "...");
    delay(measurementDelayTime); // wait for the NaOH to take affect
    printLcd(1, "done");
  } else {
    delay(1000);  // pH is stable! :)
  }
}

void runCalibration() {
  float pHVoltage401;
  float pHVoltage686;
  float pA;
  float pB;
  
  printLcd(0, "CALIBRATION MODE");
  displayPauzeMessage(1, "PRESS 2 CANCEL ", 9);
  
  if(digitalRead(buttonModusPin) == HIGH){
    programMode = RUN; //return to RUN
    printLcd(1, "CAL. CANCELLED");
    delay(1000);
  } else {

    displayPauzeMessage(0, "CAL. IN ", 10);
    printLcd(1, "");
  
    printLcd(0, "**CALIBRATING**");
    printLcd(1, "");
    delay(1000);
  
    // calibrate 4,01
    printLcd(0, "PUT pH SENSOR IN");
    displayPauzeMessage(1, "BUFFER 4,01 ", 20);
    pHVoltage401 = getSensorVoltage();
    printLcd(0, "4,01 FINISHED");
    printLcd(1, "VOLTAGE " + String(pHVoltage401));
    delay(1000);
  
    // calibrate 6,86
    printLcd(0, "PUT pH SENSOR IN");
    displayPauzeMessage(1, "BUFFER 6,86 ", 20);
    pHVoltage686 = getSensorVoltage();
    printLcd(0, "6,86 FINISHED");
    printLcd(1, "VOLTAGE " + String(pHVoltage686));
    delay(1000);
  
    pA = (PH686 - PH401) / (pHVoltage686 - pHVoltage401);  // use the pH equation
    pB = PH401 - (pA * pHVoltage401);
  
    if (pA < 3 && pA > -10 && pB > 10 && pB < 40) {
      a = pA;
      b = pB;
      printLcd(0, "CAL. ENDED");
      printLcd(1, "a=" + String(a) + " b=" + String(b));
    } else {
      printLcd(0, "CAL. ERROR!!!");
      printLcd(1, "a=" + String(pA) + " b=" + String(pB));
    }
  
    delay(5000);
  }
  
}

/* Test program */
float getTestpH() {
  long randNumber;
  randNumber = random(0, 1400);
  return (randNumber / 100);
}

/**
 * pRunTime : the time to pump
 */
void runPump(int pRunTime) {
  float pPumpStartTime = millis();
  digitalWrite(pumpPin, HIGH);
  Serial.println("pumping started");
  while(millis() < pPumpStartTime + pRunTime) {
    // keep on dosing
  }
  digitalWrite(pumpPin, LOW);
  Serial.println("ended pumping at " + String(pRunTime));
}

/**
 * Gets pH result from pH sensor 
 * 
 * We get the voltage of the sensor, then using the equation that we have calculated with the pH reference values, we convert pHVol
 * to pHValue.
 * 
 * @param The measured voltage to return the pH for
 */
float getpH (float pSensorVoltage) {
  float pHVol = pSensorVoltage;
  float phValue = a * pHVol + b;
  return phValue;
}

/**
 * Returns the voltage measured by the pH sensor
 * The code consists of taking 10 samples of the analogue input A0, ordering them and discarding the highest and 
 * the lowest and calculating the mean with the six remaining samples by converting this value to voltage in the 
 * variable pHVol.
 * 
 * @return measuredVoltage
 */
float getSensorVoltage() {
    // read 10 times info buf[]
  for(int i = 0; i < 10; i++) { 
     buf[i] = analogRead(analogInPin);
     delay(10);
  }

  // put lowest values first in buf[]
  for(int i = 0; i < 9; i++) {
     for(int j = i + 1; j < 10; j++) {
        if(buf[i] > buf[j]) {
           temp = buf[i];
           buf[i] = buf[j];
           buf[j] = temp;
        }
     }
  }

  // calculate average, starting with third value and also discarting last 2
  avgValue = 0;
  for(int i = 2; i < 8; i++) avgValue += buf[i];
  float pHVol = (float)avgValue / 6 * 5.0 / 1024; // 6 measurements: convert measured int to voltage (0-1024 range to 0-5V range), use this value also to calibrate your pH sensor

  return pHVol;
}

/* Easy way to communicate with lcd */
void printLcd(int row, String value) {
  Serial.println("printLcd " + String(row) + " [" + value + "]");
  lcd.setCursor(0, row);
  lcd.print(appendSpaces(value, 16));
}

/* Returns string of length len, by appending spaces to the right */
String appendSpaces(String text, int len) {
  String iText = text;
  if(iText.length() > len) {
    iText = iText.substring(1, 17);
  } else {
    while (iText.length() < len) {
      iText.concat(" ");
    };
  }
  return iText;
}

void displayPauzeMessage(int row, String text, int delaySeconds) {
  for(int i = delaySeconds; i > 0; i--) {
    printLcd(row, text + String(i));
    delay(1000);
  }
}
