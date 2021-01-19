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
#define DAC1 25 // Break Voltage Out to PS4/PC

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
break 0% - 2.86v = 221 
break 100% - 1.93v = 149
Assetto Corsa Comp
break 0% - 2.88v = 222-223
break 100% - 1.48v = 114
*/
int min_break_volt = 221;
int max_break_volt = 149;

float esp32Volt = 3.3;

float ref_voltage = (1.0 / 255.0) * esp32Volt;

float newCalibrationValue = 1.23f;

float kg_factor = 1000.0f;

int SerialPrintData = 0; //no Serial Printout of Data

bool reboot_esp32 = false;

bool normal = true;
float gammafac = 1.0; //linear
int laod_percent_steps = 79;
float load_percent[79] = {1.0, 0.963, 0.93, 0.895, 0.86, 0.835, 0.804, 0.776, 0.749, 0.725,
                          0.701, 0.68, 0.66, 0.642, 0.625, 0.609, 0.595, 0.58, 0.564, 0.55,
                          0.535, 0.525, 0.512, 0.498, 0.485, 0.475, 0.462, 0.455, 0.447, 0.439,
                          0.431, 0.423, 0.416, 0.408, 0.4, 0.392, 0.384, 0.377, 0.369, 0.361,
                          0.353, 0.345, 0.337, 0.33, 0.322, 0.314, 0.306, 0.298, 0.291, 0.283,
                          0.275, 0.267, 0.259, 0.252, 0.247, 0.236, 0.228, 0.220, 0.212, 0.205,
                          0.197, 0.189, 0.181, 0.173, 0.166, 0.158, 0.15, 0.142, 0.134, 0.127,
                          0.116, 0.105, 0.094, 0.078, 0.063, 0.047, 0.031, 0.016, 0.0};

void bitcheckint(int &value2check)
{
    if (value2check < 0)
        value2check = 0;
    if (value2check > 255)
        value2check = 255;
}

void bitcheckfloat(float &value2check)
{
    if (value2check < 0.0)
        value2check = 0.0;
    if (value2check > 255.0)
        value2check = 255.0;
}

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

