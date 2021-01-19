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
#include "string2char.h"
#include <HX711_ADC.h>
#include <EEPROM.h>

#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

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

//max voltage in esp32 is 3.3volt
int min_volt = 10;
int max_volt = 200;
float ref_voltage = (1.0 / 255.0) * 3.3;

float newCalibrationValue = 1.23f;

float kg_factor = 1000.0f;

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

    SerialBT.println("***");
    SerialBT.println("Tara started");
    SerialBT.println("Tara finished");
    SerialBT.println("***");
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

    temp_adress += sizeof(max_break_redfac);
    EEPROM.get(temp_adress, max_volt); //get also max voltage

    temp_adress += sizeof(max_volt);
    EEPROM.get(temp_adress, min_volt); //get also min voltage

    Serial.println("");
    Serial.print("Max Brake Kg: ");
    Serial.print(max_break / kg_factor);
    Serial.println("");

    Serial.println("");
    Serial.print("Min Brake RedFac %: ");
    Serial.print(max_break_redfac);
    Serial.println("");

    Serial.println("");
    Serial.print("Min Brake Kg: ");
    Serial.print(min_break / kg_factor);
    Serial.println("");

    Serial.println("");
    Serial.print("calibration value : ");
    Serial.print(newCalibrationValue);
    Serial.println(", use this as calibration value (calFactor) in your project sketch.");

    Serial.println("");
    Serial.print("Max Voltage: ");
    Serial.print(max_volt);
    Serial.print("/");
    Serial.print(max_volt * ref_voltage);
    Serial.println("");

    Serial.println("");
    Serial.print("Min Voltage: ");
    Serial.print(min_volt);
    Serial.print("/");
    Serial.print(min_volt * ref_voltage);
    Serial.println("");

    SerialBT.println("");
    SerialBT.print("Max Brake Kg: ");
    SerialBT.println(max_break / kg_factor);
    SerialBT.print("Min Brake RedFac %: ");
    SerialBT.println(max_break_redfac);
    SerialBT.print("Min Brake Kg: ");
    SerialBT.println(min_break / kg_factor);
    SerialBT.print("calibration value : ");
    SerialBT.println(newCalibrationValue);
    SerialBT.print("Max Voltage: ");
    SerialBT.print(max_volt);
    SerialBT.print("/");
    SerialBT.println(max_volt * ref_voltage);
    SerialBT.print("Min Voltage: ");
    SerialBT.print(min_volt);
    SerialBT.print("/");
    SerialBT.println(min_volt * ref_voltage);
    SerialBT.println("");
}

