# What is this?

This is a POC for an issue with the SB2 parsing in the LPC55S69 version 1B ROM.
Parts of the header are not checked carefully which allows for a buffer
buffer overflow. From this buffer overflow:

- We overwrite the start of where heap allocation occurs
- We continue copying bytes into the executable SRAM area (`0x14005a00`)
- One we're done copying, the SB2 code allocates from our specified heap address
- The SB2 code copies our bogus header to the heap allocated address
- The next time it goes through the processing loop it jumps to our specified
  address at `0x14005a00`
- The assembly at `0x14005a00` disables the MPU/SAU/NXP's security
- The code jumps to what should be a non-executable address `0x14005400`

```
pyocd> reg
general registers:
      lr: 0x14005a4f                   r7: 0x00000010 (16)          
      pc: 0x14005400                   r8: 0x14003434 (335557684)   
      r0: 0x0000aaaa (43690)           r9: 0x14001478 (335549560)   
      r1: 0x500acff8 (1342885880)     r10: 0x00000200 (512)         
      r2: 0x00000000 (0)              r11: 0x00000170 (368)         
      r3: 0x00080103 (524547)         r12: 0x00000000 (0)           
      r4: 0x000000a0 (160)             sp: 0x30005f08               
      r5: 0x14005401 (335565825)     xpsr: 0x49000000 (1224736768)  
      r6: 0x00000170 (368) 
```

# Caveats

This doesn't work out of the box with NXP's ISP tools. We overwrite the global
state of the stack canary as part of our extended buffer overrun which gets
detected when the code goes back up to ISP to get another packet.

This PoC includes a minimal implementation of the ISP protocol to get around
this. The code sends everything up to the stack canary and then overwrites
the canary with the last packet.

# Usage

`make run` will build and run everything

`header.c` has the code to build the buggy header
`serial.c` is a minimal implementation of ISP over serial
`assembly.S` contains the payload that gets loaded to the executable area
at `0x14005a00`

# Further work

Add a file to load more assembly at the specified address
