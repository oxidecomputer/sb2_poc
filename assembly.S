fake:
// set our store address
//movw r0, 0x0
//movt r0, 0x1400
// meh this is easier and we have space
//movw r5, 0x6f45
//movt r5, 0x1300
//blx r5
// now go to our loop
movw r5, 0xae71
movt r5, 0x1301
bx r5
