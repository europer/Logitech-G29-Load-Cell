# Logitech-G29-Load-Cell


My ideas to develop this program was due to the inspiration and using the ideas of References:

1) https://github.com/GeekyDeaks/g29-load-cell (orignal idea did come from here)
2) https://github.com/Skidude88/Skidude88-G29-PS4-LoadCell-Arduino/wiki (orignal idea did come from here)
3) https://github.com/olkal/HX711_ADC (libary for the HX711 I did use)
4) https://github.com/iforce2d/thrustTester (to get the defalt setup of the HX711 from 10 to 80 Hz, strage my HX711 did end up with 89 Hz)
5) https://circuitjournal.com/50kg-load-cells-with-HX711 (wiring with 2 cells)
6) https://www.youtube.com/watch?v=iywsJB-T-mU (at 7:15, due to ESP32 speed)
7) https://www.youtube.com/watch?v=99u9cy7Mnl4 (for mod the rubber to simulate more like a load cell)

and I will not refere it more in the text, since it is a mix overall and all of them above did contribut to my result.

The reson for this is that the G29 use pot.meter from 0-100% break over distance => give the output voltage to regulate coresponding break %
The force feedback is more or less linear, at the end we touch a rubber what will simulate a higher pressure but anyway the muscle memory is difficult
and constant good breaking espesally in trail breaking is difficult. With an load cell we are simulating more close to real hydralic breaks in real life
where we have overporosional force to push to reach higher break.

Then the g29 and PS4 do not give any possibilites to trim the break to your driving style and this was alos one of my reason to have an micro controller so that I could setup the break after my condisitons.

What whas needed:
1) Load Cells with HX711
2) Micro controller
3) Enclosering for the Micro Controller/HX711 and for the Load Cells
4) Resistors and cables









