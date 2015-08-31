# A Teensy-style "debug" channel over USB raw HID

## A BaseAsynchronousChannel version

This example implements a raw HID device, which is able to send "debug" messages, akin to the original [Teensy] [usb_debug_only] example.

This version implements the debug-out as an instance of ChibiOS's BaseAsynchronousChannel, so one can use ChibiOS's BaseChannel functions to write to it.

The output is automatically flushed after 50ms (see `usb_hid_debug.h`), or immediately by calling `usb_debug_flush_output()`.

It is compatible with the original PJRC's [hid_listen] program, but a simple python reimplementation is provided (requires the [hidapi] module, i.e. `pip install hidapi`).


[Teensy]: https://www.pjrc.com/teensy/index.html
[usb_debug_only]: https://www.pjrc.com/teensy/usb_debug_only.html
[hid_listen]: https://www.pjrc.com/teensy/hid_listen.html
[hidapi]: https://github.com/trezor/cython-hidapi