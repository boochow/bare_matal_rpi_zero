# video2

Framebuffer example for RPi Zero W.

This example yields the same results as the [video](https://github.com/boochow/bare_matal_rpi_zero/tree/master/video) example.

The diffrence is that this example uses the mailbox property interface to set up the frame buffer.

See [Mailbox property interface · raspberrypi/firmware Wiki](https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface) for details.

Virtual dislpay resolution is 480x270 pixels, 16 bpp.
Physical signal (HDMI) is 1920x1080.
