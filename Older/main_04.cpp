#include <Arduino.h>
/*
   -------------------------------------------------------------------------------------
   HX711_ADC
   Arduino library for HX711 24-Bit Analog-to-Digital Converter for Weight Scales
   Olav Kallhovd sept2017
   -------------------------------------------------------------------------------------
*/

/*
   This example file shows how to calibrate the load cell and optionally store the calibration
   value in EEPROM, and also how to change the value manually.
   The result value can then later be included in your project sketch or fetched from EEPROM.

   To implement calibration in your project sketch the simplified procedure is as follow:
       LoadCell.tare();
       //place known mass
       LoadCell.refreshDataSet();
       float newCalibrationValue = LoadCell.getNewCalibration(known_mass);
*/

#include <HX711_ADC.h>
#include <EEPROM.h>

//pins:
const int HX711_dout = 27; //mcu > HX711 dout pin
const int HX711_sck = 14;  //mcu > HX711 sck pin

//DAC output
#define DAC1 26 // Green LED
#define DAC2 25 // Red LED

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
long t;

long count = 0;
int count1 = 0;

float max_break = 21.23f;
float max_break_redfac = 100.00f; //if max break to much reduce here in %
float min_break = 1.43f;

float min_volt = 0.00f;
float max_volt = 3.30f;

float newCalibrationValue = 1.23f;

float mapping(float value, float in_min, float in_max, float out_min, float out_max)
{
  return (out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min));
}

void tara_load_cell()
{
  Serial.println("***");
  Serial.println("Tara started");
  LoadCell.update();
  LoadCell.tareNoDelay();

  Serial.println("Tara finished");
  Serial.println("***");
}

void load_variables_eeprom()
{

#if defined(ESP8266) || defined(ESP32)
  EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
  int temp_adress = 0;
  EEPROM.get(calVal_eepromAdress, newCalibrationValue);
  LoadCell.setCalFactor(newCalibrationValue); // set calibration value (float)

  temp_adress += sizeof(newCalibrationValue);
  EEPROM.get(temp_adress, max_break); //get  max break

  temp_adress += sizeof(max_break);
  EEPROM.get(temp_adress, min_break); //get also mix break

  temp_adress += sizeof(min_break);
  EEPROM.get(temp_adress, max_break_redfac); //get also max reduced break factor

  Serial.println("");
  Serial.print("Max Brake loaded: ");
  Serial.print(max_break);
  Serial.println("");

  Serial.println("");
  Serial.print("Min Brake RedFac loaded: ");
  Serial.print(max_break_redfac);
  Serial.println("");

  Serial.println("");
  Serial.print("Min Brake loaded: ");
  Serial.print(min_break);
  Serial.println("");

  Serial.println("");
  Serial.print("calibration value : ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.println("");
}

void calibrate()
{
  float known_mass = 100000.0; //definded as max load

  //refresh the dataset to be sure that the known mass is measured correct
  LoadCell.refreshDataSet();

  Serial.println("***");
  Serial.println("Start calibration max load:");
  Serial.println("Push Pedal to max posistion and max force for your requirements");
  Serial.println("Serial Command is 'm'");
  Serial.println("***");

  boolean _resume = false;
  while (_resume == false)
  {
    LoadCell.update();
    if (Serial.available() > 0)
    {
      char inByte = Serial.read();
      if (inByte == 'm')
      {
        //get the new calibration value and the reference data for all other calulations
        newCalibrationValue = LoadCell.getNewCalibration(known_mass);
        max_break = LoadCell.getData(); //muss give the same as known_mass
        if (max_break < known_mass * 0.999 && max_break > known_mass * 1.001)
        {
          _resume = false;
          Serial.println("");
          Serial.println("max_break is: ");
          Serial.print(max_break);
          Serial.println("max_break & known_mass deviating to much, hit 'm' again please");
          Serial.println("");
        }
        else
        {
          _resume = true;
        }
      }
    }
  }

  Serial.println("***");
  Serial.println("Start calibration min load:");
  Serial.println("Push Pedal to min posistion and min force for your requirements");
  Serial.println("at zero pos the breaks starts immediately");
  Serial.println("or give the pedal a deadzone before breaks starts immediately");
  Serial.println("Serial Command is 'i'");
  Serial.println("***");

  _resume = false;
  while (_resume == false)
  {
    LoadCell.update();
    if (Serial.available() > 0)
    {
      char inByte = Serial.read();
      if (inByte == 'i')
      {
        min_break = LoadCell.getData(); //muss be less then max
        if (min_break <= (0.8 * max_break))
        {
          _resume = true;
        }
        else
        {
          Serial.println("***");
          Serial.println("to close to max break point, try again with less force");
          Serial.println("***");
        }
      }
    }
  }

  Serial.println("***");
  Serial.println("give in max break reduction factor in % and hit return. Return without factor=100%");
  Serial.println("***");

  _resume = false;
  while (_resume == false)
  {
    if (Serial.available() > 0)
    {
      max_break_redfac = Serial.parseFloat();
      if (max_break_redfac == 0.0)
      {
        max_break_redfac = 100.00f;
        _resume = true;
      }
      else if (max_break_redfac <= 100.0f && max_break_redfac > 0.0f)
      {
        _resume = true;
      }
      else
      {
        Serial.println("***");
        Serial.println("something wrong, between 0.00 and 100.00 as input");
        Serial.println("***");
      }
    }
  }

  Serial.println("");
  Serial.print("Max Brake: ");
  Serial.print(max_break);
  Serial.println("");

  Serial.println("");
  Serial.print("Max Brake Reduce Factor: ");
  Serial.print(max_break_redfac);
  Serial.println("");

  Serial.println("");
  Serial.print("Min Brake: ");
  Serial.print(min_break);
  Serial.println("");

  Serial.println("");
  Serial.print("New calibration value: ");
  Serial.print(newCalibrationValue);
  Serial.println("");

  Serial.println("");
  Serial.print("To save new Data in EEPROM hit 's' or not save 'u'");
  Serial.println("");

  _resume = false;
  while (_resume == false)
  {
    LoadCell.update();
    if (Serial.available() > 0)
    {
      char inByte = Serial.read();
      if (inByte == 's')
      {
#if defined(ESP8266) || defined(ESP32)
        EEPROM.begin(512);
#endif
        int temp_adress = 0;
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);

        temp_adress += sizeof(newCalibrationValue);
        EEPROM.put(temp_adress, max_break); //Save also max break

        temp_adress += sizeof(max_break);
        EEPROM.put(temp_adress, min_break); //Save also max break

        temp_adress += sizeof(min_break);
        EEPROM.put(temp_adress, max_break_redfac); //get also max reduced break factor

#if defined(ESP8266) || defined(ESP32)
        EEPROM.commit();
#endif

        Serial.println("Saved to EEPROM");
        Serial.println("");
        _resume = true;
      }
      else if (inByte == 'u')
      {
        _resume = true;
      }
    }
  }

  Serial.println("End calibration");
  Serial.println("***");
  Serial.println("To re-calibrate/calibrate, send 'c' from serial monitor.");
  Serial.println("***");
}

