# Logitech-G29-Load-Cell

I will not repeat text what you can find in the "References" and keep the story short:

My ideas to develop this program was due to the inspiration and using the ideas of References:

# References
1) https://github.com/GeekyDeaks/g29-load-cell (original idea did come from here)
2) https://github.com/Skidude88/Skidude88-G29-PS4-LoadCell-Arduino/wiki (original idea did come from here)
3) https://github.com/olkal/HX711_ADC (library for the HX711 I did use)
4) https://github.com/iforce2d/thrustTester (to get the default setup of the HX711 from 10 to 80 Hz, my HX711 did end up with 89 Hz)
5) https://circuitjournal.com/50kg-load-cells-with-HX711 (wiring with 2 cells)
6) https://www.youtube.com/watch?v=iywsJB-T-mU (at 7:15, due to ESP32 speed)
7) https://www.youtube.com/watch?v=99u9cy7Mnl4 (for mod the rubber to simulate more like a load cell)
8) Google Store: Serial Bluetooth Terminal from Kai Morich
9) Gamma Calculation: https://www.desmos.com/calculator/swnathc6og

and I will not refer it more in the text, since it is a mix overall and all of them above did contribute to my result.

The reason for this is that the G29 use pot.meter from 0-100% break over distance => give the output voltage to regulate corresponding break %
The force feedback is more or less linear, at the end we touch a rubber what will simulate a higher pressure but anyway the muscle memory is difficult
and constant good breaking especially in trail breaking is difficult. With a load cell we are simulating closer to real hydraulic breaks in real life
where we have overpromotion force to push to reach higher break.

Then the g29 and PS4 do not give any possibilities to trim the break to your driving style and this was also one of my reason to have a micro controller so that I could setup the break after my conditions.

# Shopping List:
1) Load Cells with HX711 
2) Micro controller 
3) Enclosing for the Micro Controller/HX711 and for the Load Cells
4) Resistors and cables

# How-to and why

To use PWM signal to generate the needed voltage range was not good if not using a low filter or a DAC. Since I am using ESP32 anyway to other Arduino projects and for IoT and they are cheap and can even over Amazon go under 6 Euro (included delivery) is a very unexpensive MC.
The advantage:
1) rel. fast 2xCPU with 32 bit
2) 2 x8 bit DAC (range 0-255)
3) Bluetooth for serial com. for changing parameters in the setup/trimming the break
4) Voltage range 0-3.3V fits anyway to the G29 what also is in this range

That was the reason to use ESP32 and as I was work with the project I wanted to "simulate" a "PWM" with my DAC between 2 values and here the multitask function of the ESP32 was perfect, letting the main program calculate the values for breaking and if a needed value was between, let’s say 231 and 232 I would let the DAC jump between 231 and 232 in an constant loop (of course calculated how many times of 231 and 232 to get the right simulated value let say 231.12) so in this case the task would not have any "delay" but just looping and looping with the latest known values until the task did get new values to loop. Fantastic the ESP32 :)

Another issue known is that the G29 let’s say your breaking at 80% distance on the break is not equal to 80% in your PS4. G29 have an over exponential curve and I had to find out the values so that I could do a linearization of the curve (also my references are referring about this issue). After finding 0% break and 100% I had the outer points. So, 50% must be between, well was not as explained above. How find 50%, I did find the bit value what was correspond to 50% in the game. Let’s say 0% was 210 and 100% 140 => (140+210)/2 = 175 in the DAC as bit but was not. So, you then change the bit value until you get the 50% in the game. So, the work was now to check for many different % between 0 and 100% after this I could make a load array, you could also use a function but is quicker to use points in an array and calculate the output.

Then I wanted to have a gamma factor to adjust the bias of the curve to fit my way of breaking. 
G>1.0 means that the break% is increasing over proportional, G<1.0 the opposite. G = 1 linear

Then the break needed a dead zone, means your break don’t start at zero Kg but at example 0.2 Kg.
Then the break needed a max load = 100% break, so if you hit the break more than max load = 100% break
Then I did read in IRacing the max load = 100% could be reduced to a %, lets say 80%, means if you break at max load = 100% (normally) it would be 80%, so I also implemented this (never know if I will do IRacing but anyway good to have)
Then I wanted to send on the air serial commands to the ESP32 to change parameter for my breaking if trimming was needed.
Then of course encloser for the brake system and for the ESP32 and HX711 (see 3D files)

That is all in a nus shell.

Im an engineer and not a programmer so could be that my programming is dirty so I do not mind if somebody would pimp it up, but it works and it works like a charm.

# Circuit/Wiring















