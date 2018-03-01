# usb_kbd

Example of reading USB Keyboard keycodes.
The raw key codes of pressed keys are printed to UART.

Tested on RPi Zero W with a USB hub.

USB driver is derived from
https://github.com/LdB-ECM/Raspberry-Pi/tree/master/Arm32_64_USB

Only basic USB keyboards can be used. The keyboards with extra
functions such as multimedia keys or wireless communication may
cause device driver error.


Multiple keyboards can be connected but this program reads keys
from the first keyboard only.

