#!/usr/bin/python

from onl.platform.base import *
from onl.platform.delta import *

class OnlPlatform_arm64_delta_tn4810m_r0(OnlPlatformDelta,
                                         OnlPlatformPortConfig_48x10):
    PLATFORM='arm64-delta-tn4810m-r0'
    MODEL="TN4810M"
    SYS_OBJECT_ID=".48.12"

    def baseconfig(self):
        self.insmod('optoe')

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
            ########### initialize I2C bus 5 ###########
            self.new_i2c_devices (
                [
                    ('tn4810m_cpld', 0x41, 5),
                ]
            )

            # initialize SFP+ port 1~48
            self.new_i2c_devices (
                [
                    ('optoe2', 0x50, 7),
                    ('optoe2', 0x50, 8),
                    ('optoe2', 0x50, 9),
                    ('optoe2', 0x50, 10),
                    ('optoe2', 0x50, 11),
                    ('optoe2', 0x50, 12),
                    ('optoe2', 0x50, 13),
                    ('optoe2', 0x50, 14),
                    ('optoe2', 0x50, 15),
                    ('optoe2', 0x50, 16),
                    ('optoe2', 0x50, 17),
                    ('optoe2', 0x50, 18),
                    ('optoe2', 0x50, 19),
                    ('optoe2', 0x50, 20),
                    ('optoe2', 0x50, 21),
                    ('optoe2', 0x50, 22),
                    ('optoe2', 0x50, 23),
                    ('optoe2', 0x50, 24),
                    ('optoe2', 0x50, 25),
                    ('optoe2', 0x50, 26),
                    ('optoe2', 0x50, 27),
                    ('optoe2', 0x50, 28),
                    ('optoe2', 0x50, 29),
                    ('optoe2', 0x50, 30),
                    ('optoe2', 0x50, 31),
                    ('optoe2', 0x50, 32),
                    ('optoe2', 0x50, 33),
                    ('optoe2', 0x50, 34),
                    ('optoe2', 0x50, 35),
                    ('optoe2', 0x50, 36),
                    ('optoe2', 0x50, 37),
                    ('optoe2', 0x50, 38),
                    ('optoe2', 0x50, 39),
                    ('optoe2', 0x50, 40),
                    ('optoe2', 0x50, 41),
                    ('optoe2', 0x50, 42),
                    ('optoe2', 0x50, 43),
                    ('optoe2', 0x50, 44),
                    ('optoe2', 0x50, 45),
                    ('optoe2', 0x50, 46),
                    ('optoe2', 0x50, 47),
                    ('optoe2', 0x50, 48),
                    ('optoe2', 0x50, 49),
                    ('optoe2', 0x50, 50),
                    ('optoe2', 0x50, 51),
                    ('optoe2', 0x50, 52),
                    ('optoe2', 0x50, 53),
                    ('optoe2', 0x50, 54),
                ]
            )

            # Set SFP port name
            subprocess.call('echo port1  > /sys/bus/i2c/devices/7-0050/port_name', shell=True)
            subprocess.call('echo port2  > /sys/bus/i2c/devices/8-0050/port_name', shell=True)
            subprocess.call('echo port3  > /sys/bus/i2c/devices/9-0050/port_name', shell=True)
            subprocess.call('echo port4  > /sys/bus/i2c/devices/10-0050/port_name', shell=True)
            subprocess.call('echo port5  > /sys/bus/i2c/devices/11-0050/port_name', shell=True)
            subprocess.call('echo port6  > /sys/bus/i2c/devices/12-0050/port_name', shell=True)
            subprocess.call('echo port7  > /sys/bus/i2c/devices/13-0050/port_name', shell=True)
            subprocess.call('echo port8  > /sys/bus/i2c/devices/14-0050/port_name', shell=True)
            subprocess.call('echo port9  > /sys/bus/i2c/devices/15-0050/port_name', shell=True)
            subprocess.call('echo port10 > /sys/bus/i2c/devices/16-0050/port_name', shell=True)
            subprocess.call('echo port11 > /sys/bus/i2c/devices/17-0050/port_name', shell=True)
            subprocess.call('echo port12 > /sys/bus/i2c/devices/18-0050/port_name', shell=True)
            subprocess.call('echo port13 > /sys/bus/i2c/devices/19-0050/port_name', shell=True)
            subprocess.call('echo port14 > /sys/bus/i2c/devices/20-0050/port_name', shell=True)
            subprocess.call('echo port15 > /sys/bus/i2c/devices/21-0050/port_name', shell=True)
            subprocess.call('echo port16 > /sys/bus/i2c/devices/22-0050/port_name', shell=True)
            subprocess.call('echo port17 > /sys/bus/i2c/devices/23-0050/port_name', shell=True)
            subprocess.call('echo port18 > /sys/bus/i2c/devices/24-0050/port_name', shell=True)
            subprocess.call('echo port19 > /sys/bus/i2c/devices/25-0050/port_name', shell=True)
            subprocess.call('echo port20 > /sys/bus/i2c/devices/26-0050/port_name', shell=True)
            subprocess.call('echo port21 > /sys/bus/i2c/devices/27-0050/port_name', shell=True)
            subprocess.call('echo port22 > /sys/bus/i2c/devices/28-0050/port_name', shell=True)
            subprocess.call('echo port23 > /sys/bus/i2c/devices/29-0050/port_name', shell=True)
            subprocess.call('echo port24 > /sys/bus/i2c/devices/30-0050/port_name', shell=True)
            subprocess.call('echo port25 > /sys/bus/i2c/devices/31-0050/port_name', shell=True)
            subprocess.call('echo port26 > /sys/bus/i2c/devices/32-0050/port_name', shell=True)
            subprocess.call('echo port27 > /sys/bus/i2c/devices/33-0050/port_name', shell=True)
            subprocess.call('echo port28 > /sys/bus/i2c/devices/34-0050/port_name', shell=True)
            subprocess.call('echo port29 > /sys/bus/i2c/devices/35-0050/port_name', shell=True)
            subprocess.call('echo port30 > /sys/bus/i2c/devices/36-0050/port_name', shell=True)
            subprocess.call('echo port31 > /sys/bus/i2c/devices/37-0050/port_name', shell=True)
            subprocess.call('echo port32 > /sys/bus/i2c/devices/38-0050/port_name', shell=True)
            subprocess.call('echo port33 > /sys/bus/i2c/devices/39-0050/port_name', shell=True)
            subprocess.call('echo port34 > /sys/bus/i2c/devices/40-0050/port_name', shell=True)
            subprocess.call('echo port35 > /sys/bus/i2c/devices/41-0050/port_name', shell=True)
            subprocess.call('echo port36 > /sys/bus/i2c/devices/42-0050/port_name', shell=True)
            subprocess.call('echo port37 > /sys/bus/i2c/devices/43-0050/port_name', shell=True)
            subprocess.call('echo port38 > /sys/bus/i2c/devices/44-0050/port_name', shell=True)
            subprocess.call('echo port39 > /sys/bus/i2c/devices/45-0050/port_name', shell=True)
            subprocess.call('echo port40 > /sys/bus/i2c/devices/46-0050/port_name', shell=True)
            subprocess.call('echo port41 > /sys/bus/i2c/devices/47-0050/port_name', shell=True)
            subprocess.call('echo port42 > /sys/bus/i2c/devices/48-0050/port_name', shell=True)
            subprocess.call('echo port43 > /sys/bus/i2c/devices/49-0050/port_name', shell=True)
            subprocess.call('echo port44 > /sys/bus/i2c/devices/50-0050/port_name', shell=True)
            subprocess.call('echo port45 > /sys/bus/i2c/devices/51-0050/port_name', shell=True)
            subprocess.call('echo port46 > /sys/bus/i2c/devices/52-0050/port_name', shell=True)
            subprocess.call('echo port47 > /sys/bus/i2c/devices/53-0050/port_name', shell=True)
            subprocess.call('echo port48 > /sys/bus/i2c/devices/54-0050/port_name', shell=True)

            # Set default idle state to MUX_IDLE_DISCONNECT (-2) for PCA9546 MUX PCA9546 on I2C bus 0
            subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-0/0-0077/idle_state', shell=True)

        # TN4810M non-PVT platform
        else:
            ########### initialize I2C bus 0 ###########
            self.new_i2c_devices (
                [
                    ('tn4810m_cpld', 0x41, 0),
                ]
            )

			# initialize SFP+ port 1~48
            self.new_i2c_devices (
                [
                    ('optoe2', 0x50, 3),
                    ('optoe2', 0x50, 4),
                    ('optoe2', 0x50, 5),
                    ('optoe2', 0x50, 6),
                    ('optoe2', 0x50, 7),
                    ('optoe2', 0x50, 8),
                    ('optoe2', 0x50, 9),
                    ('optoe2', 0x50, 10),
                    ('optoe2', 0x50, 11),
                    ('optoe2', 0x50, 12),
                    ('optoe2', 0x50, 13),
                    ('optoe2', 0x50, 14),
                    ('optoe2', 0x50, 15),
                    ('optoe2', 0x50, 16),
                    ('optoe2', 0x50, 17),
                    ('optoe2', 0x50, 18),
                    ('optoe2', 0x50, 19),
                    ('optoe2', 0x50, 20),
                    ('optoe2', 0x50, 21),
                    ('optoe2', 0x50, 22),
                    ('optoe2', 0x50, 23),
                    ('optoe2', 0x50, 24),
                    ('optoe2', 0x50, 25),
                    ('optoe2', 0x50, 26),
                    ('optoe2', 0x50, 27),
                    ('optoe2', 0x50, 28),
                    ('optoe2', 0x50, 29),
                    ('optoe2', 0x50, 30),
                    ('optoe2', 0x50, 31),
                    ('optoe2', 0x50, 32),
                    ('optoe2', 0x50, 33),
                    ('optoe2', 0x50, 34),
                    ('optoe2', 0x50, 35),
                    ('optoe2', 0x50, 36),
                    ('optoe2', 0x50, 37),
                    ('optoe2', 0x50, 38),
                    ('optoe2', 0x50, 39),
                    ('optoe2', 0x50, 40),
                    ('optoe2', 0x50, 41),
                    ('optoe2', 0x50, 42),
                    ('optoe2', 0x50, 43),
                    ('optoe2', 0x50, 44),
                    ('optoe2', 0x50, 45),
                    ('optoe2', 0x50, 46),
                    ('optoe2', 0x50, 47),
                    ('optoe2', 0x50, 48),
                    ('optoe2', 0x50, 49),
                    ('optoe2', 0x50, 50),
                ]
            )

            # Set SFP port name
            subprocess.call('echo port1  > /sys/bus/i2c/devices/3-0050/port_name', shell=True)
            subprocess.call('echo port2  > /sys/bus/i2c/devices/4-0050/port_name', shell=True)
            subprocess.call('echo port3  > /sys/bus/i2c/devices/5-0050/port_name', shell=True)
            subprocess.call('echo port4  > /sys/bus/i2c/devices/6-0050/port_name', shell=True)
            subprocess.call('echo port5  > /sys/bus/i2c/devices/7-0050/port_name', shell=True)
            subprocess.call('echo port6  > /sys/bus/i2c/devices/8-0050/port_name', shell=True)
            subprocess.call('echo port7  > /sys/bus/i2c/devices/9-0050/port_name', shell=True)
            subprocess.call('echo port8  > /sys/bus/i2c/devices/10-0050/port_name', shell=True)
            subprocess.call('echo port9  > /sys/bus/i2c/devices/11-0050/port_name', shell=True)
            subprocess.call('echo port10 > /sys/bus/i2c/devices/12-0050/port_name', shell=True)
            subprocess.call('echo port11 > /sys/bus/i2c/devices/13-0050/port_name', shell=True)
            subprocess.call('echo port12 > /sys/bus/i2c/devices/14-0050/port_name', shell=True)
            subprocess.call('echo port13 > /sys/bus/i2c/devices/15-0050/port_name', shell=True)
            subprocess.call('echo port14 > /sys/bus/i2c/devices/16-0050/port_name', shell=True)
            subprocess.call('echo port15 > /sys/bus/i2c/devices/17-0050/port_name', shell=True)
            subprocess.call('echo port16 > /sys/bus/i2c/devices/18-0050/port_name', shell=True)
            subprocess.call('echo port17 > /sys/bus/i2c/devices/19-0050/port_name', shell=True)
            subprocess.call('echo port18 > /sys/bus/i2c/devices/20-0050/port_name', shell=True)
            subprocess.call('echo port19 > /sys/bus/i2c/devices/21-0050/port_name', shell=True)
            subprocess.call('echo port20 > /sys/bus/i2c/devices/22-0050/port_name', shell=True)
            subprocess.call('echo port21 > /sys/bus/i2c/devices/23-0050/port_name', shell=True)
            subprocess.call('echo port22 > /sys/bus/i2c/devices/24-0050/port_name', shell=True)
            subprocess.call('echo port23 > /sys/bus/i2c/devices/25-0050/port_name', shell=True)
            subprocess.call('echo port24 > /sys/bus/i2c/devices/26-0050/port_name', shell=True)
            subprocess.call('echo port25 > /sys/bus/i2c/devices/27-0050/port_name', shell=True)
            subprocess.call('echo port26 > /sys/bus/i2c/devices/28-0050/port_name', shell=True)
            subprocess.call('echo port27 > /sys/bus/i2c/devices/29-0050/port_name', shell=True)
            subprocess.call('echo port28 > /sys/bus/i2c/devices/30-0050/port_name', shell=True)
            subprocess.call('echo port29 > /sys/bus/i2c/devices/31-0050/port_name', shell=True)
            subprocess.call('echo port30 > /sys/bus/i2c/devices/32-0050/port_name', shell=True)
            subprocess.call('echo port31 > /sys/bus/i2c/devices/33-0050/port_name', shell=True)
            subprocess.call('echo port32 > /sys/bus/i2c/devices/34-0050/port_name', shell=True)
            subprocess.call('echo port33 > /sys/bus/i2c/devices/35-0050/port_name', shell=True)
            subprocess.call('echo port34 > /sys/bus/i2c/devices/36-0050/port_name', shell=True)
            subprocess.call('echo port35 > /sys/bus/i2c/devices/37-0050/port_name', shell=True)
            subprocess.call('echo port36 > /sys/bus/i2c/devices/38-0050/port_name', shell=True)
            subprocess.call('echo port37 > /sys/bus/i2c/devices/39-0050/port_name', shell=True)
            subprocess.call('echo port38 > /sys/bus/i2c/devices/40-0050/port_name', shell=True)
            subprocess.call('echo port39 > /sys/bus/i2c/devices/41-0050/port_name', shell=True)
            subprocess.call('echo port40 > /sys/bus/i2c/devices/42-0050/port_name', shell=True)
            subprocess.call('echo port41 > /sys/bus/i2c/devices/43-0050/port_name', shell=True)
            subprocess.call('echo port42 > /sys/bus/i2c/devices/44-0050/port_name', shell=True)
            subprocess.call('echo port43 > /sys/bus/i2c/devices/45-0050/port_name', shell=True)
            subprocess.call('echo port44 > /sys/bus/i2c/devices/46-0050/port_name', shell=True)
            subprocess.call('echo port45 > /sys/bus/i2c/devices/47-0050/port_name', shell=True)
            subprocess.call('echo port46 > /sys/bus/i2c/devices/48-0050/port_name', shell=True)
            subprocess.call('echo port47 > /sys/bus/i2c/devices/49-0050/port_name', shell=True)
            subprocess.call('echo port48 > /sys/bus/i2c/devices/50-0050/port_name', shell=True)

        # Set default idle state to MUX_IDLE_DISCONNECT (-2) for PCA9546 MUX PCA9548 on I2C bus 2
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0070/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0071/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0072/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0073/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0074/idle_state', shell=True)
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-2/2-0075/idle_state', shell=True)

        return True