void setup()
{
  Serial.begin(57600);
  delay(10);

  setCpuFrequencyMhz(80); //Set CPU clock to 80MHz fo example
  getCpuFrequencyMhz();   //Get CPU clock
  Serial.println("");
  Serial.print(" CPU Mhz: ");
  Serial.print(getCpuFrequencyMhz()); //Get CPU clock)
  Serial.println("");

  Serial.println();
  Serial.println("Starting...");

  LoadCell.begin();
  long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = false;       //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag())
  {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1)
      ;
  }
  else
  {
    LoadCell.setCalFactor(1.0); // user set calibration value (float), initial value 1.0 may be used for this sketch
    Serial.println("Startup is complete");
  }

  tara_load_cell(); // tara load cells
  delay(5000);

  load_variables_eeprom(); //load in the variables
  delay(10000);
}

void loop()
{
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update())
    newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady)
  {
    if (millis() > t + serialPrintInterval)
    {
      float i = LoadCell.getData();
      Serial.print(count++);
      Serial.print(" LC output val: ");
      Serial.print(i);
      Serial.print(" dev2tara%: ");
      Serial.print((i * 100.0) / max_break);

      if (i > max_break)
        i = max_break;
      if (i < 0.00)
        i = 0.00;

      float gi = i;
      float reduces_break = (255.0 * max_break_redfac) / 100.0; //if max break to much, reduce not input side but output side = less voltage

      if (gi >= min_break)
      {

        float GLED = mapping(gi, min_break, max_break, 160.0, reduces_break); //LED need approx 160 as min value to shine
        dacWrite(DAC1, round(GLED));                                          //Green LED
        Serial.print("  GLED: ");
        Serial.print(GLED);
        Serial.print("/");
        Serial.print((GLED/255.0)*3.30);
        Serial.print(" Volt");
      }
      else
      {
        dacWrite(DAC1, 0); //Green LED
        Serial.print("  GLED: ");
        Serial.print(0);
        Serial.print("/");
        Serial.print(0.00);
        Serial.print(" Volt");
      }

      float ri = i;
      if (ri < max_break * 0.80)
      {
        dacWrite(DAC2, 0); //Red LED
        Serial.print("  RLED: ");
        Serial.print(0);
      }
      else
      {
        float RLED = mapping(ri, max_break * 0.80, max_break, 160.0, reduces_break); //LED need approx 160 as min value to shine
        dacWrite(DAC2, round(RLED));                                                 //Red LED
        Serial.print("  RLED: ");
        Serial.print(RLED);
      }

      Serial.print("  MaxB: ");
      Serial.print(max_break);

      Serial.print("  Max Break RedFac: ");
      Serial.print(max_break_redfac);

      Serial.print("  MinB: ");
      Serial.print(min_break);

      Serial.println("");

      newDataReady = 0;
      t = millis();
    }
  }

  // receive command from serial terminal
  if (Serial.available() > 0)
  {
    float i;
    char inByte = Serial.read();
    if (inByte == 't')
      tara_load_cell(); //tare
    else if (inByte == 'c')
      calibrate(); //calibrate
  }
}
