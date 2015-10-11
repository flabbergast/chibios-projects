# f072-keyboard

This example implements a more complicated USB device, which enumerates as a keyboard (possibly with 2 interfaces, one "normal" one and one which supports NKRO; both possibly supporting "extra" keys, like mute or sleep), a mouse and a teensy-style debug channel (or console). The features can be selected/disabled in the `Makefile`.

Have a look at the `main.c` file for examples of how to use various interfaces from the main program logic; `usb_main.c` contains the stack itself.

By default, the code is set to run on the ST F072RB DISCOVERY board.

This code is aimed towards integration with [TMK] Keyboard Firmware Collection, so some parts are written in a slightly less straighforward manner than it would be possible if it was geared towards a standalone project.


[TMK]: https://github.com/tmk/tmk_keyboard/
