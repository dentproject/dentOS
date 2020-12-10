#!/usr/bin/python

from onl.platform.base import *
from onl.platform.delta import *

class OnlPlatform_arm64_delta_tn48m2_r0(OnlPlatformDelta,
                                         OnlPlatformPortConfig_48x1_4x10):
    PLATFORM='arm64-delta-tn48m2-r0'
    MODEL="TN48M2"
    SYS_OBJECT_ID=".48.13"

    def baseconfig(self):
        # Insert platform drivers
        self.insmod("arm64-delta-tn48m-cpld.ko")
        self.insmod("arm64-delta-tn48m-led.ko")

        ########### initialize I2C bus 1 ###########
        self.new_i2c_devices (
            [
                # FAN Controller
                ('adt7473', 0x2e, 1),

                # Temperature devices
                ('tmp1075', 0x4a, 1),
                ('tmp1075', 0x4b, 1),
            ]
        )

        # Insert prestera kernel module
        self.insmod("prestera_sw.ko")
        self.insmod("prestera_pci.ko")
        return True
