#!/usr/bin/python

from onl.platform.base import *
from onl.platform.delta import *

class OnlPlatform_arm64_delta_tn48m_poe_r0(OnlPlatformDelta,
                                           OnlPlatformPortConfig_48x1_4x10):
    PLATFORM='arm64-delta-tn48m-poe-r0'
    MODEL="TN48M-POE"
    SYS_OBJECT_ID=".48.3"

    def baseconfig(self):
        # Insert platform drivers
        self.insmod("arm64-delta-tn48m-poe-psu.ko")
        self.insmod("arm64-delta-tn48m-cpld.ko")
        self.insmod("arm64-delta-tn48m-led.ko")

        ########### initialize I2C bus 0 ###########
        self.new_i2c_devices (
            [
                ('tn48m_poe_psu', 0x58, 0),
                ('tn48m_poe_psu', 0x59, 0),
            ]
        )

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

        # Insert Marvell prestera modules by only probing prestera_pci module
        # Suggested by Taras Chornyi
        self.modprobe('prestera_pci')

        # setup systemctl rules
        for swp in range(1, 53):
           cmd = "systemctl enable switchdev-online@swp%d" % swp
        subprocess.check_call(cmd, shell=True)

        return True
