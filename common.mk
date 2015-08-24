### OpenOCD

# Location of OpenOCD Board .cfg files
OPENOCD_BOARD_DIR = /usr/local/Cellar/open-ocd/0.9.0/share/openocd/scripts/board

# OpenOCD board config file
OPENOCD_BOARD_CFG = stm32f0discovery.cfg

# Flashes your board using OpenOCD
openocd: all
	openocd -f $(OPENOCD_BOARD_DIR)/$(OPENOCD_BOARD_CFG) -f $(ROOTDIR)/openocd/stm32f0-flash.cfg \
            -c "stm_flash `pwd`/$(BUILDDIR)/$(PROJECT).bin" -c shutdown

### stm32flash

SERIAL_PORT = /dev/tty.usbserial-DJ005LMJ

stm32flash: all
	stm32flash -w $(BUILDDIR)/$(PROJECT).bin -v -R $(SERIAL_PORT)

### dfu-util

dfu: all
	dfu-util -a 0 -s 0x08000000 -R -D $(BUILDDIR)/$(PROJECT).bin

### st-link

stlink: all
	st-flash write $(BUILDDIR)/$(PROJECT).bin 0x8000000

