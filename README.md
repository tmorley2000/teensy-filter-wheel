# Simple filter wheel driver firmware

Firmware for a teensy-2.0 with a sd8825 stepper driver.

Uses AccelStepper library to gove smooth acceleration.

Pins used:
|Arduino Name|AVR Port | Use|
|0| PB0 | Dir|
|1| PB1 | Step|
|5| PD0 |home sensor in|
|2| PB2 |Enable|
|19| PF4 |MS1|
|20| PF1 |MS2|
|21| PF0 |MS3|
| Serial 1 / Pin7 | | Debug serial out 115200 |
| LED/Pin11 | | Blinking light to show its doing something! |


