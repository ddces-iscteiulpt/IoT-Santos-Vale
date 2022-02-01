// EmonLibrary examples openenergymonitor.org, Licence GNU GPL V3

#include "EmonLib.h"                   // Include Emon Library
EnergyMonitor emon1;                   // Create an instance
double Irms1;
EnergyMonitor emon2;                   // Create an instance
double Irms2;
EnergyMonitor emon3;                   // Create an instance
double Irms3;

void setup()
{  
  Serial.begin(115200);  
  emon1.current(3, 111.1);             // Current: input pin, calibration.
  emon2.current(6, 111.1);             // Current: input pin, calibration.
  emon3.current(7, 111.1);             // Current: input pin, calibration.

  for (int i = 10; i >= 0; i--) {
  Irms1 = emon1.calcIrms(1480);  // Calculate Irms only
  Irms2 = emon2.calcIrms(1480);
  Irms3 = emon3.calcIrms(1480);
  Serial.print(i);
  Serial.print(" ");
  }
  
  Serial.println("Caibração iniciada:");
}

void loop()
{
  Irms1 = emon1.calcIrms(1480);  // Calculate Irms only
  Irms2 = emon2.calcIrms(1480);  // Calculate Irms only
  Irms3 = emon3.calcIrms(1480);  // Calculate Irms only
  
  Serial.print("AC Fase 1: ");
  Serial.println(Irms1);
  Serial.print("  AC Fase 2: ");
  Serial.println(Irms2);
  Serial.print("    AC Fase 3: ");
  Serial.println(Irms3);
  Serial.println(" --- ");
  delay(500);
}
