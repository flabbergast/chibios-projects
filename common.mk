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

serialflash: all
	#stm32flash -w $(BUILDDIR)/$(PROJECT).hex -v $(SERIAL_PORT)
	stm32loader.py -evw -b 57600 $(BUILDDIR)/$(PROJECT).bin

### dfu-util

dfu: all
	dfu-util -a 0 -s 0x08000000 -R -D $(BUILDDIR)/$(PROJECT).bin

### st-link

stlink: all
	st-flash write $(BUILDDIR)/$(PROJECT).bin 0x8000000

### blhost

blhost: all
	blhost -u -- flash-erase-all
	blhost -u -- write-memory 0 $(BUILDDIR)/$(PROJECT).bin
	blhost -u -- reset
