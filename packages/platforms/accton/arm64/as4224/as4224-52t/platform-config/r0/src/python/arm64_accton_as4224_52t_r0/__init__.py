#!/usr/bin/python

from onl.platform.base import *
from onl.platform.accton import *

class OnlPlatform_arm64_accton_as4224_52t_r0(OnlPlatformAccton,
                                       OnlPlatformPortConfig_48x1_4x10):
    PLATFORM='arm64-accton-as4224-52t-r0'
    MODEL="AS4224-52T"
    SYS_OBJECT_ID=".4224.52.2"

    def baseconfig(self):
        self.insmod('optoe')

        # Insert platform drivers
        self.insmod("arm64-accton-as4224-cpld.ko")
        self.insmod('arm64-accton-as4224-gpio-i2c.ko')

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
        for port in range(49, 53):
            self.new_i2c_device('optoe2', 0x50, port-46)
            subprocess.call('echo port%d > /sys/bus/i2c/devices/%d-0050/port_name' % (port, port-46), shell=True)

        # Below platform drivers should be inserted after cpld driver is initiated.
        for m in [ 'fan', 'psu' ]:
            self.insmod("arm64-accton-as4224-%s" % m)

        # Insert prestera kernel modules
        os.system("insmod /lib/modules/`uname -r`/kernel/drivers/net/ethernet/marvell/prestera_sw/prestera_sw.ko")
        os.system("insmod /lib/modules/`uname -r`/kernel/drivers/net/ethernet/marvell/prestera_sw/prestera_pci.ko")

        #subprocess.call('echo 1 > ./sys/devices/platform/cp0/cp0:config-space/f2500000.usb3/usb1/1-1/1-1.1/bConfigurationValue', shell=True)

        return True
