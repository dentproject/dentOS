#!/usr/bin/python

from onl.platform.base import *
from onl.platform.accton import *

class OnlPlatform_arm64_accton_as5114_48x_r0(OnlPlatformAccton,
                                       OnlPlatformPortConfig_48x10):
    PLATFORM='arm64-accton-as5114-48x-r0'
    MODEL="AS5114-48X"
    SYS_OBJECT_ID=".5114.48"

    def baseconfig(self):
        self.insmod('optoe')

        # Insert platform drivers
        self.insmod("arm64-accton-as4224-cpld.ko")
        self.insmod('arm64-accton-as4224-gpio-i2c.ko')

        # Insert prestera kernel modules

        ########### initialize I2C bus 0 ###########
        self.new_i2c_devices(
            [
                # initialize CPLD
                ('as4224_cpld1', 0x40, 0),
                ]
            )

        ########### initialize I2C bus 1 ###########
        self.new_i2c_devices(
            [
                # inititate LM75
                ('lm75', 0x48, 1),
                ('lm75', 0x49, 1),
                ('lm75', 0x4b, 1),
                ('lm75', 0x4c, 1),
                ]
            )

        ########### initialize I2C bus 2 ###########

        # initialize SFP devices
        for port in range(1, 49):
            self.new_i2c_device('optoe2', 0x50, port+2)
            subprocess.call('echo port%d > /sys/bus/i2c/devices/%d-0050/port_name' % (port, port+2), shell=True)

        # Below platform drivers should be inserted after cpld driver is initiated.
        for m in [ 'fan', 'psu' ]:
            self.insmod("arm64-accton-as4224-%s" % m)

        # Insert prestera kernel modules
        self.modprobe('prestera_pci')


        return True
