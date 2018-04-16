# timer-irq

Timer interrupt example for RPi Zero W.

ARM Timer interrupt occurs every 0.5 seconds.
The interrupt handler increases a counter value.
Inifinite loop in the main function prints counter value
to mini-UART port when it was changed.
