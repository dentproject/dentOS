#!/usr/bin/python

from onl.platform.base import *
from onl.platform.accton import *

class OnlPlatform_arm64_accton_as4581_52pl_r0(OnlPlatformAccton,
                                       OnlPlatformPortConfig_48x1_4x10):
    PLATFORM='arm64-accton-as4581-52pl-r0'
    MODEL="AS4581-52PL"
    SYS_OBJECT_ID=".4581.52.1"

    def baseconfig(self):
        self.modprobe('i2c-imx')
        self.modprobe('i2c-mux-pca954x')
        self.insmod('optoe')
        self.insmod('dps850')

        # Insert platform drivers
        for m in [ 'mux', 'cpld', 'psu', 'asc' ]:
            self.insmod("arm64-accton-as45xx-52p-%s" % m)

        ########### initialize I2C bus 1 ###########
        self.new_i2c_devices(
            [
                # initialize CPLD
                ('pca9641_mux', 0x73, 1), # i2c-8
                ('as45xx_cpldm_mux', 0x76, 1), # i2c-9 ~ i2c-14
                ('as4581_cpld_m', 0x61, 1),
                ('as45xx_cpld_s', 0x62, 8),
                ('as45xx_psu1', 0x50, 12),
                ('dps850', 0x58, 12),
                ('as45xx_psu2', 0x51, 11),
                ('dps850', 0x59, 11),
                ('pca9546', 0x71, 13), # i2c-15 ~ i2c-18
                ('pca9546', 0x70, 14), # i2c-19 ~ i2c-22
                ('tmp175', 0x48, 16),
                ('tmp175', 0x49, 16),
                ('tmp175', 0x4b, 16),
                ('tmp175', 0x4c, 16),
                ('24c64', 0x54, 15),

                # initialize ASC
                ('pca9548', 0x70, 0), # i2c-23 ~ i2c-30
                ('as45xx_asc', 0x60, 26)
                ]
            )

        # Insert platform drivers
        for m in [ 'fan', 'sfp' ]:
            self.insmod("arm64-accton-as45xx-52p-%s" % m)

        # initialize SFP devices
        for port in range(49, 53):
            self.new_i2c_device('optoe2', 0x50, port-30)
            subprocess.call('echo port%d > /sys/bus/i2c/devices/%d-0050/port_name' % (port, port-30), shell=True)

        return True