void calibrate()
{

    tara_load_cell();

    //refresh the dataset to be sure that the known mass is measured correct
    LoadCell.refreshDataSet();

    Serial.println("***");
    Serial.println("Start calibration max load:");
    Serial.println("Push Pedal to max posistion and max force for your requirements");
    Serial.println("Serial Command is 'y' or 'n' not to change this value");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.println("Start calibration max load:");
    SerialBT.println("Push Pedal to max posistion and max force for your requirements");
    SerialBT.println("SerialBT Command is 'y' or 'n' not to change this value");
    SerialBT.println("***");

    boolean _resume = false;
    while (_resume == false)
    {
        LoadCell.update();
        if (Serial.available() > 0 || SerialBT.available())
        {
            char inByte = Serial.read();
            char BTByte = SerialBT.read();
            if (inByte == 'y' || BTByte == 'y')
            {
                max_break = LoadCell.getData(); //muss be less then max
                _resume = true;
            }
            if (inByte == 'n' || BTByte == 'n')
            {
                _resume = true;
            }
        }
    }

    Serial.println("***");
    Serial.println("Start calibration min load:");
    Serial.println("Push Pedal to min posistion and min force for your requirements");
    Serial.println("at zero pos the breaks starts immediately");
    Serial.println("or give the pedal a deadzone before breaks starts immediately");
    Serial.println("Serial Command is 'y' or 'n' not to change this value");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.println("Start calibration min load:");
    SerialBT.println("Push Pedal to min posistion and min force for your requirements");
    SerialBT.println("at zero pos the breaks starts immediately");
    SerialBT.println("or give the pedal a deadzone before breaks starts immediately");
    SerialBT.println("SerialBT Command is 'y' or 'n' not to change this value");
    SerialBT.println("***");

    _resume = false;
    while (_resume == false)
    {
        LoadCell.update();
        if (Serial.available() > 0)
        {
            char inByte = Serial.read();
            if (inByte == 'y')
            {
                float temp_min_break = 0.00f;        //a temp value since if something wrong we do not change the original variable
                temp_min_break = LoadCell.getData(); //muss be less then max
                if (temp_min_break <= (0.9 * max_break))
                {
                    min_break = temp_min_break;
                    _resume = true;
                }
                else
                {
                    Serial.println("***");
                    Serial.println("to close to max break point, try again with less force");
                    Serial.println("***");

                    SerialBT.println("***");
                    SerialBT.println("to close to max break point, try again with less force");
                    SerialBT.println("***");
                    _resume = false;
                }
            }
            if (inByte == 'n')
            {
                _resume = true;
            }
        }
    }

    Serial.println("***");
    Serial.println("give in max break reduction factor in % and hit return. Return without factor no changes");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.println("give in max break reduction factor in % and hit return. Return without factor no changes");
    SerialBT.println("***");

    _resume = false;
    while (_resume == false)
    {
        if (Serial.available() > 0)
        {
            float temp_redfac = 100.0f;
            temp_redfac = Serial.parseFloat();
            if (temp_redfac == 0.0)
            {
                _resume = true;
            }
            else if (temp_redfac <= 100.0f && temp_redfac >= 0.0f)
            {
                max_break_redfac = temp_redfac;
                _resume = true;
            }
            else
            {
                Serial.println("***");
                Serial.println("something wrong, between 0.00 and 100.00 as input");
                Serial.println("***");

                SerialBT.println("***");
                SerialBT.println("something wrong, between 0.00 and 100.00 as input");
                SerialBT.println("***");
            }
        }
    }

    Serial.println("");
    Serial.print("Max Brake Kg: ");
    Serial.print(max_break / kg_factor);
    Serial.println("");

    Serial.println("");
    Serial.print("Max Brake Reduce Factor: ");
    Serial.print(max_break_redfac);
    Serial.println("");

    Serial.println("");
    Serial.print("Min Brake Kg: ");
    Serial.print(min_break / kg_factor);
    Serial.println("");

    Serial.println("");
    Serial.print("New calibration value: ");
    Serial.print(newCalibrationValue);
    Serial.println("");

    Serial.println("");
    Serial.print("To save new Data in EEPROM hit 's' or not save 'u'");
    Serial.println("");

    SerialBT.println("");
    SerialBT.print("Max Brake Kg: ");
    SerialBT.print(max_break / kg_factor);
    SerialBT.println("");

    SerialBT.println("");
    SerialBT.print("Max Brake Reduce Factor: ");
    SerialBT.print(max_break_redfac);
    SerialBT.println("");

    SerialBT.println("");
    SerialBT.print("Min Brake Kg: ");
    SerialBT.print(min_break / kg_factor);
    SerialBT.println("");

    SerialBT.println("");
    SerialBT.print("New calibration value: ");
    SerialBT.print(newCalibrationValue);
    SerialBT.println("");

    SerialBT.println("");
    SerialBT.print("To save new Data in EEPROM hit 's' or not save 'u'");
    SerialBT.println("");

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

                SerialBT.println("Saved to EEPROM");
                SerialBT.println("");
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

    SerialBT.println("End calibration");
    SerialBT.println("***");
    SerialBT.println("To re-calibrate/calibrate, send 'c' from SerialBT monitor.");
    SerialBT.println("***");
}

