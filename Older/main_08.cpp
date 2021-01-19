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
#define DAC1 26 // Break Voltage Out to PS4/PC

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
long t;

long count = 0;
int count1 = 0;

float max_break = 21.23f;
float max_break_redfac = 100.00f; //if max break to much reduce here in %
float min_break = 1.43f;

//max voltage in esp32 is 3.3volt and adjust the break in the PC/PS4
/*
GTSport
Brake 0% - 2.86v = 221 
Brake 100% - 1.93v = 149
Assetto Corsa Comp
Brake 0% - 2.88v = 222-223
Brake 100% - 1.48v = 114
*/
int min_break_volt = 221;
int max_break_volt = 149;

float esp32Volt = 3.3;

float ref_voltage = (1.0 / 255.0) * esp32Volt;

float newCalibrationValue = 1.23f;

float kg_factor = 1000.0f;

int SerialPrintData = 0; //no Serial Printout of Data

float mapping(float value, float in_min, float in_max, float out_min, float out_max)
{
    return (out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min));
}

void SDPrint()
{
    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

    Serial.println("***");
    Serial.println("SerailDataOutput?");
    Serial.println("Send 'y' or 'n'");

    SerialBT.println("***");
    SerialBT.println("SerailDataOutput?");
    SerialBT.println("Send 'y' or 'n'");

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
                SerialPrintData = 1;
                _resume = true;
            }
            else if (inByte == 'n' || BTByte == 'n')
            {
                SerialPrintData = 0;
                _resume = true;
            }
        }
    }
    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer
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
    EEPROM.get(temp_adress, max_break_volt); //get also max voltage

    temp_adress += sizeof(max_break_volt);
    EEPROM.get(temp_adress, min_break_volt); //get also min voltage

    Serial.println("");
    Serial.print("Max Brake Kg: ");
    Serial.print(max_break / kg_factor);

    Serial.println("");
    Serial.print("Min Brake RedFac %: ");
    Serial.print(max_break_redfac);

    Serial.println("");
    Serial.print("Min Brake Kg: ");
    Serial.print(min_break / kg_factor);

    Serial.println("");
    Serial.print("calibration value : ");
    Serial.print(newCalibrationValue);

    Serial.println("");
    Serial.print("Max Break Voltage: ");
    Serial.print(max_break_volt);
    Serial.print("/");
    Serial.print(max_break_volt * ref_voltage);

    Serial.println("");
    Serial.print("Min Break Voltage: ");
    Serial.print(min_break_volt);
    Serial.print("/");
    Serial.print(min_break_volt * ref_voltage);

    SerialBT.println("");
    SerialBT.print("Max Break Kg: ");
    SerialBT.println(max_break / kg_factor);
    SerialBT.print("Min Break RedFac %: ");
    SerialBT.println(max_break_redfac);
    SerialBT.print("Min Break Kg: ");
    SerialBT.println(min_break / kg_factor);
    SerialBT.print("calibration value : ");
    SerialBT.println(newCalibrationValue);
    SerialBT.print("Max Break Volt: ");
    SerialBT.print(max_break_volt);
    SerialBT.print("/");
    SerialBT.println(max_break_volt * ref_voltage);
    SerialBT.print("Min Break Volt: ");
    SerialBT.print(min_break_volt);
    SerialBT.print("/");
    SerialBT.println(min_break_volt * ref_voltage);
    SerialBT.println("");
}

