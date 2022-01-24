#!/usr/bin/python

from onl.platform.base import *
from onl.platform.accton import *

class OnlPlatform_arm64_accton_as4564_26p_r0(OnlPlatformAccton,
                                       OnlPlatformPortConfig_24x1_2x10):
    PLATFORM='arm64-accton-as4564-26p-r0'
    MODEL="AS4564-26P"
    SYS_OBJECT_ID=".4564.26.1"

    def baseconfig(self):
        self.insmod('optoe')

        # Insert platform drivers
        self.insmod("arm64-accton-as4564-26p-cpld.ko")
        self.insmod('arm64-accton-as4564-26p-gpio-i2c.ko')

        ########### initialize I2C bus 0 ###########
        self.new_i2c_devices(
            [
                # initialize CPLD
                ('as4564_26p_cpld1', 0x40, 0),
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
        for port in range(25, 27):
            self.new_i2c_device('optoe2', 0x50, port-20)
            subprocess.call('echo port%d > /sys/bus/i2c/devices/%d-0050/port_name' % (port, port-20), shell=True)

        # Below platform drivers should be inserted after cpld driver is initiated.
        for m in [ 'psu', 'fan', 'thermal' ]:
            self.insmod("arm64-accton-as4564-26p-%s" % m)

        # Insert prestera kernel module
        self.modprobe('prestera_pci')

        # set up systemctl rules
        for swp in range(1, 27):
            cmd = "systemctl enable switchdev-online@swp%d" % swp
            subprocess.check_call(cmd, shell=True)

        return True
