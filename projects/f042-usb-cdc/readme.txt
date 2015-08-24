need to activate a serial port in mcuconf.h and serial in halconf.h in order for sd* functions to work
but instead can use BaseSequentialStream cast od SDU1 and chprintf, write/read/get/put
or BaseChannel cast and chnWrite etc
note chprintf adds >1.3k to binary size
