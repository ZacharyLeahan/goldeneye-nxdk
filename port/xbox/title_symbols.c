/* i386 COFF decorates C names with an underscore. title.c takes the
 * addresses of these segment-2 labels. The software RSP resolves them
 * against the base registered by the game's gSPSegment command. */
#define SEGMENT_LABEL(name, addr) \
    __asm__(".globl _" #name "\n.set _" #name ", " #addr)
SEGMENT_LABEL(D_020043E8, 0x020043e8);
SEGMENT_LABEL(DL_RAREWARETEXT, 0x020044b0);
SEGMENT_LABEL(D_02004758, 0x02004758);
SEGMENT_LABEL(D_02004FE8, 0x02004fe8);
SEGMENT_LABEL(D_02005FF0, 0x02005ff0);
