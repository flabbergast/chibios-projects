#!/usr/bin/env python

import hid
import time
import sys

try:
    print "Opening device"
    h = hid.device()
    h.open(0x16c0, 0x0479) # teensy

    print "Manufacturer: %s" % h.get_manufacturer_string()
    print "Product: %s" % h.get_product_string()
    print "Serial No: %s" % h.get_serial_number_string()

    # try non-blocking mode by uncommenting the next line
    #h.set_nonblocking(1)

    print "Interrupt with Ctrl-C..."

    try:
        while True:
            d = h.read(64,50) # bytes, <timeout>
            if d:
                for c in d:
                    sys.stdout.write(chr(c))
                sys.stdout.flush()
    except KeyboardInterrupt:
        pass

    print "\nClosing device"
    h.close()

except IOError, ex:
    print ex
    print "Device not present or:"
    print "The USB's VID and PID don't match the hardcoded ones on the hid.device line"

print "Done"
