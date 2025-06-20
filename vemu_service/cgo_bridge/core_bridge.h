#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void* vemu_new();
void  vemu_delete(void* inst);

int   vemu_load_hex(void* inst, const char* text, int len);
int   vemu_step(void* inst, uint64_t n, uint64_t* executed);
int   vemu_read(void* inst, uint32_t addr, uint32_t len, uint8_t* out);
int   vemu_write(void* inst, uint32_t addr, uint32_t len, const uint8_t* in);
void  vemu_reset(void* inst);
void  vemu_get_state(void* inst, uint32_t* regs32 /*len=32*/, uint32_t* pc, uint64_t* cycle);
void  vemu_shutdown(void* inst);

#ifdef __cplusplus
}
#endif 