#include "game/ob.h"
#include "system.h"
#include <string.h>

extern fileentry file_resource_table[];
extern resource_lookup_data_entry resource_lookup_data_array[];
extern s32 file_entry_max;

typedef struct {
    u32 kind, original, offset, length;
} XboxResource;

static const XboxResource *resources;
static const u8 *payload;
static u32 count;

void xboxMountResources(u8 *cart, u32 cartSize, const u8 *pack, u32 size)
{
    if (size < 8 || memcmp(pack, "GE32", 4)) sysFatalError("Invalid Xbox resource pack");
    count = ((const u32 *)pack)[1];
    if (count > (size - 8) / sizeof(XboxResource)) sysFatalError("Invalid Xbox resource count");
    resources = (const XboxResource *)(pack + 8);
    u32 start = (8 + count * sizeof(XboxResource) + 15) & ~15u;
    if (start > size) sysFatalError("Invalid Xbox resource index");
    payload = pack + start;
    for (u32 i = 0; i < count; ++i) {
        const XboxResource *r = &resources[i];
        if (r->original >= cartSize || r->offset > size - start ||
            r->length > size - start - r->offset || r->kind > 1)
            sysFatalError("Invalid Xbox resource %u", i);
        if (!r->kind) {
            if (r->length > cartSize - r->original) sysFatalError("Invalid bank length");
            memcpy(cart + r->original, payload + r->offset, r->length);
        }
    }
}

void pcmodelsPatchTable(void)
{
    for (int t = 1; t < file_entry_max; ++t) {
        for (u32 i = 0; i < count; ++i) {
            const XboxResource *r = &resources[i];
            if (r->kind && (uintptr_t)file_resource_table[t].hw_address == 0x10000000 + r->original) {
                file_resource_table[t].hw_address = (u8 *)payload + r->offset;
                resource_lookup_data_array[t].rom_size = r->length;
                break;
            }
        }
    }
}
