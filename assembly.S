fake:
# arg0 = vtor
movw r0, 0x5400
movt r0, 0x1400
# arg1 = entry point
movw r1, 0x5531
movt r1, 0x1400
# arg2 = app_stack
movw r2, 0x4000
movt r2, 0x2000

# now our branch
movw r5, 0x1653
movt r5, 0x1301
blx r5
