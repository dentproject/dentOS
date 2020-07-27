#!/usr/bin/python

from onl.platform.base import *
from onl.platform.wnc import *

class OnlPlatform_arm64_wnc_qsd61_aom_a_48_r0(OnlPlatformWNC,
                                          OnlPlatformPortConfig_48x10_4x40):
    PLATFORM='arm64-wnc-qsd61-aom-a-48-r0'
    MODEL="QSD61-AOM-A-48"
    SYS_OBJECT_ID=".6189.30.5449"

    def baseconfig(self):

        self.insmod("qsd61-aom-a-48-sys_cpld")
        self.insmod("qsd61-aom-a-48-sfp_plus_cpld")
        self.insmod("optoe")

        self.new_i2c_devices([
                ('pca9548', 0x70, 0),
                ('pca9548', 0x70, 1),
                ('24c02', 0x54, 3),
                ('lm75', 0x48, 6),
                ('lm75', 0x49, 7),
                ('lm75', 0x4A, 8),
                ('pca9548', 0x71, 11),
                ('pca9548', 0x72, 12),
                ('pca9548', 0x73, 13),
                ('pca9548', 0x74, 14),
                ('pca9548', 0x75, 15),
                ('pca9548', 0x76, 16),
                ])

        port_num = [19, 20, 21, 22, 23, 24, 25, 26, \
                27, 28, 29, 30, 31, 32, 33, 34,\
                35, 36, 37, 38, 39, 40, 41, 42,\
                43, 44, 45, 46, 47, 48, 49, 50,\
                51, 52, 53, 54, 55, 56, 57, 58,\
                59, 60, 61, 63, 64, 65, 66]

        for port in port_num:
            self.new_i2c_devices([
                ('optoe2', 0x50, port),
                ])

        #CP_MPP 6 CPLD1 interrupt
        subprocess.call('echo 38 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio38/direction', shell=True)
        subprocess.call('echo falling > /sys/class/gpio/gpio38/edge', shell=True)

        #CP_MPP 18,19,20,21,22,23 Boot mode
        subprocess.call('echo 50 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio50/direction', shell=True)
        subprocess.call('echo 51 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio51/direction', shell=True)
        subprocess.call('echo 52 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio52/direction', shell=True)
        subprocess.call('echo 53 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio53/direction', shell=True)
        subprocess.call('echo 54 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio54/direction', shell=True)
        subprocess.call('echo 55 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio55/direction', shell=True)

        #CP_MPP 28, CPLD_TDI, input
        subprocess.call('echo 60 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo in  > /sys/class/gpio/gpio60/direction', shell=True)
        #CP_MPP 30, CPLD_CLK, output
        subprocess.call('echo 62 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo out  > /sys/class/gpio/gpio62/direction', shell=True)
        #CP_MPP 32, CPLD_TMS, output
        subprocess.call('echo 64 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo out  > /sys/class/gpio/gpio64/direction', shell=True)
        #CP_MPP 33, CPLD_TDO, output
        subprocess.call('echo 65 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo out  > /sys/class/gpio/gpio65/direction', shell=True)
        #CP_MPP34, A7040_CPLD1_I2C_BZ
        subprocess.call('echo 66 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo out  > /sys/class/gpio/gpio66/direction', shell=True)
        subprocess.call('echo 1 > /sys/class/gpio/gpio66/value', shell=True)


        self.new_i2c_devices([
                        ('qsd61_48_sys_cpld1', 0x77, 2),
                        ('qsd61_48_sfp_cpld2', 0x76, 2),
                        ])

        return True