void calicalulation_break(float &value2change, float value2compare, int flag2compare, int calc_type)
{
    boolean _resume = false;
    if (calc_type == 1)
    {
        while (_resume == false)
        {
            LoadCell.update();
            if (Serial.available() > 0 || SerialBT.available())
            {
                char inByte = Serial.read();
                char BTByte = SerialBT.read();
                if (inByte == 'y' || BTByte == 'y')
                {
                    value2change = LoadCell.getData(); //muss be less then max
                    if (flag2compare == 1)
                    {
                        if (value2change <= (0.9 * value2compare))
                        {

                            _resume = true;
                        }
                        else
                        {
                            Serial.println("***");
                            Serial.println("To close to max break point, try again with less force");
                            Serial.println("***");

                            SerialBT.println("***");
                            SerialBT.println("To close to max break point");
                            SerialBT.println("Try again with less force");
                            SerialBT.println("***");
                            _resume = false;
                        }
                    }
                    else
                    {
                        _resume = true;
                    }
                }
                if (inByte == 'n' || BTByte == 'n')
                {
                    _resume = true;
                }
            }
        }
    }
    else if (calc_type == 2)
    {
        _resume = false;
        while (_resume == false)
        {
            if (Serial.available() > 0 || SerialBT.available())
            {
                float temp_redfac = 0.0f;
                temp_redfac = Serial.parseFloat();

                float temp_redfacBT = 0.0f;
                temp_redfacBT = SerialBT.parseFloat();

                if (temp_redfac == -1)
                {
                    _resume = true;
                }
                else if (temp_redfac <= 100.0f && temp_redfac > 0.0f)
                {
                    value2change = temp_redfac;
                    _resume = true;
                }

                if (temp_redfacBT == -1)
                {
                    _resume = true;
                }
                else if (temp_redfacBT <= 100.0f && temp_redfacBT > 0.0f)
                {
                    value2change = temp_redfacBT;
                    _resume = true;
                }
            }
        }
    }
}