void voltage_cali()
{
    int temp_volt = 200;
    int temp_volt_max = 200;
    int temp_volt_min = 100;

    Serial.println("***");
    Serial.println("Current Max Voltage (0-255/Voltage): ");
    Serial.print(max_volt);
    Serial.print("/");
    Serial.println(max_volt * ref_voltage);
    Serial.println("Start calibration max voltate to get 100% breaking:");
    Serial.println("Adjust the voltage up or down to get max breaking (100%)");
    Serial.println("Values are given in a range between 0 and 255, the voltage Value will be displayed");
    Serial.println("Serial Command is 'y' for good or 'n' not to change this value");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.println("Current Max Voltage (0-255/Voltage): ");
    SerialBT.print(max_volt);
    SerialBT.print("/");
    SerialBT.println(max_volt * ref_voltage);
    SerialBT.println("Start calibration max voltate to get 100% breaking:");
    SerialBT.println("Adjust the voltage up or down to get max breaking (100%)");
    SerialBT.println("Values are given in a range between 0 and 255, the voltage Value will be displayed");
    SerialBT.println("SerialBT Command is 'y' for good or 'n' not to change this value");
    SerialBT.println("***");

    boolean _resume = false;
    temp_volt = 200;
    while (_resume == false)
    {
        if (Serial.available() > 0)
        {
            String inByte = Serial.readStringUntil('\n');
            temp_volt = inByte.toInt();
            if (inByte == "y")
            {
                max_volt = temp_volt_max;
                _resume = true;
            }
            else if (inByte == "n")
            {
                _resume = true;
            }
            else if (temp_volt >= 0 || temp_volt <= 255)
            {
                temp_volt_max = temp_volt;

                dacWrite(DAC1, temp_volt); //sending voltage to break and look at TV/Monitor to find Max Break

                Serial.println("Temporary MAX Voltage (0-255/Voltage): ");
                Serial.print(temp_volt);
                Serial.print("/");
                Serial.print(temp_volt * ref_voltage);
                Serial.println("");

                SerialBT.println("Temporary MAX Voltage (0-255/Voltage): ");
                SerialBT.print(temp_volt);
                SerialBT.print("/");
                SerialBT.print(temp_volt * ref_voltage);
                SerialBT.println("");
            }
            else if (temp_volt < 0 || temp_volt > 255)
            {
                temp_volt = 200;
            }
        }
    }

    Serial.println("***");
    Serial.println("Current Min Voltage (0-255/Voltage): ");
    Serial.print(min_volt);
    Serial.print("/");
    Serial.println(min_volt * ref_voltage);
    Serial.println("Start calibration min voltate to get ZERO% breaking:");
    Serial.println("Adjust the voltage up or down to get min breaking (ZERO%)");
    Serial.println("Values are given in a range between 0 and 255, the voltage Value will be displayed");
    Serial.println("Serial Command is 'y' for good or 'n' not to change this value");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.println("Current Min Voltage (0-255/Voltage): ");
    SerialBT.print(min_volt);
    SerialBT.print("/");
    SerialBT.println(min_volt * ref_voltage);
    SerialBT.println("Start calibration min voltate to get ZERO% breaking:");
    SerialBT.println("Adjust the voltage up or down to get min breaking (ZERO%)");
    SerialBT.println("Values are given in a range between 0 and 255, the voltage Value will be displayed");
    SerialBT.println("SerialBT Command is 'y' for good or 'n' not to change this value");
    SerialBT.println("***");

    _resume = false;
    temp_volt = 100;
    while (_resume == false)
    {
        if (Serial.available() > 0)
        {
            String inByte = Serial.readStringUntil('\n');
            temp_volt = inByte.toInt();
            if (inByte == "y")
            {
                min_volt = temp_volt_min;
                _resume = true;
            }
            else if (inByte == "n")
            {
                _resume = true;
            }
            else if (temp_volt >= 0 || temp_volt <= 255)
            {
                temp_volt_min = temp_volt;
                dacWrite(DAC1, temp_volt); //sending voltage to break and look at TV/Monitor to find Max Break
                Serial.println("Temporary MIN Voltage (0-255/Voltage): ");
                Serial.print(temp_volt);
                Serial.print("/");
                Serial.print(temp_volt * ref_voltage);
                Serial.println("");

                SerialBT.println("Temporary MIN Voltage (0-255/Voltage): ");
                SerialBT.print(temp_volt);
                SerialBT.print("/");
                SerialBT.print(temp_volt * ref_voltage);
                SerialBT.println("");
            }
            else if (temp_volt < 0 || temp_volt > 255)
            {
                temp_volt = 100;
            }
        }
    }

    Serial.println("");
    Serial.print("Max Voltage: ");
    Serial.print(max_volt);
    Serial.print("/");
    Serial.print(max_volt * ref_voltage);
    Serial.println("");

    Serial.println("");
    Serial.print("Min Voltage: ");
    Serial.print(min_volt);
    Serial.print("/");
    Serial.print(min_volt * ref_voltage);
    Serial.println("");

    Serial.println("");
    Serial.print("To save new Data in EEPROM hit 's' or not save 'u'");
    Serial.println("");

    SerialBT.println("");
    SerialBT.print("Max Voltage: ");
    SerialBT.print(max_volt);
    SerialBT.print("/");
    SerialBT.print(max_volt * ref_voltage);
    SerialBT.println("");

    SerialBT.println("");
    SerialBT.print("Min Voltage: ");
    SerialBT.print(min_volt);
    SerialBT.print("/");
    SerialBT.print(min_volt * ref_voltage);
    SerialBT.println("");

    SerialBT.println("");
    SerialBT.print("To save new Data in EEPROM hit 's' or not save 'u'");
    SerialBT.println("");

    _resume = false;
    while (_resume == false)
    {
        if (Serial.available() > 0)
        {
            char inByte = Serial.read();
            if (inByte == 's')
            {
#if defined(ESP8266) || defined(ESP32)
                EEPROM.begin(512);
#endif
                int temp_adress = 0;
                temp_adress += sizeof(newCalibrationValue);
                temp_adress += sizeof(max_break);
                temp_adress += sizeof(min_break);

                temp_adress += sizeof(max_break_redfac);
                EEPROM.put(temp_adress, max_volt); //get also max voltage
                temp_adress += sizeof(max_volt);
                EEPROM.put(temp_adress, min_volt); //get also min voltage

#if defined(ESP8266) || defined(ESP32)
                EEPROM.commit();
#endif

                Serial.println("Saved to EEPROM");
                Serial.println("");

                SerialBT.println("Saved to EEPROM");
                SerialBT.println("");
                _resume = true;
            }
            else if (inByte == 'u')
            {
                _resume = true;
            }
        }
    }

    Serial.println("End voltage calibration");
    Serial.println("***");
    Serial.println("To re-calibrate/calibrate, send 'v' from serial monitor.");
    Serial.println("***");

    SerialBT.println("End voltage calibration");
    SerialBT.println("***");
    SerialBT.println("To re-calibrate/calibrate, send 'v' from serial monitor.");
    SerialBT.println("***");

    delay(10000);
}

