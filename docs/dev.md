==== FIXME ===
Finish this doc

== Adding a new package

* Create most of the files in the 'any' architecture for cross compiling
* Copy the directory structure from an existing package
    * APKG.yml goes into the $(ONL)/packages/base/any/foo/ directory
    * PKG.yml goes into the $(ONL)/packages/base/$ARCH/foo directory
        * Create for each $ARCH you intend to support
* Put code in $(ONL)/packages/base/any/foo/src
* Put package specific Makefiles in $(ONL)/packages/base/any/builds

* Run `make rebuild` in $(ONL)/packages/base/$ARCH/foo to rebuild the package cache
    * particularly if you see an error like:
      """ERROR:onlpm:'Package all does not exist.'"""

/dev/mvsw_fw_comm_debug - Character device for firmware debugging
=================================================================

Description
-----------
The `/dev/mvsw_fw_comm_debug` character device allows sending commands from user
mode to the firmware running on the secondary CPU. This device is intended for
debugging purposes and provides a limited set of commands to retrieve information
about the firmware's status and execution environment.

Usage
-----
The device accepts commands through the `write` system call, and the output is
retrieved using the `read` system call. The "help" command lists all available
commands and their descriptions.

Command examples
----------------

### help
The "help" command lists all available commands and their descriptions.

```
# echo "help" > mvsw_fw_comm_debug
# cat mvsw_fw_comm_debug
Available commands:
 help - shows this prompt
 system_metrics - shows system metrics from FW CPU
 kernel_logs  - shows logs from kernel ring buffer
 cpss_logs - shows logs of appDemo service
 kernel_logs/cpss_logs <reset> - resets the pointer to the beginning of the logs
 exec_luacli <command> - executes <command> in luaCLI in FW CPU
```

### system_metrics
Retrieves system metrics from the firmware CPU, including memory, CPU, and filesystem usage.

```
root@netprod-dentlab-sea55-mendel-infra-sw1:/dev# echo "system_metrics" > mvsw_fw_comm_debug
root@netprod-dentlab-sea55-mendel-infra-sw1:/dev# cat mvsw_fw_comm_debug
              total        used        free      shared  buff/cache   available
Mem:            997          49         825         112         123         816
Swap:             0           0           0
Filesystem                Size      Used Available Use% Mounted on
devtmpfs                496.7M         0    496.7M   0% /dev
tmpfs                   498.7M      4.0K    498.7M   0% /dev/shm
tmpfs                   498.7M    112.8M    385.9M  23% /tmp
tmpfs                   498.7M     20.0K    498.7M   0% /run
Mem: 176684K used, 844648K free, 115524K shrd, 0K buff, 123076K cached
CPU:  10% usr  10% sys   0% nic  80% idle   0% io   0% irq   0% sirq
Load average: 0.23 0.34 0.36 1/67 1284
  PID  PPID USER     STAT   VSZ %VSZ %CPU COMMAND
 1239     1 root     S     2928   0%   0% /sbin/getty -L console 0 vt100
    1     0 root     S     2924   0%   0% init
 1196     1 root     S     2924   0%   0% /sbin/klogd -n
 1192     1 root     S     2924   0%   0% /sbin/syslogd -n
```


### exec_luacli
Executes a command in the luaCLI environment running on the firmware CPU.

```
# echo "exec_luacli ?" > mvsw_fw_comm_debug
# cat mvsw_fw_comm_debug
Entering LuaCLI took 0.004775 msec

 LUA CLI based on LUA 5.1 from www.lua.org
 LUA CLI uses Mini-XML engine from www.minixml.org
***************************************************
               LUA CLI shell ready
***************************************************

Console# ?
  autoInitSystem             Auto Init system (determines init params according to PCI/PEX scanning)
  clear                      Reset functions
  cls                        clear screen
  configure                  Enter configuration mode
  cpss-api
  cpssInitSystem             Init system
  cpssLsSmi                  Lists the SMI devices
  cpssLspci                  Lists the PCI devices
  cpssPciProvision           Insert Pci data of needed device, device number and hw device number
  cpssPciRemove              Remove Pex device from sysfs
  cpssPciRescan              rescan Pex devices
  cpssPpInsert               Insert Packet Processor
  cpssPpRemove               Remove Packet Processor
  cpssPpShowDevices          Lists the Pp devices and PCI info
  cpssSystemBaseInit         Init CPSS Base system- device independent
  cpssinitsystem             Init system
  dbg                        Allows to execute debug command while in other contexts
  debug-mode                 Exit from the EXEC to debug mo
  ...
```


Security Considerations
-----------------------
For security reasons, the list of commands available through `/dev/mvsw_fw_comm_debug` is limited to a predefined subset. This restriction is enforced to prevent execution of potentially harmful commands on the firmware CPU.
