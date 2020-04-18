# arduino-iba-ph-stearing
Hou met behulp van een Arduino de pH van je IBA op peil

De PH is dikwijls te laag door lage zuurtegraad van regenwater en stadswater. Minimale pH voor IBA's is wettelijk bepaald op 7,5. Met dit opzet monitor je de pH met een digitale pH meter en met een doseerpomp met NaOH 1 M (mol/l) om de pH te verhogen.

Hardware:
 - Peristaltic pump 12V DC DIY Liquid Dosing Pump for Aquarium Lab Analytical
       https://www.aliexpress.com/item/32770860268.html
 - pH measurement with Liquid PH 0-14 Value Detection Regulator Sensor Module Monitoring Control Meter Tester + BNC PH Electrode Probe For Arduino
       https://www.aliexpress.com/item/32964738891.html
 - NaOH 1 M: (NaOH 40g/mol > voor 1 mol/l 40g oplossen in 1l)

Setup
-----
Pump
----
We drive the pump with Arduino 5V power supply: directly connect one pole to an digital outpin, the other to GND

pH meter
--------
Code for the pH meter is based on: https://scidle.com/how-to-use-a-ph-sensor-with-arduino/

First calibrate the pH with buffer fluids. Measure the voltage at p0, using the Arduino int value.
         y = ax + b // y = ph, x = voltage, a en b are to be determined
         
         4,01 = a4,08 + b // first buffersolution is pH 4,01, measured voltage is 4,08
         6,86 = a3,61 + b // second buffersolution is pH 6,86, measured voltage is 3,63
         
         b = 4,01 - a4,08
         6,86 = a3,61 + (4,01 - a4,08) // substitute the value of b of the first equation into the second in order to calculate a
         6,86 = a3,61 + 4,01 - a4,08
         6,86 = 3,61a + 4,01 - 4,08a
         6,86 - 4,01 = a (3,61 - 4,08)
         2,85 = - 0,47a
         a = -6,06
         b = 4,01 - (-6,064,08) = 28,75 // fill in a into the first equation to determine b
         
         => 6,86 = -6,063,61 + 28,75 =  // dubbelcheck
         
         (You can use arduino_ph_berekening.xls to compute the values)

To connect with Arduino we will need an analog input (A0), power (5V) and two GND that actually in the sensor circuit are separated but we can use the same.

Pinout <BR>
   To  Temperature     > not used <BR>
   Do  Limit pH Signal > not used <BR>
   Po  Analog pH value > A0 <BR>
   G   Analog GND      > GND <BR>
   G   Supply GND      > GND <BR>
   V+  Supply (5V)     > 5V <BR>
