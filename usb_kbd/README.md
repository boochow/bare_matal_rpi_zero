# usb_kbd

Example of reading USB Keyboard keycode.
The raw key codes of pressed keys (up to six keys) are printed
to UART.

Tested on RPi Zero W.

USB driver is derived from https://github.com/Chadderz121/csud .

Only basic USB keyboards can be used. The keyboards with extra
functions such as multimedia keys or wireless communication may
cause device driver error.


You can use a USB hub to connect a keyboard. Multiple keyboards
can be connected but this program scans key codes from only one
keyboard.

Conncting / Disconnecting keyboard while the program is running
is allowed.