void reboot_mc()
{
    reboot_esp32 = true;
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

void load_variables_eeprom(int ja_nein)
{

    if (ja_nein == 1) //from EEPROM = 1, and from RAM = 0
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
    }

    Serial.println("");
    Serial.print("Max break Kg: ");
    Serial.print(max_break / kg_factor);

    Serial.println("");
    Serial.print("Min break RedFac %: ");
    Serial.print(max_break_redfac);

    Serial.println("");
    Serial.print("Min break Kg: ");
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

int calicalulation_break(float &value2change, float value2compare, int flag2compare, int calc_type)
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
                    return -1;
                }
                else if (temp_redfac <= 100.0f && temp_redfac > 0.0f)
                {
                    value2change = temp_redfac;
                    _resume = true;
                    return 0;
                }

                if (temp_redfacBT == -1)
                {
                    _resume = true;
                    return -1;
                }
                else if (temp_redfacBT <= 100.0f && temp_redfacBT > 0.0f)
                {
                    value2change = temp_redfacBT;
                    _resume = true;
                    return 0;
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
    Serial.print("Max break Kg: ");
    Serial.print(max_break / kg_factor);
    Serial.println("");

    Serial.println("");
    Serial.print("Max break Reduce Factor: ");
    Serial.print(max_break_redfac);
    Serial.println("");

    Serial.println("");
    Serial.print("Min break Kg: ");
    Serial.print(min_break / kg_factor);
    Serial.println("");

    Serial.println("");
    Serial.print("To save new Data in EEPROM hit 'y' or not save 'n'");
    Serial.println("");

    SerialBT.println("");
    SerialBT.print("Max break Kg: ");
    SerialBT.print(max_break / kg_factor);

    SerialBT.println("");
    SerialBT.print("Max break Reduce Factor: ");
    SerialBT.print(max_break_redfac);

    SerialBT.println("");
    SerialBT.print("Min break Kg: ");
    SerialBT.print(min_break / kg_factor);

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

void change_breake_load_values()
{
    int cblv_return = 0;

    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

    //refresh the dataset to be sure that the known mass is measured correct
    LoadCell.refreshDataSet();

    Serial.println("***");
    Serial.print("NOW: MAX break Kg: ");
    Serial.println(max_break / kg_factor);
    Serial.println("NEW value in Kg (Example: 15.23)");
    Serial.println("or -1 without changes");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.print("NOW: MAX break Kg: ");
    SerialBT.println(max_break / kg_factor);
    SerialBT.println("NEW value in Kg (Example: 15.23)");
    SerialBT.println("or -1 without changes");
    SerialBT.println("***");

    //max_break_redfac, no value to compare to, and flag not to compare, case 2
    cblv_return = calicalulation_break(max_break, 0, 0, 2);
    if (cblv_return != -1)
        max_break = max_break * kg_factor;

    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

    Serial.println("***");
    Serial.print("NOW: MIN break Kg: ");
    Serial.println(min_break / kg_factor);
    Serial.println("NEW value in Kg (Example: 4.12)");
    Serial.println("or -1 without changes");
    Serial.println("***");

    SerialBT.println("***");
    SerialBT.print("NOW: MIN break Kg: ");
    SerialBT.println(min_break / kg_factor);
    SerialBT.println("NEW value in Kg (Example: 4.12)");
    SerialBT.println("or -1 without changes");
    SerialBT.println("***");

    //max_break_redfac, no value to compare to, and flag not to compare, case 2
    calicalulation_break(min_break, 0, 0, 2);
    if (cblv_return != -1)
        min_break = min_break * kg_factor;

    Serial.flush();   //clean buffer
    SerialBT.flush(); //clean buffer

    if (min_break < 0.8 * max_break)
    {

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
        Serial.print("Max break Kg: ");
        Serial.print(max_break / kg_factor);
        Serial.println("");

        Serial.println("");
        Serial.print("Max break Reduce Factor: ");
        Serial.print(max_break_redfac);
        Serial.println("");

        Serial.println("");
        Serial.print("Min break Kg: ");
        Serial.print(min_break / kg_factor);
        Serial.println("");

        Serial.println("");
        Serial.print("To save new Data in EEPROM hit 'y' or not save 'n'");
        Serial.println("");

        SerialBT.println("");
        SerialBT.print("Max break Kg: ");
        SerialBT.print(max_break / kg_factor);

        SerialBT.println("");
        SerialBT.print("Max break Reduce Factor: ");
        SerialBT.print(max_break_redfac);

        SerialBT.println("");
        SerialBT.print("Min break Kg: ");
        SerialBT.print(min_break / kg_factor);

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
    else
    {
        Serial.println("");
        Serial.println("Termination of Load Changes");
        Serial.println("Something did go wrong!!!");
        Serial.println("Wrong values? Try again!!!");
        Serial.println("");

        SerialBT.println("");
        SerialBT.println("Termination of Load Changes");
        SerialBT.println("Something did go wrong!!!");
        SerialBT.println("Wrong values? Try again!!!");
        SerialBT.println("");
    }
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

    calicalc_volt(max_break_volt, 100);
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

    calicalc_volt(min_break_volt, 200);
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

    delay(3000);
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

    delay(3000);
}

void SerialPrintOutCollector(int lc_out, float mapped_voltage, bool nmal, float gfac, float LowerBitCase, int outputtype)
{
    if (outputtype == 1)
    {
        Serial.print(count);
        Serial.print(" LC output Kg: ");
        Serial.print(lc_out / kg_factor);
        Serial.print(" dev2tara%: ");
        Serial.print((lc_out * 100.0) / max_break);

        Serial.print("  DAC1: ");
        Serial.print(round(mapped_voltage));
        Serial.print("/");
        Serial.print((mapped_voltage / 255.0) * 3.30);
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
        SerialBT.println(lc_out / kg_factor);
        SerialBT.print("dev2tara%: ");
        SerialBT.println((lc_out * 100.0) / max_break);

        SerialBT.print("DAC1: ");
        SerialBT.print(round(mapped_voltage));
        SerialBT.print("/");
        SerialBT.print((mapped_voltage / 255.0) * 3.30);
        SerialBT.println("V");

        SerialBT.print("MaxB: ");
        SerialBT.println(max_break / kg_factor);

        SerialBT.print("Max Break RedFac: ");
        SerialBT.println(max_break_redfac);

        SerialBT.print("MinB: ");
        SerialBT.print(min_break / kg_factor);
        SerialBT.println("");
    }
    else if (outputtype == 2)
    {
        Serial.print(count);
        Serial.print(" LC output Kg: ");
        Serial.print(lc_out / kg_factor);
        Serial.print(" dev2tara%: ");
        Serial.print((lc_out * 100.0) / max_break);

        Serial.print("  Nor: ");
        Serial.print(nmal);

        Serial.print("  Gamma: ");
        Serial.print(gfac);

        Serial.print("  DAC1: ");
        Serial.print(LowerBitCase);
        Serial.print("/");
        Serial.print((LowerBitCase / 255.0) * 3.30);
        Serial.print(" Volt");

        Serial.print("  MaxB: ");
        Serial.print(max_break / kg_factor);

        Serial.print("  MinB: ");
        Serial.print(min_break / kg_factor);
        Serial.println("");

        //BT
        SerialBT.print(count);
        SerialBT.print(" LC output Kg: ");
        SerialBT.println(lc_out / kg_factor);
        SerialBT.print("dev2tara%: ");
        SerialBT.println((lc_out * 100.0) / max_break);

        SerialBT.print("Nor: ");
        SerialBT.println(nmal);

        SerialBT.print("Gamma: ");
        SerialBT.println(gfac);

        Serial.print("DAC1: ");
        Serial.print(LowerBitCase);
        Serial.print("/");
        Serial.print((LowerBitCase / 255.0) * 3.30);
        Serial.println(" V");

        SerialBT.print("MaxB: ");
        SerialBT.println(max_break / kg_factor);

        SerialBT.print("MinB: ");
        SerialBT.print(min_break / kg_factor);
        SerialBT.println("");
    }
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
    delay(2000);

    load_variables_eeprom(1); //load in the variables
    delay(3000);
}

void loop()
{
    static boolean newDataReady = 0;
    const int serialPrintInterval = 1000; //increase value to slow down serial print activity
    float loadcellraw;
    float loadcellcleaned;

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
        float loadcellraw = LoadCell.getData();
        loadcellcleaned = loadcellraw;
        count++;

        //if max_break more then max_break it cant be more then max_break
        if (loadcellcleaned > max_break)
            loadcellcleaned = max_break;
        if (loadcellcleaned < 0.00)
            loadcellcleaned = 0.00;

        if (normal == false)
        {
            //if max break to much, reduce not input side but output side adjust the voltage

            if (volt_direction_normal == true)
            {
                reduces_break = (float)max_break_volt * (max_break_redfac / 100.0); // example max_break_volt*0.80
            }
            else
            {
                reduces_break = (float)max_break_volt * ((100.0 + (100.0 - max_break_redfac)) / 100.0); // example max_break_volt*1.20
            }

            if (loadcellcleaned >= min_break)
            {
                GLED = mapping(loadcellcleaned, min_break, max_break, min_break_volt, reduces_break); //LED need approx 160 as min value to shine

                //give the needed voltage to given load=break force
                dacWrite(DAC1, round(GLED));
            }
            else
            {
                float min_break_range;
                float mbr = 15.0;
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
                    SerialPrintOutCollector(loadcellraw, GLED, 0, 0, 0, 1);
                }

                t = millis();
            }
        }
        else
        {
            ////////////////// new from 29.12.2020 for the linearization
            float weight_in_percent = mapping(loadcellraw, min_break, max_break, 0.0, 1.0); //mapping into %

            //gamma>2.0 means break at 50% is now  71% (faster break curve)
            //gamma<0.5 means break at 50% is now just 25%  (slower break curve)
            if (gammafac != 1.0) //1.0 linear no change needed
            {
                weight_in_percent = pow(weight_in_percent, (1.0 / gammafac)); //change curve after gamma factor
            }

            int lower_band_case;  //acctual case N from load_percent[N] array value // same as load_case just one after
            int upper_band_case;  // same as load_case just one before
            float lower_band_per; // same as load_case but the value from the array load_percent[] after
            float upper_band_per; // same as load_case but the value from the array load_percent[] before

            if (weight_in_percent != 0.0 && weight_in_percent != 1.0) // for 0 or 1 we do not need to check
            {
                for (int n = (laod_percent_steps - 1); n >= 0; n--) // n=0 =>100%
                {
                    if (weight_in_percent > load_percent[n])
                    {
                        lower_band_case = n;     //take the hightest value until the next load_percent(n-1) is bigger then weight_in_percent
                        upper_band_case = n - 1; //would be the next value but is bigger then the weight_in_percent
                    }
                    if (weight_in_percent <= load_percent[n])
                    {
                        lower_band_per = load_percent[lower_band_case];
                        if (upper_band_case < 0) //min value in array is 0
                        {
                            upper_band_per = load_percent[lower_band_case];
                        }
                        else
                        {
                            upper_band_per = load_percent[upper_band_case];
                        }
                        break; // and when it is less we go out and have the load_case
                    }
                }
            }
            else if (weight_in_percent == 0.0 || weight_in_percent == 1.0) // for 0 or 1 we do not need to check
            {
                if (weight_in_percent == 0.0)
                {
                    lower_band_case = (laod_percent_steps - 1); //78 (0 to 78 = 79 steps)
                    upper_band_case = (laod_percent_steps - 1); //78
                    lower_band_per = load_percent[lower_band_case];
                    upper_band_per = load_percent[upper_band_case];
                }
                if (weight_in_percent == 1.0)
                {
                    lower_band_case = 0;                            //0
                    upper_band_case = 0;                            //0
                    lower_band_per = load_percent[lower_band_case]; //0
                    upper_band_per = load_percent[upper_band_case];
                }
            }

            float lower_weighted_percent; //the distance from value
            float upper_weighted_percent; //the distance from value
            float delta;                  // delta between lower and upper value
            float pwm = 10.0;             //max cylce or loops before next calulatuion
            int lower_pwm;                //how many DAC to use the lower bit number in a cylce
            int upper_pwm;                //how many DAC to use the upper bit number in a cylce

            delta = abs(upper_weighted_percent - lower_weighted_percent);
            if (delta > 0.0001) //no ZERO in deviding and use a bit higher value then zero not to get an huge value out
            {
                lower_weighted_percent = 1.0 - (abs(weight_in_percent - lower_band_per) / delta);
                upper_weighted_percent = 1.0 - (abs(weight_in_percent - upper_band_per) / delta);
                lower_pwm = int(pwm * lower_weighted_percent + 0.5); // example 80% gives 8 times PWM and use the int(value +0.5) to round correctly
                upper_pwm = int(pwm * upper_weighted_percent + 0.5); // example 20% give 2 times PWM
            }
            else
            {
                lower_weighted_percent = 0.0;
                upper_weighted_percent = 0.0;
                lower_pwm = 0; // example 0 times PWM
                upper_pwm = 0; // example 0 times PWM
            }
            //extra control on outer band points
            if (weight_in_percent == 0.0 || weight_in_percent == 1.0)
            {
                lower_weighted_percent = 0.0;
                upper_weighted_percent = 0.0;
                lower_pwm = 0; // example 0 times PWM
                upper_pwm = 0; // example 0 times PWM
            }

            int lower_bit_case;
            int upper_bit_case;
            if (weight_in_percent != 0.0 || weight_in_percent != 1.0)
            {
                //calulating cooresponing bit to load = linearisation of load to output => 50% load gives 50% break in PS4
                //and use the int(value +0.5) to round correctly
                int lower_bit_case = int(mapping(lower_band_case, (laod_percent_steps - 1), 0, min_break_volt, max_break_volt) + 0.5);
                int upper_bit_case = int(mapping(upper_band_case, (laod_percent_steps - 1), 0, min_break_volt, max_break_volt) + 0.5);
            }
            else
            {
                if (weight_in_percent == 0.0)
                {
                    lower_bit_case = min_break_volt;
                    upper_bit_case = min_break_volt;
                }
                if (weight_in_percent == 1.0)
                {
                    lower_bit_case = max_break_volt;
                    upper_bit_case = max_break_volt;
                }
            }

            //security check that we have right range 0 -255
            bitcheckint(lower_bit_case);
            bitcheckint(upper_bit_case);

            if (lower_bit_case == min_break_volt || lower_bit_case == max_break_volt) //max or min break
            {
                if (volt_direction_normal == true)
                {
                    if (lower_bit_case == min_break_volt) //min break
                    {
                        lower_bit_case = min_break_volt - 2; //give 2 bit less
                        upper_bit_case = min_break_volt - 2;
                        //security check that we have right range
                        bitcheckint(lower_bit_case);
                        bitcheckint(upper_bit_case);
                        dacWrite(DAC1, lower_bit_case);
                    }
                    else if (lower_bit_case == max_break_volt) //max break
                    {
                        lower_bit_case = max_break_volt + 2; //give 2 bit more
                        upper_bit_case = max_break_volt + 2;
                        //security check that we have right range
                        bitcheckint(lower_bit_case);
                        bitcheckint(upper_bit_case);
                        dacWrite(DAC1, lower_bit_case);
                    }
                }
                else
                {
                    if (lower_bit_case == min_break_volt) //min break
                    {
                        lower_bit_case = min_break_volt + 2; //give 2 bit more
                        upper_bit_case = min_break_volt + 2;
                        //security check that we have right range
                        bitcheckint(lower_bit_case);
                        bitcheckint(upper_bit_case);
                        dacWrite(DAC1, lower_bit_case);
                    }
                    else if (lower_bit_case == max_break_volt) //max break
                    {
                        lower_bit_case = max_break_volt - 2; //give 2 bit less
                        upper_bit_case = max_break_volt - 2;
                        //security check that we have right range
                        bitcheckint(lower_bit_case);
                        bitcheckint(upper_bit_case);
                        dacWrite(DAC1, lower_bit_case);
                    }
                }
            }
            else if (lower_pwm == 0 && upper_pwm > 0)
            {
                dacWrite(DAC1, upper_bit_case);
                       }
            else if (lower_pwm > 0 && upper_pwm == 0)
            {
                dacWrite(DAC1, lower_bit_case);
            }
            else if (lower_pwm > 0 && upper_pwm > 0) //PWM part of the DAC
            {
                int x;
                if (lower_pwm > upper_pwm)
                {
                    for (x = 1; x <= upper_pwm; x++)
                        dacWrite(DAC1, upper_bit_case);
                    for (x = 1; x <= lower_pwm; x++)
                        dacWrite(DAC1, lower_bit_case);
                }
                if (lower_pwm < upper_pwm)
                {
                    for (x = 1; x <= lower_pwm; x++)
                        dacWrite(DAC1, lower_bit_case);
                    for (x = 1; x <= upper_pwm; x++)
                        dacWrite(DAC1, upper_bit_case);
                }
                if (lower_pwm == upper_pwm)
                {
                    for (x = 1; x <= (lower_pwm + upper_pwm); x++)
                    {
                        if ((x % 2) == 0)
                            dacWrite(DAC1, upper_bit_case);
                        else
                            dacWrite(DAC1, lower_bit_case);
                    }
                }
            }

            if (millis() > t + serialPrintInterval)
            {
                if (SerialPrintData == 1)
                {
                    SerialPrintOutCollector(loadcellraw, GLED, normal, gammafac, lower_bit_case, 2);
                }

                t = millis();
            }
        }

        newDataReady = 0;
    }

    if (reboot_esp32 == true) //mqtt command to reboot esp32
    {
        Serial.println("");
        Serial.print("Reboot ESP32 in 2 sec.");
        Serial.println("");

        SerialBT.println("");
        SerialBT.print("Reboot ESP32 in 2 sec.");
        SerialBT.println("");

        delay(2000);          // 5 sec on display
        reboot_esp32 = false; //make no sence since it will reboot but anyway
        ESP.restart();
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
            load_variables_eeprom(1); //read out eeprom
        else if (inByte == "a")
            load_variables_eeprom(0); //read out RAM
        else if (inByte == "www")
            weight_reference_calibration_first_time(); //calibrate basis weight
        else if (inByte == "s")
            SDPrint(); //calibrate basis weight
        else if (inByte == "l")
            change_breake_load_values(); //calibrate basis weight
        else if (inByte == "r")
            reboot_mc(); //calibrate basis weight
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
            load_variables_eeprom(1); //read out eeprom
        else if (BTByte == 'a')
            load_variables_eeprom(0); //read out RAM
        else if (BTByte == 'w')
            weight_reference_calibration_first_time(); //calibrate basis weight
        else if (BTByte == 's')
            SDPrint(); //calibrate basis weight
        else if (BTByte == 'l')
            change_breake_load_values(); //calibrate basis weight
        else if (BTByte == 'r')
            reboot_mc(); //calibrate basis weight
    }
}
