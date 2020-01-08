/**
 * Control the pH from your IBA (Indivuele Behandeling van Afvalwater) output
 * 
 * Hardware:
 *  - Peristaltic pump 12V DC DIY Liquid Dosing Pump for Aquarium Lab Analytical
 *        https://www.aliexpress.com/item/32770860268.html
 *  - pH measurement with Liquid PH 0-14 Value Detection Regulator Sensor Module Monitoring Control Meter Tester + BNC PH Electrode Probe For Arduino
 *        https://www.aliexpress.com/item/32964738891.html
 * 
 * 
 * Pump
 * ----
 * We drive the pump with Arduino 5V power supply: directly connect one pole to an digital outpin, the other to GND
 * 
 * pH meter
 * --------
 * Code for the pH meter is based on: https://scidle.com/how-to-use-a-ph-sensor-with-arduino/
 * 
 * First calibrate the pH with buffer fluids. Measure the voltage at p0, using the Arduino int value.
 *          y = ax + b // y = ph, x = voltage, a en b are to be determined
 *          
 *          4,01 = a * 4,08 + b // eerste bufferoplossing is pH 4,01, gemeten voltage is 4,08
 *          6,86 = a * 3,61 + b // tweede bufferoplossing is pH 6,86, gemeten voltage is 3,63
            
 *          b = 4,01 - a * 4,08
 *          6,86 = a * 3,61 + (4,01 - a * 4,08) // substitueer waarde van b in de eerste vergelijking in de tweede vergelijking om a te berekenen
 *          6,86 = a * 3,61 + 4,01 - a * 4,08
 *          6,86 = 3,61a + 4,01 - 4,08a
 *          6,86 - 4,01 = a (3,61 - 4,08)
 *          2,85 = - 0,47 * a
 *          a = -6,06
 *          b = 4,01 - (-6,06 * 4,08) = 28,75 // vul a in de eerste vergelijking om b te berekenen
 *          
 *          => 6,86 = -6,06 * 3,61 + 28,75 =  // dubbelcheck
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
 */

// Pump
int pumpPin = 2;                  // the pin we will use as output
float alarmMin = 6.4;             // sound alarm when pH level is lower
float alarmMax = 7.5;             // sound alarm when pH level is higher
float lowPh = 6.5;                // start adding pH+ (NaOH 1M)
int pumpTime = 2000;              // the amount of time tot pump NaOH into the barrel, whithout measuring
int measurementDelayTime = 5000; // the amount of time to wait after pumping in order to stabilize pH

// pH
const int analogInPin = A0; 
int sensorValue = 0; 
unsigned long int avgValue; 
const float a = -6.06383; //@see "calibration"
const float b = 28.75043; //@see "calibration"
int buf[10],temp;

void setup() {
  Serial.begin(57600);
  pinMode(pumpPin, OUTPUT);   // initiate pump pin as output
  digitalWrite(pumpPin, LOW); // stop pump
  Serial.println("STARTED");
}

/**
 */
void loop() {
  float phValue = measurepH(); // get the pH
  Serial.print("pH = ");
  Serial.println(phValue); //  and send it to the serial port to see it in the serial monitor

  // when pH lower then 6.7: add NaOH 1 M
  if(phValue < 6.7) {
    // DOSE!
    runPump(pumpTime);
    Serial.print("delaying...");
    delay(measurementDelayTime); // wait for the NaOH to take affect
    Serial.println(" - starting");
  } else {
    delay(20);
  }
}

/**
 * pRunTime : the time to pump
 */
void runPump(int pRunTime) {
  float startTime = millis();
  digitalWrite(pumpPin, HIGH);
  Serial.println("pumping started");
  while(millis() < startTime + pRunTime) {
    // keep on dosing
  }
  digitalWrite(pumpPin, LOW);
  Serial.println("ended pumping at " + String(pRunTime));
}

/**
 * Gets pH result from pH sensor 
 * 
 * The code consists of taking 10 samples of the analogue input A0, ordering them and discarding the highest and 
 * the lowest and calculating the mean with the six remaining samples by converting this value to voltage in the 
 * variable pHVol, then using the equation that we have calculated with the pH reference values we convert pHVol
 * to pHValue.
 */
float measurepH () {
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
  avgValue=0;
  for(int i = 2; i < 8; i++) avgValue += buf[i];
  float pHVol = (float)avgValue * 5.0 / 1024 / 6; // 6 measurements: convert measured int to voltage (0-1024 range to 0-5V range), use this value also to calibrate your pH sensor
  float phValue = a * pHVol + b;

  return phValue;
}
