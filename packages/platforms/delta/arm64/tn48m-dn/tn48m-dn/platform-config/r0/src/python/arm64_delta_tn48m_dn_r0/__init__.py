#!/usr/bin/python

from onl.platform.base import *
from onl.platform.delta import *

class OnlPlatform_arm64_delta_tn48m_dn_r0(OnlPlatformDelta,
                                          OnlPlatformPortConfig_48x1_4x10):
    PLATFORM='arm64-delta-tn48m-dn-r0'
    MODEL="TN48M-DN"
    SYS_OBJECT_ID=".48.10"

    def baseconfig(self):
        # Insert platform drivers
        self.insmod("arm64-delta-tn48m-dn-cpld.ko")
        self.insmod("arm64-delta-tn48m-dn-led.ko")

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

