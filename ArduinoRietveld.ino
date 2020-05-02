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
 *  - Pushbutton
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
 *          4,01 = a * 4,40 + b // eerste bufferoplossing is pH 4,01, gemeten voltage is 4,40
 *          6,86 = a * 3,82 + b // tweede bufferoplossing is pH 6,86, gemeten voltage is 3,82
            
 *          b = 4,01 - a * 4,40
 *          6,86 = a * 3,82 + (4,01 - a * 4,40) // substitueer waarde van b in de eerste vergelijking in de tweede vergelijking om a te berekenen
 *          6,86 = a * 3,82 + 4,01 - a * 4,40
 *          6,86 = 3,82a + 4,01 - 4,40a
 *          6,86 - 4,01 = a (3,82 - 4,40)
 *          2,85 = - 0,58 * a
 *          a = -4,91
 *          b = 4,01 - (-4,91 * 4,40) = 25,61 // vul a in de eerste vergelijking om b te berekenen
 *          
 *          => 6,86 = -4,91 * 3,82 + 25,61 =  // dubbelcheck
 *          
 *          (You can use arduino_ph_berekening.xls to compute the values)
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
 *  - Pushbutton attached to pin 3 from +5V
 */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

enum eProgramModes {
  TEST_PUMP,       // program is running in test: pH values are random
  TEST_PH,       // program is running in test: pH values are random
  RUN,        // program is running!
  CALIBRATE   // calibration mode: use buffers to calibrate the pH sensor
};

enum eProgramModes programMode = RUN;
unsigned long startTime = millis();

// Pump
int pumpPin = 2;                  // the pin we will use as output
float alarmMin = 6.4;             // sound alarm when pH level is lower
float alarmMax = 7.5;             // sound alarm when pH level is higher
float lowPh = 6.5;                // start adding pH+ (NaOH 1M)
int pumpTime = 2000;              // the amount of time tot pump NaOH into the barrel, whithout measuring
int measurementDelayTime = 5000; // the amount of time to wait after pumping in order to stabilize pH
int timesPumped = 0;

// pH
const int analogInPin = A0; 
int sensorValue = 0; 
unsigned long int avgValue; 
float a = -4.91; //@see "calibration"
float b = 25.61; //@see "calibration"
int buf[10],temp;

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
  
  pinMode(pumpPin, OUTPUT);   // initiate pump pin as output
  digitalWrite(pumpPin, LOW); // stop pump

  // initialize the pushbutton pin as an input:
  pinMode(buttonModusPin, INPUT);

  Serial.println("STARTED");
}

/**
 * Run program
 */
void loop() {
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonModusPin);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    String sModus;
    if (programMode < CALIBRATE) {
      programMode = programMode + 1;
    } else {
      programMode = TEST_PH;
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
        break;
      printLcd(0, sModus);
      printLcd(1, "");
      delay(1000);
    }
  } else {
      switch (programMode) {
        case TEST_PH:
          runTestPump();
          break;
        case TEST_PUMP:
          runTestPump();
          break;
        case RUN:
          runMeasurement();
          break;
        case CALIBRATE:
          runCalibration();
          programMode = RUN; // Do not keep calibrating: change back to normal mode
          break;
        default:
          printLcd(1, "ERROR MODE :(");
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
  float phValue = getpH();
  printLcd(1, "pH=" + String(phValue));    
  // No dosing!
}

void runMeasurement() {
  float phValue = getpH();
  printLcd(0, "pH=" + String(phValue));
  doseIfNeeded(phValue);  
}

void doseIfNeeded(float phValue) {
  // when pH lower then 6.7: add NaOH 1 M
  if(phValue < 6.7) {
    // DOSE!
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
  
  printLcd(1, "");
  displayPauzeMessage(0, "CALIBRATION IN", 20);

  printLcd(0, "**CALIBRATING**");
  printLcd(1, "");
  delay(1000);

  // calibrate 4,01
  printLcd(0, "PUT pH SENSOR IN BUFFER 4,01");
  displayPauzeMessage(1, "IN ", 20);
  pHVoltage401 = getSensorVoltage();
  printLcd(1, "VOLTAGE " + String(pHVoltage401));
  delay(1000);

  // calibrate 6,86
  printLcd(0, "PUT pH SENSOR IN BUFFER 6,86");
  displayPauzeMessage(1, "IN ", 20);
  pHVoltage686 = getSensorVoltage();
  printLcd(1, "VOLTAGE " + String(pHVoltage686));
  delay(1000);

  pA = (6,86 - 4,01) / (pHVoltage686 - pHVoltage401);  // use the pH equation
  pB = 4,01 - (pA * pHVoltage401);

  if(pA < 3 && pA > -7 && pB > 20 && pB < 30) {
    a = pA;
    b = pB;
    printLcd(0, "CAL. ENDED");
    printLcd(1, "a=" + String(a) + " b=" + String(b));
  } else {
    printLcd(0, "CAL. FAULTED!!!");
    printLcd(1, "a=" + String(pA) + " b=" + String(pB));
  }

  delay(5000);
  
}

/* Test program */
float getTestpH() {
  long randNumber;
  printLcd(1, "RUNNING IN TEST!");
  delay(1000);
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
 * We get the voltage of the sensor, then using the equation that we have calculated with the pH reference values we convert pHVol
 * to pHValue.
 */
float getpH () {
  float pHVol = getSensorVoltage();
  delay(1000);
  
  float phValue = a * pHVol + b;

  return phValue;
}

/**
 * Returns the voltage measured by the pH sensor
 * The code consists of taking 10 samples of the analogue input A0, ordering them and discarding the highest and 
 * the lowest and calculating the mean with the six remaining samples by converting this value to voltage in the 
 * variable pHVol.
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
  float pHVol = (float)avgValue * 5.0 / 1024 / 6; // 6 measurements: convert measured int to voltage (0-1024 range to 0-5V range), use this value also to calibrate your pH sensor

  return pHVol;
}

/* Easy way to communicate with lcd */
void printLcd(int row, String value) {
  Serial.println(value);
  lcd.setCursor(0, row);
  lcd.print(appendSpaces(value, 16));
}

/* Returns string of length len, by appending spaces to the right */
String appendSpaces(String text, int len) {
  String iText = text;
  if(iText.length() > len) {
    iText = iText.substring(1, 17);
  }
  while (iText.length() < len) {
    iText.concat(" ");
  };
  return iText;
}

void displayPauzeMessage(int row, String text, int delaySeconds) {
  for(int i = delaySeconds; i > 0; i--) {
    printLcd(row, text + " " + String(i));
    delay(1000);
  }
}
