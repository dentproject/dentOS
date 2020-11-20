How to build ONL for ACCTON arm64 platforms
----------------------------------------
$ cd OpenNetworkLinux
$ export VERSION=9
$ make docker
ONL installer will be generated in builds/arm64/installer/installed/builds directory.


How to install ONL via ONIE
----------------------------------------
There are two ways to install ONL as below
- Put ONL installer under the service directory of TFTP server
- Put ONL installer on /dev/sda1, /dev/sda2 or /dev/sda3 under the root directory
with name onie-installer-arm64.bin or onie-installer.bin
e.g. /dev/sda1/onie-installer-arm64-delta_tn4810m.bin
ONIE will automatically search the installer from /dev/sda(1/2/3) or download it
from TFTP server, then run the installer to install ONL to target storage drive.


ACCTON arm64 platforms supported in ONL
------------------------------------
AS4224-52P     (install on /dev/sda, SSD card)


ONL Linux LTS-4.9 features to be verified on ACCTON arm64 platforms
----------------------------------------------------------
-Linux SMP
-Linux reboot
-GIC
-Generic Timer
-SMMU
-Flextimer
-IFC NOR
-IFC NAND
-DSPI
-QSPI
-UART
-I2C
-I2C RTC
-USB
-PCIe
-SATA
-SDHC
-MC bus
-MC console
-VFIO
-DPAA Ethernet
-DPAA2 Ethernet
