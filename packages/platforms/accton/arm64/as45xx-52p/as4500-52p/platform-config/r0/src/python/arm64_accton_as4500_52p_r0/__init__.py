#!/usr/bin/python

from onl.platform.base import *
from onl.platform.accton import *

class OnlPlatform_arm64_accton_as4500_52p_r0(OnlPlatformAccton,
                                       OnlPlatformPortConfig_48x1_4x10):
    PLATFORM='arm64-accton-as4500-52p-r0'
    MODEL="AS4500-52P"
    SYS_OBJECT_ID=".4500.52.1"

    def baseconfig(self):
        self.insmod('optoe')
        self.insmod('dps850')

        # Insert platform drivers
        for m in [ 'mux', 'cpld', 'psu' ]:
            self.insmod("arm64-accton-as45xx-52p-%s" % m)

        ########### initialize I2C bus 1 ###########
        self.new_i2c_devices(
            [
                # initialize CPLD
                ('pca9641_mux', 0x73, 0), # i2c-2
                ('as45xx_cpldm_mux', 0x76, 0), # i2c-3 ~ i2c-8
                ('as4500_cpld_m', 0x42, 0),
                ('as45xx_cpld_s', 0x62, 2),
                ('as45xx_psu1', 0x50, 6),
                ('dps850', 0x58, 6),
                ('as45xx_psu2', 0x51, 5),
                ('dps850', 0x59, 5),
                ('pca9546', 0x71, 7), # i2c-9 ~ i2c-12
                ('pca9546', 0x70, 8), # i2c-13 ~ i2c-16
                ('tmp175', 0x48, 10),
                ('tmp175', 0x49, 10),
                ('tmp175', 0x4b, 10),
                ('tmp175', 0x4c, 10),
                ('24c64', 0x54, 9)
                ]
            )

        # Insert platform drivers
        for m in [ 'fan', 'sfp' ]:
            self.insmod("arm64-accton-as45xx-52p-%s" % m)

        # initialize SFP devices
        for port in range(49, 53):
            self.new_i2c_device('optoe2', 0x50, port-36)
            subprocess.call('echo port%d > /sys/bus/i2c/devices/%d-0050/port_name' % (port, port-36), shell=True)

        return True
