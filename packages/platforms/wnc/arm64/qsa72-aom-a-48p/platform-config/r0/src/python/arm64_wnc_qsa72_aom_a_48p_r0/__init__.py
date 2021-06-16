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
        self.insmod("qsa72-aom-a-48p-sfp_plus_cpld")
        self.insmod("qsa72-aom-a-48p-gpio_i2c")
        self.insmod("optoe")

        self.new_i2c_devices([
                ('qsa72_48p_sfp_cpld2', 0x74, 0),
                ('lm75', 0x48, 6),
                ('lm75', 0x49, 7),
                ('lm75', 0x4A, 8),
                ])

        port_num = [11, 12, 13, 14]

        #CP_MPP 6 CPLD1 interrupt
        subprocess.call('echo 38 > /sys/class/gpio/export', shell=True)
        subprocess.call('echo in > /sys/class/gpio/gpio38/direction', shell=True)
        subprocess.call('echo falling > /sys/class/gpio/gpio38/edge', shell=True)

        #SFP CPLD defulat direction
        subprocess.call('echo 0xb6 > /sys/bus/i2c/devices/0-0074/dir_1', shell=True)
        subprocess.call('echo 0xfd > /sys/bus/i2c/devices/0-0074/dir_2', shell=True)

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
                        ('qsa72_48p_sys_cpld', 0x77, 0),
                        ])

        # Insert Marvell prestera modules by only probing prestera_pci module
        self.modprobe('prestera_pci')

        # set up systemctl rules
        for swp in range(1, 49):
           cmd = "systemctl enable switchdev-online@swp%d" % swp
        subprocess.check_call(cmd, shell=True)

        for port in port_num:
            self.new_i2c_devices([
                ('optoe2', 0x50, port),
                ])

        return True