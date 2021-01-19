char input1;
char input2[10];
int indexInput2 = 0;
boolean inputValue = false;
boolean negativo = false;
void readSerial()
{
    /*
  input data: *name:value\n
  */
    while (Serial.available() > 0)
    {
        char c = Serial.read();
        if (c == '*')
        {
            indexInput2 = 0;
            inputValue = false;
            negativo = false;
            //      Serial.println("starting input");
        }
        else
        {
            if (c == '\n')
            {
                input2[indexInput2] = '\0';
                //        Serial.print("value: ");
                //        Serial.println(input2);
                int point = 0, i = 0;
                float ris = 0;
                while (input2[i] != '\0')
                {
                    if (input2[i] == '.')
                    {
                        point = indexInput2 - i - 1;
                    }
                    else
                    {
                        ris = (ris * 10) + input2[i] - '0';
                    }
                    i++;
                }
                /*     
        Serial.print("ris: ");
        Serial.println(ris);
        Serial.print("point: ");
        Serial.println(point);
*/
                ris /= pow(10, point);
                if (negativo)
                {
                    ris *= -1;
                }
                executeCommand(input1, ris);
            }
            else
            {
                if (inputValue)
                {
                    if (indexInput2 == 0 && c == '-')
                    {
                        negativo = true;
                    }
                    else
                    {
                        input2[indexInput2] = c;
                        indexInput2++;
                    }
                }
                else
                {
                    if (c == ':')
                    {
                        //            Serial.println("command ok, waiting value");
                        inputValue = true;
                    }
                    else
                    {
                        input1 = c;
                    }
                }
            }
        }
    }
}