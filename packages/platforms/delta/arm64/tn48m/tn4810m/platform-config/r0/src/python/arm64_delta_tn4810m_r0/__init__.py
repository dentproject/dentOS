#!/usr/bin/python

from onl.platform.base import *
from onl.platform.delta import *

class OnlPlatform_arm64_delta_tn4810m_r0(OnlPlatformDelta,
                                         OnlPlatformPortConfig_48x10):
    PLATFORM='arm64-delta-tn4810m-r0'
    MODEL="TN4810M"
    SYS_OBJECT_ID=".48.12"

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

        ret = subprocess.call('i2cget -f -y 0 0x77 0x00 > /dev/null 2>&1', shell=True)
        # if ret == 0 means pca9546 mux exists (at 0x77), it is TN4810M PVT platform
        if ret == 0:
            # Set default idle state to MUX_IDLE_DISCONNECT (-2) for PCA9546 MUX PCA9546 on I2C bus 0
            subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-0/0-0077/idle_state', shell=True)

        # Set default idle state to MUX_IDLE_DISCONNECT (-2) for PCA9546 MUX PCA9548 on I2C bus 2
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0070/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0071/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0072/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0073/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0074/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0075/idle_state', shell=True)


        # Insert prestera kernel module
        self.insmod("prestera_sw.ko")
        self.insmod("prestera_pci.ko")

        return True