void weight_reference_calibration_first_time()
{
    float newCalibrationValue = 0.00f;

    Serial.println("***");
    Serial.println("Start calibration of the reference weight:");
    Serial.println("This should just be done one time as calibration of a reference point to messure in Kg later:");
    Serial.println("Place the load cell an a level stable surface.");
    Serial.println("Remove any load applied to the load cell.");
    Serial.println("Send 't' from serial monitor to set the tare offset.");

    SerialBT.println("***");
    SerialBT.println("Start calibration of the reference weight:");
    SerialBT.println("This should just be done one time as calibration of a reference point to messure in Kg later:");
    SerialBT.println("Place the load cell an a level stable surface.");
    SerialBT.println("Remove any load applied to the load cell.");
    SerialBT.println("Send 't' from SerialBT monitor to set the tare offset.");

    boolean _resume = false;
    while (_resume == false)
    {
        LoadCell.update();
        if (Serial.available() > 0)
        {
            if (Serial.available() > 0)
            {
                char inByte = Serial.read();
                if (inByte == 't')
                    LoadCell.tareNoDelay();
            }
        }
        if (LoadCell.getTareStatus() == true)
        {
            Serial.println("Tare complete");
            SerialBT.println("Tare complete");
            _resume = true;
        }
    }

    Serial.println("Now, place your known mass on the loadcell.");
    Serial.println("Then send the weight of this mass in GRAM from serial monitor. (example 1234 = 1.234kg)");
    Serial.println("or 0 or return without changes");

    SerialBT.println("Now, place your known mass on the loadcell.");
    SerialBT.println("Then send the weight of this mass in GRAM from serial monitor. (example 1234 = 1.234kg)");
    SerialBT.println("or 0 or return without changes");

    float known_mass = 0;
    _resume = false;
    while (_resume == false)
    {
        LoadCell.update();
        if (Serial.available() > 0)
        {
            known_mass = Serial.parseFloat();
            if (known_mass != 0)
            {
                Serial.print("Known mass is: ");
                Serial.println(known_mass);

                SerialBT.print("Known mass is: ");
                SerialBT.println(known_mass);
                _resume = true;
            }
            else if (known_mass == 0)
            {
                _resume = true;
            }
        }
    }

    LoadCell.refreshDataSet();

    if (known_mass != 0)
    {
        //refresh the dataset to be sure that the known mass is measured correct

        //get the new calibration value
        newCalibrationValue = LoadCell.getNewCalibration(known_mass);

        Serial.print("New calibration value has been set to: ");
        Serial.print(newCalibrationValue);
        Serial.println(", use this as calibration value (calFactor) in your project sketch.");
        Serial.print("Save this value to EEPROM adress ");
        Serial.print(calVal_eepromAdress);
        Serial.println("? y/n");

        SerialBT.print("New calibration value has been set to: ");
        SerialBT.print(newCalibrationValue);
        SerialBT.println(", use this as calibration value (calFactor) in your project sketch.");
        SerialBT.print("Save this value to EEPROM adress ");
        SerialBT.print(calVal_eepromAdress);
        SerialBT.println("? y/n");

        _resume = false;
        while (_resume == false)
        {
            if (Serial.available() > 0)
            {
                char inByte = Serial.read();
                if (inByte == 'y')
                {
                    Serial.print("");
                    Serial.println("You are realy sure, since this will change the referece point for displaying right Kg");
                    Serial.print("Will not affect the calulation of max and min break but the values are perhaps not right in Kg");
                    Serial.print("But relative to max and min will be coorect, is just for the 'optic'");
                    Serial.print("Type in 'y' to save into EEPROM or 'n' for no saving into EEPROM");
                    Serial.print("");

                    SerialBT.print("");
                    SerialBT.println("You are realy sure, since this will change the referece point for displaying right Kg");
                    SerialBT.print("Will not affect the calulation of max and min break but the values are perhaps not right in Kg");
                    SerialBT.print("But relative to max and min will be coorect, is just for the 'optic'");
                    SerialBT.print("Type in 'y' to save into EEPROM or 'n' for no saving into EEPROM");
                    SerialBT.print("");

                    while (_resume == false)
                    {

                        if (Serial.available() > 0)
                        {
                            inByte = Serial.read();
                            if (inByte == 'y')
                            {
#if defined(ESP8266) || defined(ESP32)
                                EEPROM.begin(512);
#endif
                                EEPROM.put(calVal_eepromAdress, newCalibrationValue);

#if defined(ESP8266) || defined(ESP32)
                                EEPROM.commit();
#endif
                                EEPROM.get(calVal_eepromAdress, newCalibrationValue);
                                Serial.print("Value ");
                                Serial.print(newCalibrationValue);
                                Serial.print(" saved to EEPROM address: ");
                                Serial.println(calVal_eepromAdress);

                                SerialBT.print("Value ");
                                SerialBT.print(newCalibrationValue);
                                SerialBT.print(" saved to EEPROM address: ");
                                SerialBT.println(calVal_eepromAdress);
                                _resume = true;
                            }
                        }
                    }
                }
                else if (inByte == 'n')
                {
                    Serial.println("Value not saved to EEPROM");
                    SerialBT.println("Value not saved to EEPROM");
                    _resume = true;
                }
            }
        }

        Serial.println("End weight_reference_calibration_first_time");
        Serial.println("***");

        SerialBT.println("End weight_reference_calibration_first_time");
        SerialBT.println("***");
    }
    else
    {
        Serial.println("End weight_reference_calibration_first_time without chnages");
        Serial.println("***");

        SerialBT.println("End weight_reference_calibration_first_time without chnages");
        SerialBT.println("***");
    }

    delay(10000);
}

