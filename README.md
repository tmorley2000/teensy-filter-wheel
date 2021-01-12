# Simple filter wheel driver firmware

Firmware for a teensy-2.0 with a sd8825 stepper driver.

Uses AccelStepper library to gove smooth acceleration.

Pins used:

|Arduino Name|AVR Port | Use|
|-------|---|---|
|0| PB0 | Dir|
|1| PB1 | Step|
|5| PD0 |home sensor in|
|2| PB2 |Enable|
|19| PF4 |MS1|
|20| PF1 |MS2|
|21| PF0 |MS3|
| Serial 1 / Pin7 | | Debug serial out 115200 |
| LED/Pin11 | | Blinking light to show its doing something! |

Pins on 9-way d type stepper connection for PBs filter wheel

| Pins | Use |
| -- | -- |
|1+6 |motor B|
|5+9 |motor A|
|2+4 |home pos sensor|

# Useful links

Optec filter protocol? 
 - http://www.optecinc.com/astronomy/catalog/ifw/images/17350_manual.pdf
 - http://www.optecinc.com/astronomy/catalog/ifw/resources/ifw_technical_manual.pdf
 - Maxim doesn't seem to use much of it!

Stepper on wheel
 - Portescap 42M100B1B 5Vdc 12.5Ohm 3.6deg
 - 5v 12.5Ohm -> 0.4A

Pololu stepper driver
 - 'refernce' https://www.pololu.com/product/1182
 - Actually its a http://www.panucatt.com/product_p/sd8825.htm
 - Datasheet http://files.panucatt.com/datasheets/sd8825_user_guide.pdf

Teensy refernce pages
 - https://www.pjrc.com/teensy/td_libs_AccelStepper.html
 - http://www.airspayce.com/mikem/arduino/AccelStepper/index.html
 - https://www.pjrc.com/teensy/td_libs_EEPROM.html
 - https://www.pjrc.com/teensy/td_serial.html
 - https://www.pjrc.com/teensy/td_digital.html
