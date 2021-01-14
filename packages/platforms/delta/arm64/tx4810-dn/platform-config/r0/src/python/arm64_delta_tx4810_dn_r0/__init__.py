#!/usr/bin/python

from onl.platform.base import *
from onl.platform.delta import *

from fcntl import ioctl
import socket
import struct

class OnlPlatform_arm64_delta_tx4810_dn_r0(OnlPlatformDelta,
                                           OnlPlatformPortConfig_48x10):
    PLATFORM='arm64-delta-tx4810-dn-r0'
    MODEL="TX4810-DN"
    SYS_OBJECT_ID=".48.14"

    def baseconfig(self):
        # Insert platform drivers
        self.insmod("arm64-delta-tx4810-dn-cpld.ko")
        self.insmod("arm64-delta-tx4810-dn-led.ko")

        # Insert the drivers of Mellanox Firmware Tools (MFT)
        self.insmod("mst_pci.ko")
        self.insmod("mst_pciconf.ko")

        ########### initialize I2C bus 1 ###########
        self.new_i2c_devices (
            [
                # FAN Controller
                ('adt7473', 0x2e, 1),

                # CPLD
                ('tx4810_dn_cpld', 0x41, 1),

                # Temperature devices
                ('tmp1075', 0x4a, 1),
                ('tmp1075', 0x4b, 1),
                ('tmp1075', 0x4c, 1),

                # System EEPROM
                ('24c64', 0x56, 1),
            ]
        )

        # Set MUX default idle state to MUX_IDLE_DISCONNECT (-2) to avoid i2c bus conflicts bewteen mux channels
        subprocess.call('echo "-2" > /sys/bus/i2c/devices/i2c-0/0-0070/idle_state', shell=True)

        # Set OOB LED behavior by the TX4810_DN HW SPEC
        # - Amber OFF: Link is 10Mbps/100Mbps
        # - Amber ON:  Link is 1Gbps
        sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        ioctl(sock.fileno(), 0x8949, struct.pack('16s4H', 'ma1', 0x00, 0x16, 0x03, 0x00))
        ioctl(sock.fileno(), 0x8949, struct.pack('16s4H', 'ma1', 0x00, 0x10, 0x1187, 0x00))
        ioctl(sock.fileno(), 0x8949, struct.pack('16s4H', 'ma1', 0x00, 0x16, 0x00, 0x00))

        return True
