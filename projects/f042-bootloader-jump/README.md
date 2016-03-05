# Jump to bootloader example

Supposed to run on STM32F042FxP6 breakout board. The LED blinks. A button press jumps to bootloader.

**Note!** It seems that it is not possible to cleanly jump to the DFU bootloader on STM32F042 [forum thread](https://my.st.com/public/STe2ecommunities/mcu/Lists/cortex_mx_stm32/Flat.aspx?RootFolder=%2Fpublic%2FSTe2ecommunities%2Fmcu%2FLists%2Fcortex_mx_stm32%2FJump%20to%20USB%20DFU%20Bootloader%20in%20startup%20code%20on%20STM32F042&FolderCTID=0x01200200770978C69A1141439FE559EB459D7580009C4E14902C3CDE46A77F0FFD06506F5B&currentviews=1098). The point is that the DFU bootloader _either_ checks the `BOOT0` pin, _or_ honours the User Option byte. So _either_ one makes the `BOOT0` pin `HIGH` (this requires extra components and/or connections; or one can set the pin high and then jump to the bootloader code, hoping that it will last -- wasn't successful for me, as I have a 10k pulldown + button on that pin); _or_ it requires messing with the option byte in flash.

## Notes

~~This needs a patch to the standard ChibiOS (tested with 3.0.1).
Apply the patch by changing into the root directory of your ChibiOS, and then `patch -p1 < /path/to/ch-bootloader-jump.patch`.~~

Patching no longer encouraged; it is possible to put the code into the board files. For an example, see the STM32F042_BREAKOUT board file.

Furthermore, the correct BOOTLOADER_ADDRESS define has to be passed to the compiler in the `Makefile`.

The addresses for the STM32 chips can be found in [AN2606 application note](http://www.st.com/web/en/resource/technical/document/application_note/CD00167594.pdf), page _(keeps changing)_.

Here are some of the values:
 - for SSTM32F03xx4/6/8 `BOOTLOADER_ADDRESS=0x1FFFEC00`
 - for STM32F04xxx `BOOTLOADER_ADDRESS=0x1FFFC400`
 - for STM32F072xx `BOOTLOADER_ADDRESS=0x1FFFC800`

The mechanism has been descibed on several forums - basically the user code writes a "magic number" into the last 4 bytes of RAM, and then resets the MCU. The `ResetHandler` then checks for this number, and if it is there, it jumps to the bootloader. This of course needs changes to the default `ResetHandler` supplied with ChibiOS.