void calibrate()
{
    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

    tara_load_cell();

    //refresh the dataset to be sure that the known mass is measured correct
    LoadCell.refreshDataSet();

    Serial.println("***");
    Serial.println("Start calibration MAX load:");
    Serial.println("Push Pedal to MAX posistion and MAX force for your requirements");
    Serial.println("Serial Command is 'y' or 'n' not to change this value");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.println("Start calibration MAX load:");
    SerialBT.println("Push Pedal to MAX pos. ");
    SerialBT.println("Then: 'y'/'n' to change val or not");
    SerialBT.println("***");

    //max_break, no value to compare to, and flag to not to compare, case 1
    calicalulation_break(max_break, 0, 0, 1);
    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

    Serial.println("***");
    Serial.println("Start calibration MIN load:");
    Serial.println("Push Pedal to MIN posistion and MIN force for your requirements");
    Serial.println("at zero pos the breaks starts immediately");
    Serial.println("or give the pedal a deadzone before breaks starts immediately");
    Serial.println("Serial Command is 'y' or 'n' not to change this value");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.println("Start calibration MIN load:");
    SerialBT.println("Push Pedal to Min pos. ");
    SerialBT.println("Then: 'y'/'n' to change val or not");
    SerialBT.println("***");

    //min_break, value to compare to, and flag to compare, case 1
    calicalulation_break(min_break, max_break, 1, 1);
    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

    Serial.println("***");
    Serial.println("give in max break reduction factor in % and hit return. -1 without factor no changes");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.println("Max break red. factor in % .");
    SerialBT.println("With '-1' no changes");
    SerialBT.println("***");

    //max_break_redfac, no value to compare to, and flag not to compare, case 2
    calicalulation_break(max_break_redfac, 0, 0, 2);
    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

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
    Serial.print("To save new Data in EEPROM hit 'y' or not save 'n'");
    Serial.println("");

    SerialBT.println("");
    SerialBT.print("Max Brake Kg: ");
    SerialBT.print(max_break / kg_factor);

    SerialBT.println("");
    SerialBT.print("Max Brake Reduce Factor: ");
    SerialBT.print(max_break_redfac);

    SerialBT.println("");
    SerialBT.print("Min Brake Kg: ");
    SerialBT.print(min_break / kg_factor);

    SerialBT.println("");
    SerialBT.print("New calibration value: ");
    SerialBT.print(newCalibrationValue);

    SerialBT.println("");
    SerialBT.print("Save into EEPROM? y/n");
    SerialBT.println("");

    bool _resume = false;
    while (_resume == false)
    {
        LoadCell.update();
        if (Serial.available() > 0 || SerialBT.available())
        {
            char inByte = Serial.read();
            char BTByte = SerialBT.read();
            if (inByte == 'y' || BTByte == 'y')
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
            else if (inByte == 'n' || BTByte == 'n')
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
}

void calicalc_volt(int &value2change, int error_voltage)
{
    int temp_volt_min;
    int temp_volt_minBT;

    boolean _resume = false;
    while (_resume == false)
    {
        if (Serial.available() > 0)
        {
            String inByte = Serial.readStringUntil('\n');
            int temp_volt = inByte.toInt();

            if (inByte.toInt() == -2)
            {
                value2change = temp_volt_min;
                _resume = true;
            }
            else if (inByte.toInt() == -1)
            {
                _resume = true;
            }
            else if (inByte.toInt() >= 0 && inByte.toInt() <= 255)
            {
                temp_volt_min = temp_volt;

                dacWrite(DAC1, temp_volt); //sending voltage to break and look at TV/Monitor to find min Break

                Serial.println("Temp Volt bit/Volt: ");
                Serial.print(temp_volt);
                Serial.print("/");
                Serial.print(temp_volt * ref_voltage);
                Serial.println("");

                SerialBT.println("Temp Volt bit/Volt: ");
                SerialBT.print(temp_volt);
                SerialBT.print("/");
                SerialBT.print(temp_volt * ref_voltage);
                SerialBT.println("");
            }
            else if (inByte.toInt() < 0 || inByte.toInt() > 255)
            {
                temp_volt = error_voltage;
            }
        }
        else if (SerialBT.available())
        {
            String temp = SerialBT.readString();
            int temp_voltBT = temp.toInt();

            if (temp_voltBT == -2)
            {
                value2change = temp_volt_minBT;
                _resume = true;
            }
            else if (temp_voltBT == -1)
            {
                _resume = true;
            }
            else if (temp_voltBT >= 0 && temp_voltBT <= 255)
            {
                temp_volt_minBT = temp_voltBT;

                dacWrite(DAC1, temp_voltBT); //sending voltage to break and look at TV/Monitor to find min Break

                Serial.println("Temp VoltBT bit/Volt: ");
                Serial.print(temp_voltBT);
                Serial.print("/");
                Serial.print(temp_voltBT * ref_voltage);
                Serial.println("");

                SerialBT.println("Temp VoltBT bit/Volt: ");
                SerialBT.print(temp_voltBT);
                SerialBT.print("/");
                SerialBT.print(temp_voltBT * ref_voltage);
                SerialBT.println("");
            }
            else if (temp_voltBT < 0 || temp_voltBT > 255)
            {
                temp_voltBT = error_voltage;
            }
        }
    }
}

void voltage_cali()
{
    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

    Serial.println("***");
    Serial.println("Current MAX Break Volt bit/Volt: ");
    Serial.print(max_break_volt);
    Serial.print("/");
    Serial.println(max_break_volt * ref_voltage);
    Serial.println("Start calibration volt to get 100% breaking:");
    Serial.println("Adjust the voltage up or down to get max breaking (100%)");
    Serial.println("Values are given in a range between 0 and 255, the voltage Value will be displayed");
    Serial.println("Serial Command is '-2' for good or '-1' not to change this value");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.println("Current MAX Break Volt bit/Volt: ");
    SerialBT.print(max_break_volt);
    SerialBT.print("/");
    SerialBT.println(max_break_volt * ref_voltage);
    SerialBT.println("Start cali MAX Break volt for 100% breaking:");
    SerialBT.println("Adjust the volt to max breaking (100%)");
    SerialBT.println("Values range 0 to 255");
    SerialBT.println("Corr. Volt will be displayed");
    SerialBT.println("'-2' for ok and '-1' not to change");
    SerialBT.println("***");

    calicalc_volt(max_break_volt, 200);
    dacWrite(DAC1, 0); //Turn off voltage
    Serial.flush();    //clean buffer
    SerialBT.flush();  //clean buffer

    Serial.println("***");
    Serial.println("Current MIN Break Volt bit/Volt: ");
    Serial.print(min_break_volt);
    Serial.print("/");
    Serial.println(min_break_volt * ref_voltage);
    Serial.println("Start calibration MIN Break voltate to get 0% breaking:");
    Serial.println("Adjust the voltage up or down to get min breaking (100%)");
    Serial.println("Values are given in a range between 0 and 255, the voltage Value will be displayed");
    Serial.println("Serial Command is '-2' for good or '-1' not to change this value");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.println("Current MIN Break Volt bit/Volt: ");
    SerialBT.print(min_break_volt);
    SerialBT.print("/");
    SerialBT.println(min_break_volt * ref_voltage);
    SerialBT.println("Start cali MIN Break volt for 0% breaking:");
    SerialBT.println("Adjust the volt to MIN breaking (0%)");
    SerialBT.println("Values range 0 to 255");
    SerialBT.println("Corr. Volt will be displayed");
    SerialBT.println("'-2' for ok and '-1' not to change");
    SerialBT.println("***");

    calicalc_volt(min_break_volt, 100);
    dacWrite(DAC1, 0); //Turn off voltage

    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

    Serial.println("");
    Serial.print("Max Break Voltage: ");
    Serial.print(max_break_volt);
    Serial.print("/");
    Serial.print(max_break_volt * ref_voltage);
    Serial.println("");

    Serial.println("");
    Serial.print("Min Break Voltage: ");
    Serial.print(min_break_volt);
    Serial.print("/");
    Serial.print(min_break_volt * ref_voltage);
    Serial.println("");

    Serial.println("");
    Serial.print("To save new Data in EEPROM hit 'y' or not save 'n'");
    Serial.println("");

    SerialBT.println("");
    SerialBT.print("Max Break Voltage: ");
    SerialBT.print(max_break_volt);
    SerialBT.print("/");
    SerialBT.print(max_break_volt * ref_voltage);
    SerialBT.println("");

    SerialBT.println("");
    SerialBT.print("Min Break Voltage: ");
    SerialBT.print(min_break_volt);
    SerialBT.print("/");
    SerialBT.print(min_break_volt * ref_voltage);
    SerialBT.println("");

    SerialBT.println("");
    SerialBT.print("Save Data EEPROM: 'y' or not 'n'");
    SerialBT.println("");

    boolean _resume = false;
    while (_resume == false)
    {
        if (Serial.available() > 0 || SerialBT.available())
        {
            char inByte = Serial.read();
            char BTByte = SerialBT.read();
            if (inByte == 'y' || BTByte == 'y')
            {
#if defined(ESP8266) || defined(ESP32)
                EEPROM.begin(512);
#endif
                int temp_adress = 0;
                temp_adress += sizeof(newCalibrationValue);
                temp_adress += sizeof(max_break);
                temp_adress += sizeof(min_break);

                temp_adress += sizeof(max_break_redfac);
                EEPROM.put(temp_adress, max_break_volt); //get also max voltage
                temp_adress += sizeof(max_break_volt);
                EEPROM.put(temp_adress, min_break_volt); //get also min voltage

#if defined(ESP8266) || defined(ESP32)
                EEPROM.commit();
#endif

                Serial.println("Saved to EEPROM");
                Serial.println("");

                SerialBT.println("Saved to EEPROM");
                SerialBT.println("");
                _resume = true;
            }
            else if (inByte == 'n' || BTByte == 'n')
            {
                _resume = true;
            }
        }
    }

    Serial.println("End voltage calibration");
    Serial.println("***");
    Serial.println("To re-calibrate/calibrate, send 'v' from serial monitor.");
    Serial.println("***");

    SerialBT.println("End volt cali");
    SerialBT.println("***");

    delay(10000);
}

void weight_reference_calibration_first_time()
{
    float newCalibrationValue = 0.00f;
    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

    Serial.println("***");
    Serial.println("Start calibration of the reference weight:");
    Serial.println("This should just be done one time as calibration of a reference point to messure in Kg later:");
    Serial.println("Place the load cell an a level stable surface.");
    Serial.println("Remove any load applied to the load cell.");
    Serial.println("Send 't' from serial monitor to set the tare offset.");

    SerialBT.println("***");
    SerialBT.println("Start cali: reference weight:");
    SerialBT.println("This should just be done one time ");
    SerialBT.println("or first time. Is a ref. point ");
    SerialBT.println("to messure in Kg later:");
    SerialBT.println("Remove any load applied to the LC.");
    SerialBT.println("Send 't' for tare offset.");

    boolean _resume = false;
    while (_resume == false)
    {
        LoadCell.update();
        if (Serial.available() > 0 || SerialBT.available())
        {
            if (Serial.available() > 0 || SerialBT.available())
            {
                char inByte = Serial.read();
                char BTByte = SerialBT.read();
                if (inByte == 't' || BTByte == 't')
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
    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

    Serial.println("Now, place your known mass on the loadcell.");
    Serial.println("Then send the weight of this mass in GRAM from serial monitor. (example 1234 = 1.234kg)");
    Serial.println("or -1 without changes");

    SerialBT.println("Place ref laod on the LC.");
    SerialBT.println("Give the ref load into GRAM");
    SerialBT.println("example 1.234kg = 1234");
    SerialBT.println("or -1 without changes");

    float known_mass = 0;
    float known_massBT = 0;
    _resume = false;
    while (_resume == false)
    {
        LoadCell.update();
        if (Serial.available() > 0 || SerialBT.available())
        {
            known_mass = Serial.parseFloat();
            known_massBT = SerialBT.parseFloat();
            if (known_mass > 0)
            {
                Serial.print("Known mass is: ");
                Serial.println(known_mass);

                SerialBT.print("Known mass is: ");
                SerialBT.println(known_mass);
                _resume = true;
            }
            else if (known_mass == -1)
            {
                _resume = true;
            }

            if (known_massBT > 0)
            {
                Serial.print("Known mass is: ");
                Serial.println(known_massBT);

                SerialBT.print("Known mass is: ");
                SerialBT.println(known_massBT);
                known_mass = known_massBT;
                _resume = true;
            }
            else if (known_massBT == -1)
            {
                _resume = true;
            }
        }
    }

    LoadCell.refreshDataSet();

    if (known_mass > 0)
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

        SerialBT.print("New calibration value: ");
        SerialBT.print(newCalibrationValue);
        SerialBT.print("Save value to EEPROM: ");
        SerialBT.print(calVal_eepromAdress);
        SerialBT.println("? y/n");

        _resume = false;
        while (_resume == false)
        {
            if (Serial.available() > 0 || SerialBT.available())
            {
                char inByte = Serial.read();
                char BTByte = SerialBT.read();
                if (inByte == 'y' || BTByte == 'y')
                {
                    Serial.print("");
                    Serial.println("You are realy sure, since this will change the referece point for displaying right Kg");
                    Serial.print("Will not affect the calulation of max and min break but the values are perhaps not right in Kg");
                    Serial.print("But relative to max and min will be coorect, is just for the 'optic'");
                    Serial.print("Type in 'y' to save into EEPROM or 'n' for no saving into EEPROM");
                    Serial.print("");

                    SerialBT.print("");
                    SerialBT.println("You are realy sure");
                    Serial.print("Save to EEPROM? y/n");
                    SerialBT.print("");

                    while (_resume == false)
                    {

                        if (Serial.available() > 0 || SerialBT.available())
                        {
                            char inByte = Serial.read();
                            char BTByte = SerialBT.read();
                            if (inByte == 'y' || BTByte == 'y')
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
                else if (inByte == 'n' || BTByte == 'n')
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

    bool volt_direction_normal = false; // break 0-100% voltage low-hight = true
    float reduces_break;
    float GLED;

    //check if break 0% to 100% also the voltage goes in same direction from low-hight or hight to low
    if (max_break_volt > min_break_volt)
    {
        volt_direction_normal = true;
    }

    // check for new data/start next conversion:
    if (LoadCell.update())
        newDataReady = true;

    // get smoothed value from the dataset:
    if (newDataReady)
    {
        float i = LoadCell.getData();
        float j = i;
        count++;

        if (j > max_break)
            j = max_break;
        if (j < 0.00)
            j = 0.00;

        //if max_break more then max_break it cant be more then max_break
        float gi = j;

        //if max break to much, reduce not input side but output side adjust the voltage

        if (volt_direction_normal == true)
        {
            reduces_break = (float)max_break_volt * (max_break_redfac / 100.0); // example max_break_volt*0.80
        }
        else
        {
            reduces_break = (float)max_break_volt * ((100.0 + (100.0 - max_break_redfac)) / 100.0); // example max_break_volt*1.20
        }

        if (gi >= min_break)
        {
            GLED = mapping(gi, min_break, max_break, min_break_volt, reduces_break); //LED need approx 160 as min value to shine

            //give the needed voltage to given load=break force
            dacWrite(DAC1, round(GLED));
        }
        else
        {
            float min_break_range;
            float mbr=15.0;
            if (volt_direction_normal == true)
            {
                min_break_range = ((float)min_break_volt / 255.0) * 3.3 - (ref_voltage * mbr); //0.19Volt less
                GLED = mapping(min_break_range, 0.0, 3.3, 0.0, 255.0);
                if (min_break_range > 0.0)
                    min_break_range = 0.0;
            }
            else
            {
                min_break_range = ((float)min_break_volt / 255.0) * 3.3 + (ref_voltage * mbr); //0.19Volt more
                GLED = mapping(min_break_range, 0.0, 3.3, 0.0, 255.0);

                if (min_break_range > 255.0)
                    min_break_range = 255.0;
            }       
            dacWrite(DAC1, round(GLED));
        }

        if (millis() > t + serialPrintInterval)
        {
            if (SerialPrintData == 1)
            {
                Serial.print(count);
                Serial.print(" LC output Kg: ");
                Serial.print(i / kg_factor);
                Serial.print(" dev2tara%: ");
                Serial.print((i * 100.0) / max_break);

                Serial.print("  GLED: ");
                Serial.print(round(GLED));
                Serial.print("/");
                Serial.print((GLED / 255.0) * 3.30);
                Serial.print(" Volt");

                Serial.print("  MaxB: ");
                Serial.print(max_break / kg_factor);

                Serial.print("  Max Break RedFac: ");
                Serial.print(max_break_redfac);

                Serial.print("  MinB: ");
                Serial.print(min_break / kg_factor);
                Serial.println("");

                //BT
                SerialBT.print(count);
                SerialBT.print(" LC output Kg: ");
                SerialBT.println(i / kg_factor);
                SerialBT.print("dev2tara%: ");
                SerialBT.println((i * 100.0) / max_break);

                SerialBT.print("DAC1: ");
                SerialBT.print(round(GLED));
                SerialBT.print("/");
                SerialBT.print((GLED / 255.0) * 3.30);
                SerialBT.println("V");

                SerialBT.print("MaxB: ");
                SerialBT.println(max_break / kg_factor);

                SerialBT.print("Max Break RedFac: ");
                SerialBT.println(max_break_redfac);

                SerialBT.print("MinB: ");
                SerialBT.print(min_break / kg_factor);
                SerialBT.println("");
            }

            t = millis();
        }
        newDataReady = 0;
    }

    // receive command from serial terminal
    if (Serial.available() > 0)
    {
        String inByte = Serial.readStringUntil('\n');
        if (inByte == "t")
            tara_load_cell(); //tare
        else if (inByte == "c")
            calibrate(); //break calibrate
        else if (inByte == "v")
            voltage_cali(); //voltage calibrate
        else if (inByte == "b")
            load_variables_eeprom(); //read out memory
        else if (inByte == "www")
            weight_reference_calibration_first_time(); //calibrate basis weight
        else if (inByte == "s")
            SDPrint(); //calibrate basis weight
    }

    if (SerialBT.available())
    {
        char BTByte = SerialBT.read();
        if (BTByte == 't')
            tara_load_cell(); //tare
        else if (BTByte == 'c')
            calibrate(); //break calibrate
        else if (BTByte == 'v')
            voltage_cali(); //voltage calibrate
        else if (BTByte == 'b')
            load_variables_eeprom(); //read out memory
        else if (BTByte == 'w')
            weight_reference_calibration_first_time(); //calibrate basis weight
        else if (BTByte == 's')
            SDPrint(); //calibrate basis weight
    }
}