void setup()
{
    Serial.begin(115200);
    delay(10);

    SerialBT.begin("ESP32_G29_BreakSys"); //Bluetooth device name

    setCpuFrequencyMhz(80); //Set CPU clock to 80MHz fo example
    getCpuFrequencyMhz();   //Get CPU clock

    Serial.println("");
    Serial.print("CPU Mhz: ");
    Serial.print(getCpuFrequencyMhz()); //Get CPU clock)
    Serial.println("");  

    Serial.println("The device started, now you can pair it with bluetooth!");

    Serial.println();
    Serial.println("Starting...");

    SerialBT.println("");
    SerialBT.print("CPU Mhz: ");
    SerialBT.print(getCpuFrequencyMhz()); //Get CPU clock)
    SerialBT.println("");  

    SerialBT.println("The device started, now you can pair it with bluetooth!");

    SerialBT.println();
    SerialBT.println("Starting...");


    LoadCell.begin();
    long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
    boolean _tare = false;       //set this to false if you don't want tare to be performed in the next step
    LoadCell.start(stabilizingtime, _tare);
    if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag())
    {
        Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
        SerialBT.println("Timeout, check MCU>HX711 wiring and pin designations");
        while (1)
            ;
    }
    else
    {
        LoadCell.setCalFactor(1.0); // user set calibration value (float), initial value 1.0 may be used for this sketch
        Serial.println("Startup is complete");
        SerialBT.println("Startup is complete");
    }

    tara_load_cell(); // tara load cells
    delay(5000);

    load_variables_eeprom(); //load in the variables
    delay(10000);
}

