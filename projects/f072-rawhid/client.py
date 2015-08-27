#!/usr/bin/env python

import hid
import time

# list all hid devices
#for d in hid.enumerate():
    #keys = d.keys()
    #keys.sort()
    #for key in keys:
        #print "%s : %s" % (key, d[key])
    #print ""

try:
    print "Opening device"
    h = hid.device()
    h.open(0x0483, 0x5740) # stm
    #h.open(0x16c0, 0x0480) # teensy

    print "Manufacturer: %s" % h.get_manufacturer_string()
    print "Product: %s" % h.get_product_string()
    print "Serial No: %s" % h.get_serial_number_string()

    # try non-blocking mode by uncommenting the next line
    #h.set_nonblocking(1)

    # Blink the LED
    for k in range(10):
        # this may? keep blocking indefinitely if not in non-blocking mode
        #  although the packet is received by the other side
        # LED ON
        h.write([0x00, 0, 0]) # first byte is report ID
        # get button state if changed
        d = h.read(2,500) # bytes, <timeout>
        if d:
            print d
        # LED OFF
        h.write([0x00, 0, 1])
        d = h.read(2,500) # bytes, <timeout>
        if d:
            print d

    print "Closing device"
    h.close()

except IOError, ex:
    print ex
    print "Device not present or:"
    print "You probably don't have the hard coded test hid. Update the hid.device line"
    print "in this script with one from the enumeration list output above and try again."

print "Done"
