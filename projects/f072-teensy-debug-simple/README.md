# A Teensy-style "debug" channel over USB raw HID

## A simple version

This example implements a raw HID device, which is able to send "debug" messages, akin to the original [Teensy] [usb_debug_only] example.

This version does *not* implement the hid debug channel as a class, it simply provides a couple of functions to write to it, Teensy-style.

The version does not implement the flush timer, the output needs to be flushed manually by calling `usb_debug_flush_output()`.

It is compatible with the original PJRC's [hid_listen] program, but a simple python reimplementation is provided (requires the [hidapi] module, i.e. `pip install hidapi`).


[Teensy]: https://www.pjrc.com/teensy/index.html
[usb_debug_only]: https://www.pjrc.com/teensy/usb_debug_only.html
[hid_listen]: https://www.pjrc.com/teensy/hid_listen.html
[hidapi]: https://github.com/trezor/cython-hidapi