# USB-CDC example for STM32F042

Supposed to run on STM32F042FxP6 breakout board. Should enumerate as a serial device over USB. A button press should produce a "Hello, world" message over the the serial/USB pipe.

The USB connection is on PA11/PA12 pins, which share the physical pins with PA9/PA10 on the TSSOP-20 STM32F042FxP6 chips.
