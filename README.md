# chibios-projects

Several examples for ChibiOS 3.0.x (some very close to original ChibiOS demos), to be used with arm-none-eabi GCC toolchain, Makefiles and plain text editing. (So no fancy eclipse projects, etc...)

The structure is made so that it is easy to upgrade ChibiOS to a newer one (no changes to the distribution tree are required), just replace the `chibios` symlink.

The `Makefile`s are adjusted so that it is easy to add new board definitions (`boards`) and linker scripts (`ld`). These take precedence over ChibiOS provided ones if needed.

The actual examples are to be found in the `projects` directory.

Several flashing methods are set up in `common.mk` (e.g. `make dfu` works).