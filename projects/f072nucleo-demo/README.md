# An example for ST F072-NUCLEO board

It is basically just an adaptation of ChibiOS's `demos/STM32/RT-STM32F072-DISCOVERY` to the F072-NUCLEO board, for testing the board definition for this NUCLEO board.

This NUCLEO board has USART2 routed to its ST-LINK, which enumerates (among other things) as a serial port. On the USER button press, the board runs tests and prints the output over this serial port.
