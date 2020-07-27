#!/usr/bin/python

from onl.platform.base import *
from onl.platform.wnc import *

class OnlPlatform_arm64_wnc_qsa72_aom_a_48p_r0(OnlPlatformWNC,
                                          OnlPlatformPortConfig_48x1_4x10):
    PLATFORM='arm64-wnc-qsa72-aom-a-48p-r0'
    MODEL="QSA72-AOM-A-48P"
    SYS_OBJECT_ID=".6189.34.5550"

    def baseconfig(self):

        self.insmod("qsa72-aom-a-48p-sys_cpld")
        self.insmod("optoe")

        self.new_i2c_devices([
                ('pca9548', 0x70, 0),
                ('pca9548', 0x70, 1),
                ('pca9539', 0x74, 2),
                ('24c02', 0x54, 3),
                ('lm75', 0x48, 6),
                ('lm75', 0x49, 7),
                ('lm75', 0x4A, 8),
                ])

        port_num = [11, 12, 13, 14]

        for port in port_num:
            self.new_i2c_devices([
                ('optoe2', 0x50, port),
                ])

        #CP_MPP 6 CPLD1 interrupt
        subprocess.call('echo 38 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio38/direction', shell=True)
        subprocess.call('echo falling > /sys/class/gpio/gpio38/edge', shell=True)

        #txDis out, nPresent in, LOS in
        subprocess.call('echo 496 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 497 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 498 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 499 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 500 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 501 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 502 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 503 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 504 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 505 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 506 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 507 > /sys/class/gpio/export', shell=True)

	    #48P LED, not used for 48T
        subprocess.call('echo 508 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 509 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 510 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo 511 > /sys/class/gpio/export', shell=True)

	    #txDis out, nPresent in, LOS in
        subprocess.call('echo out > /sys/class/gpio/gpio496/direction', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio497/direction', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio498/direction', shell=True)
        subprocess.call('echo out > /sys/class/gpio/gpio499/direction', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio500/direction', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio501/direction', shell=True)
        subprocess.call('echo out > /sys/class/gpio/gpio502/direction', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio503/direction', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio504/direction', shell=True)
        subprocess.call('echo out > /sys/class/gpio/gpio505/direction', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio506/direction', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio507/direction', shell=True)

        #48P, LED control, 48T not used
        subprocess.call('echo out > /sys/class/gpio/gpio508/direction', shell=True)
        subprocess.call('echo out > /sys/class/gpio/gpio509/direction', shell=True)
        subprocess.call('echo out > /sys/class/gpio/gpio510/direction', shell=True)
        subprocess.call('echo out > /sys/class/gpio/gpio511/direction', shell=True)
        subprocess.call('echo 0 > /sys/class/gpio/gpio508/value', shell=True)
        subprocess.call('echo 0 > /sys/class/gpio/gpio509/value', shell=True)
        subprocess.call('echo 0 > /sys/class/gpio/gpio510/value', shell=True)
        subprocess.call('echo 0 > /sys/class/gpio/gpio511/value', shell=True)

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
                        ('qsa72_48p_sys_cpld', 0x77, 2),
                        ])

        return True