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

        # Insert prestera kernel modules
        os.system("insmod /lib/modules/`uname -r`/kernel/drivers/net/ethernet/marvell/prestera_sw/prestera_sw.ko")
        os.system("insmod /lib/modules/`uname -r`/kernel/drivers/net/ethernet/marvell/prestera_sw/prestera_pci.ko")

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

        # Set SFP port name
        subprocess.call('echo port49 > /sys/bus/i2c/devices/3-0050/port_name', shell=True)
        subprocess.call('echo port50 > /sys/bus/i2c/devices/4-0050/port_name', shell=True)
        subprocess.call('echo port51 > /sys/bus/i2c/devices/5-0050/port_name', shell=True)
        subprocess.call('echo port52 > /sys/bus/i2c/devices/6-0050/port_name', shell=True)

        return True
