#!/usr/bin/python

from onl.platform.base import *
from onl.platform.delta import *

class OnlPlatform_arm64_delta_tn4810m_dn_r0(OnlPlatformDelta,
                                            OnlPlatformPortConfig_48x10):
    PLATFORM='arm64-delta-tn4810m-dn-r0'
    MODEL="TN4810M-DN"
    SYS_OBJECT_ID=".48.12"

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

        # Set default idle state to MUX_IDLE_DISCONNECT (-2) for PCA9546 MUX PCA9546 on I2C bus 0
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-0/0-0077/idle_state', shell=True)

        # Set default idle state to MUX_IDLE_DISCONNECT (-2) for PCA9546 MUX PCA9548 on I2C bus 2
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0070/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0071/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0072/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0073/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0074/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0075/idle_state', shell=True)

        # Insert Marvell prestera modules by only probing prestera_pci module
        # Suggested by Taras Chornyi
        self.modprobe('prestera_pci')

        # set up systemctl rules
        for swp in range(1, 49):
           cmd = "systemctl enable switchdev-online@swp%d" % swp
        subprocess.check_call(cmd, shell=True)

        return True