void loop()
{
    static boolean newDataReady = 0;
    const int serialPrintInterval = 1000; //increase value to slow down serial print activity

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
            Serial.print(" LC output Kg: ");
            Serial.print(i / kg_factor);
            Serial.print(" dev2tara%: ");
            Serial.print((i * 100.0) / max_break);

            if (i > max_break)
                i = max_break;
            if (i < 0.00)
                i = 0.00;

            float gi = i;
            float reduces_break = ((float)max_volt * max_break_redfac) / 100.0; //if max break to much, reduce not input side but output side = less voltage

            if (gi >= min_break)
            {

                float GLED = mapping(gi, min_break, max_break, min_volt, reduces_break); //LED need approx 160 as min value to shine
                dacWrite(DAC1, round(GLED));                                             //Green LED
                Serial.print("  GLED: ");
                Serial.print(GLED);
                Serial.print("/");
                Serial.print((GLED / 255.0) * 3.30);
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
            Serial.print(max_break / kg_factor);

            Serial.print("  Max Break RedFac: ");
            Serial.print(max_break_redfac);

            Serial.print("  MinB: ");
            Serial.print(min_break / kg_factor);

            Serial.println("");

            newDataReady = 0;
            t = millis();
        }
    }

    // receive command from serial terminal
    if (Serial.available() > 0)
    {
        String inByte = Serial.readStringUntil('\n');
        if (inByte == "t")
            tara_load_cell(); //tare
        else if (inByte == "c")
            calibrate(); //calibrate
        else if (inByte == "v")
            voltage_cali(); //calibrate
        else if (inByte == "b")
            load_variables_eeprom(); //calibrate
        else if (inByte == "www")
            weight_reference_calibration_first_time(); //calibrate
    }

    if (SerialBT.available())
    {
        char BTByte = SerialBT.read();
        if (BTByte == 't')
            tara_load_cell(); //tare
        else if (BTByte == 'c')
            calibrate(); //calibrate
        else if (BTByte == 'v')
            voltage_cali(); //calibrate
        else if (BTByte == 'b')
            load_variables_eeprom(); //calibrate
        else if (BTByte == 'w')
            weight_reference_calibration_first_time(); //calibrate
    }
}
