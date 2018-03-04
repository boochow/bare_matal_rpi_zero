# usb_kbd2

Another example of reading USB Keyboard keycodes.

USB driver is derived from
https://github.com/LdB-ECM/Raspberry-Pi/tree/master/Arm32_64_USB

The raw key codes of pressed keys are printed to UART.

Tested on RPi Zero W with a USB hub.

If you use RPi A series or Zero series you must insert a usb
hub between RPi and USB keyboard because this USB driver assumes
a usb hub is always connected to the SoC of RPi and it is correct 
in case when you use B-type RPis, because they have onboard USB
hub chip (LAN9512).

Multiple keyboards can be connected but this program reads keys
from the first keyboard only.

