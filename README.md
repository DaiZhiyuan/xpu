# xpu
Based on QEMU emulation PCI device, offload calculation sample program.

## 1. GuestOS environment
```
[root@localhost ~]# uname -a
Linux localhost.localdomain 4.18.0-348.7.1.el8_5.aarch64 #1 SMP Wed Dec 22 13:24:11 UTC 2021 aarch64 aarch64 aarch64 GNU/Linux

[root@localhost ~]# cat /etc/os-release
NAME="CentOS Linux"
VERSION="8 (Core)"
ID="centos"
ID_LIKE="rhel fedora"
VERSION_ID="8"
PLATFORM_ID="platform:el8"
PRETTY_NAME="CentOS Linux 8 (Core)"
ANSI_COLOR="0;31"
CPE_NAME="cpe:/o:centos:centos:8"
HOME_URL="https://www.centos.org/"
BUG_REPORT_URL="https://bugs.centos.org/"

CENTOS_MANTISBT_PROJECT="CentOS-8"
CENTOS_MANTISBT_PROJECT_VERSION="8"
REDHAT_SUPPORT_PRODUCT="centos"
REDHAT_SUPPORT_PRODUCT_VERSION="8"
```

## 2. PCI Device enumeration
```
[root@localhost ~]# lspci
00:00.0 Host bridge: Red Hat, Inc. QEMU PCIe Host bridge
00:01.0 Unclassified device [00ff]: Phytium Technology, Inc. XPU accelerater device (rev 01)
00:03.0 Ethernet controller: Red Hat, Inc. Virtio network device
00:04.0 SCSI storage controller: Red Hat, Inc. Virtio block device
```



## 3. Load Linux Driver(xpu.ko)
```
[root@localhost ~]# lspci -kv -s 00:01.0
00:01.0 Unclassified device [00ff]: Phytium Technology, Inc. XPU accelerater device (rev 01)
        Subsystem: Red Hat, Inc. Device 1100
        Flags: fast devsel, IRQ 42, IOMMU group 2
        Memory at 10000000 (32-bit, non-prefetchable) [size=1M]
        Kernel driver in use: xpu_driver
        Kernel modules: xpu

[root@localhost ~]# modinfo xpu
filename:       /lib/modules/4.18.0-348.7.1.el8_5.aarch64/updates/xpu.ko
license:        GPL
author:         Jerry Dai
description:    XPU Driver
rhelversion:    8.5
srcversion:     1E01AC0804427996C93D0B3
alias:          pci:v00001DB7d0000DC3Dsv*sd*bc*sc*i*
depends:
name:           xpu
vermagic:       4.18.0-348.7.1.el8_5.aarch64 SMP mod_unload modversions aarch64
```

## 4. Runing App

```
[root@localhost xpu_app]# ./xpu_app

XPU accelerater device ID: 0x12345678

[mode]    [op1]  [action]  [op2]  [result]
------------------------------------------
  SW        97      ADD      58      155
  HW        97      ADD      58      155
  SW        97      MUL      58      5626
  HW        97      MUL      58      5626
[root@localhost xpu_app]# ./xpu_app

XPU accelerater device ID: 0x12345678

[mode]    [op1]  [action]  [op2]  [result]
------------------------------------------
  SW        97      ADD      100      197
  HW        97      ADD      100      197
  SW        97      MUL      100      9700
  HW        97      MUL      100      9700
[root@localhost xpu_app]# ./xpu_app

XPU accelerater device ID: 0x12345678

[mode]    [op1]  [action]  [op2]  [result]
------------------------------------------
  SW        1      ADD      28      29
  HW        1      ADD      28      29
  SW        1      MUL      28      28
  HW        1      MUL      28      28
```

## 5. Load uio_pci_generic Driver & run xpu_debug program
```
[root@localhost ~]# modprobe uio_pci_generic
[root@localhost ~]# echo "0000:00:01.0" > /sys/bus/pci/devices/0000\:00\:01.0/driver/unbind
[root@localhost ~]# echo "1db7 dc3d" > /sys/bus/pci/drivers/uio_pci_generic/new_id
[root@localhost ~]# ls -l /sys/bus/pci/devices/0000\:00\:01.0/driver
lrwxrwxrwx. 1 root root 0 10月 23 02:26 /sys/bus/pci/devices/0000:00:01.0/driver -> ../../../bus/pci/drivers/uio_pci_generic

[root@localhost ~]# lspci -vk -s 00:01.0
00:01.0 Unclassified device [00ff]: Phytium Technology, Inc. XPU accelerater device (rev 01)
        Subsystem: Red Hat, Inc. Device 1100
        Flags: fast devsel, IRQ 42, IOMMU group 2
        Memory at 10000000 (32-bit, non-prefetchable) [size=1M]
        Kernel driver in use: uio_pci_generic
        Kernel modules: xpu

[root@localhost xpu_uio]# ./xpu_debug
-----------XPU configure space----------
[Vendor ID]: 0x1DB7
[Device ID]: 0xDC3D
[Command]: 0x0006
        [IO Space Enable]: 0x0000
        [MMIO Space Enable]: 0x0001
        [BUS Master Enable]: 0x0001
        [Interrupt Disable]: 0x0000
[Status]: 0x0010
[Revision ID]: 0x01
[Class Code]: 0x00FF00
[Cache Line]: 0x00
[Latency Timer]: 0x80
[Header Type]: 0x00
[BIST]: 0x00
[Base Address 0]: 0x10000000
[Base Address 1]: 0x00000000
[Base Address 2]: 0x00000000
[Base Address 3]: 0x00000000
[Base Address 4]: 0x00000000
[Base Address 5]: 0x00000000
[CardBus CIS pointer]: 0x00000000
[Subsystem Vendor ID]: 0x1AF4
[Subsystem Device ID]: 0x1100
[Expansion ROM Base Address]: 0x00000000
[Reserved(Capability List)]: 0x00000040
[Reserved]: 0x00000000
[IRQ Line]: 0xFF
[IRQ Pin]: 0x01
[Min_Gnt]: 0x00
[Max_Lat]: 0x00
-----------XPU MMIO space----------
[0x00]: 0x12345678
[0x10]: 0xFFFFFFFF
[0x14]: 0xFFFFFFFF
[0x18]: 0xFFFFFFFF
[0x30]: 0xFFFFFFFF
[0x80]: 0xFFFFFFFF
[0x88]: 0xFFFFFFFF
[0x90]: 0xFFFFFFFF
[0x98]: 0xFFFFFFFF
```
