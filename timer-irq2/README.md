# timer-irq2

Another timer interrupt example for RPi Zero W.

There are 4 System timers whereas timers 0 and 2
are used by GPU so we can use timers 1 and 3.

In this example timer 1 interrupts every 960,000 counts
and timer 3 does every 720,000 counts.
Each timer increases independent counter value and the
new value is printed onto UART1.