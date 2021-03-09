#!/usr/bin/python

from onl.platform.base import *
from onl.platform.delta import *

class OnlPlatform_arm64_delta_tn48m_poe_dn_r0(OnlPlatformDelta,
                                              OnlPlatformPortConfig_48x1_4x10):
    PLATFORM='arm64-delta-tn48m-poe-dn-r0'
    MODEL="TN48M-POE-DN"
    SYS_OBJECT_ID=".48.3"

    def baseconfig(self):
        # Insert platform drivers
        self.insmod("arm64-delta-tn48m-poe-dn-psu.ko")
        self.insmod("arm64-delta-tn48m-dn-cpld.ko")
        self.insmod("arm64-delta-tn48m-dn-led.ko")

        ########### initialize I2C bus 0 ###########
        self.new_i2c_devices (
            [
                ('tn48m_poe_dn_psu', 0x58, 0),
                ('tn48m_poe_dn_psu', 0x59, 0),
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

        # set up systemctl rules
        for swp in range(1, 53):
           cmd = "systemctl enable switchdev-online@swp%d" % swp
        subprocess.check_call(cmd, shell=True)

        return True
