# xpu
Based on QEMU emulation PCI device, offload calculation sample program.

device sepc:
- BAR0 32-bit, non-prefetchable, 1M
  - 0x00: ID (reset 0x12345678)
  - 0x10: op1 
  - 0x14: op2
  - 0x18: opcode
    - 0x1: add
    - 0x2: mul
  - 0x30: result
  - 0x40: Interrupt status
  - 0x48: Interrupts ACK
  - 0x80: dma_src
  - 0x88: dma_dst
  - 0x90: dma_len
  - 0x98: dma_cmd
    - Enable, bit[0]: enable DMA.
    - direction, bit[1]: 
      - 0: cpu to device
      - 1: device to cpu
    - IRQ, bit[2]: raise interrupt.
  

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
        Control: I/O- Mem+ BusMaster+ SpecCycle- MemWINV- VGASnoop- ParErr- Stepping- SERR- FastB2B- DisINTx-
        Status: Cap+ 66MHz- UDF- FastB2B- ParErr- DEVSEL=fast >TAbort- <TAbort- <MAbort- >SERR- <PERR- INTx-
        Latency: 128
        Interrupt: pin A routed to IRQ 42
        IOMMU group: 2
        Region 0: Memory at 10000000 (32-bit, non-prefetchable) [size=1M]
        Capabilities: [40] MSI: Enable- Count=1/1 Maskable- 64bit+
                Address: 0000000000000000  Data: 0000
        Kernel driver in use: xpu_driver
        Kernel modules: xpu


