# Jump to bootloader example

Supposed to run on STM32F042FxP6 breakout board. The LED blinks. A button press jumps to bootloader.

## Notes

I do not know whether the current version of the code would work on other than STM32 MCUs.

This needs a patch to the standard ChibiOS (tested with 3.0.1).
Apply the patch by changing into the root directory of your ChibiOS, and then

        patch -p1 < /path/to/ch-bootloader-jump.patch

Furthermore, the correct BOOTLOADER_ADDRESS define has to be passed to the compiler in the `Makefile`.

The addresses for the STM32 chips can be found in [AN2606 application note](http://www.st.com/web/en/resource/technical/document/application_note/CD00167594.pdf), page 175.

Here are some of the values:
 - for SSTM32F03xx4/6/8 `BOOTLOADER_ADDRESS=0x1FFFEC00`
 - for STM32F04xxx `BOOTLOADER_ADDRESS=0x1FFFC400`
 - for STM32F072xx `BOOTLOADER_ADDRESS=0x1FFFC800`

The mechanism has been descibed on several forums - basically the user code writes a "magic number" into the last 4 bytes of RAM, and then resets the MCU. The `ResetHandler` then checks for this number, and if it is there, it jumps to the bootloader. This of course needs changes to the default `ResetHandler` supplied with ChibiOS.
