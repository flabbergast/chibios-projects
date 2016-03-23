# ChibiOS/rawHID USB example

*NOTE* Now working at the moment; needs updating to the new USB API.

Originally from: https://github.com/guiduc/usb-hid-chibios-example

Runs on STM32F072B-DISCOVERY board.

Enumerates as a raw HID USB device.

Blue LED blinks once after reset, then should start blinking regularly.
Slow blinking (500ms between changes) means USB is not active, fast
blinking (250ms between changes) means USB is active, and should be
already enumerated by the computer.

Two endpoints, one in, one out, both interrupt. Report size 2 bytes both
ways. PC-to-board is (counter, red-led-state), board-to-PC is (counter,
button-state).

Example host-side python script included, uses the `hidapi` module.