[  946.467639] [XPU] PCI BUS probe.
[  946.470295] xpu_driver 0000:00:01.0: latency timer = 128 clocks
[  946.485403] MSI Enabled
[  946.485529] MSI Vector: 1
[  946.485627] IRQ Number: 53
[  946.485697] pdev->irq: 53
[  946.487729] BAR[0]: address=0x10000000
[  946.487866] BAR[0]: length=0x100000
[  946.487957] [config space] [0x0] [0xb7]
[  946.488048] [config space] [0x1] [0x1d]
[  946.488127] [config space] [0x2] [0x3d]
[  946.488226] [config space] [0x3] [0xdc]
[  946.488302] [config space] [0x4] [0x6]
[  946.488497] [config space] [0x5] [0x4]
[  946.488675] [config space] [0x6] [0x10]
[  946.488813] [config space] [0x7] [0x0]
[  946.488911] [config space] [0x8] [0x1]
[  946.488990] [config space] [0x9] [0x0]
[  946.489087] [config space] [0xa] [0xff]
[  946.489265] [config space] [0xb] [0x0]
[  946.489427] [config space] [0xc] [0x0]
[  946.489566] [config space] [0xd] [0x80]
[  946.489713] [config space] [0xe] [0x0]
[  946.489846] [config space] [0xf] [0x0]
[  946.489993] [config space] [0x10] [0x0]
[  946.490174] [config space] [0x11] [0x0]
[  946.490320] [config space] [0x12] [0x0]
[  946.490464] [config space] [0x13] [0x10]
[  946.490609] [config space] [0x14] [0x0]
[  946.490730] [config space] [0x15] [0x0]
[  946.490874] [config space] [0x16] [0x0]
[  946.491011] [config space] [0x17] [0x0]
[  946.491146] [config space] [0x18] [0x0]
[  946.491286] [config space] [0x19] [0x0]
[  946.491426] [config space] [0x1a] [0x0]
[  946.491554] [config space] [0x1b] [0x0]
[  946.491686] [config space] [0x1c] [0x0]
[  946.491811] [config space] [0x1d] [0x0]
[  946.491955] [config space] [0x1e] [0x0]
[  946.492079] [config space] [0x1f] [0x0]
[  946.492231] [config space] [0x20] [0x0]
[  946.492375] [config space] [0x21] [0x0]
[  946.492510] [config space] [0x22] [0x0]
[  946.492654] [config space] [0x23] [0x0]
[  946.492823] [config space] [0x24] [0x0]
[  946.492969] [config space] [0x25] [0x0]
[  946.493102] [config space] [0x26] [0x0]
[  946.493245] [config space] [0x27] [0x0]
[  946.493370] [config space] [0x28] [0x0]
[  946.493463] [config space] [0x29] [0x0]
[  946.493548] [config space] [0x2a] [0x0]
[  946.493634] [config space] [0x2b] [0x0]
[  946.493726] [config space] [0x2c] [0xf4]
[  946.493823] [config space] [0x2d] [0x1a]
[  946.493919] [config space] [0x2e] [0x0]
[  946.494040] [config space] [0x2f] [0x11]
[  946.494167] [config space] [0x30] [0x0]
[  946.494306] [config space] [0x31] [0x0]
[  946.494441] [config space] [0x32] [0x0]
[  946.494571] [config space] [0x33] [0x0]
[  946.494706] [config space] [0x34] [0x40]
[  946.494834] [config space] [0x35] [0x0]
[  946.494974] [config space] [0x36] [0x0]
[  946.495115] [config space] [0x37] [0x0]
[  946.495250] [config space] [0x38] [0x0]
[  946.495374] [config space] [0x39] [0x0]
[  946.495502] [config space] [0x3a] [0x0]
[  946.495635] [config space] [0x3b] [0x0]
[  946.495773] [config space] [0x3c] [0xff]
[  946.495907] [config space] [0x3d] [0x1]
[  946.496035] [config space] [0x3e] [0x0]
[  946.496167] [config space] [0x3f] [0x0]
[  946.496689] xpu_driver 0000:00:01.0: ioremap mmio address: 0xffff000016b00000
[  946.497436] [mmio space] [0x0] [0x12345678]
[  946.497591] [mmio space] [0x4] [0xffffffff]
[  946.497760] [mmio space] [0x8] [0xffffffff]
[  946.497910] [mmio space] [0xc] [0xffffffff]
[  946.498073] [mmio space] [0x10] [0x1]
[  946.498202] [mmio space] [0x14] [0x2]
[  946.498320] [mmio space] [0x18] [0x1]
[  946.498448] [mmio space] [0x1c] [0xffffffff]
[  946.498593] [mmio space] [0x20] [0xffffffff]
[  946.498738] [mmio space] [0x24] [0xffffffff]
[  946.498880] [mmio space] [0x28] [0xffffffff]
[  946.499028] [mmio space] [0x2c] [0xffffffff]
[  946.499194] [mmio space] [0x30] [0x3]
[  946.500756] xpu_driver 0000:00:01.0: CPU address: 0xffff800005e90000
[  946.501191] xpu_driver 0000:00:01.0: DMA address: 0xffe0000
[  946.620888] xpu_irq_handler: irq = 53, irq_status = 4
[  946.656435] [mmio space] [0x80] [0xffe0000]
[  946.658920] [mmio space] [0x88] [0x40000]
[  946.660935] [mmio space] [0x90] [0x4]
[  946.661041] [mmio space] [0x98] [0x4]
[  946.661353] xpu_driver 0000:00:01.0: CPU address: 0xffff800003c80000
[  946.661708] xpu_driver 0000:00:01.0: DMA address: 0xffd0000
[  946.761423] xpu_irq_handler: irq = 53, irq_status = 4
[  946.795784] [mmio space] [0x80] [0x40000]
[  946.796058] [mmio space] [0x88] [0xffd0000]
[  946.796142] [mmio space] [0x90] [0x4]
[  946.796216] [mmio space] [0x98] [0x6]
[  946.796290] [DMA result]: 0x87654321
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
[0x40]: 0xFFFFFFFF
[0x80]: 0xFFFFFFFF
[0x88]: 0xFFFFFFFF
[0x90]: 0xFFFFFFFF
[0x98]: 0xFFFFFFFF
```
