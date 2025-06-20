#ifndef VEMU_CORE_BRIDGE_H
#define VEMU_CORE_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Stop reason codes, must match go-vemu/internal/core/core.go
#define VEMU_OK      0
#define VEMU_EBREAK  1
#define VEMU_BP      2
#define VEMU_PAUSED  3
#define VEMU_TRAP    4

// Opaque pointer to the emulator instance
void* vemu_new();
void  vemu_delete(void* inst);

typedef void (*trace_cb)(void* user_data, uint64_t cycle, uint32_t pc);

int   vemu_load_hex(void* inst, const char* txt, int len);
int   vemu_run(void* inst, uint64_t n, const uint32_t* bps, int bps_len, uint64_t* executed, trace_cb cb, void* user_data);
int   vemu_read(void* inst, uint32_t addr, uint32_t len, uint8_t* out, char** error_msg);
int   vemu_write(void* inst, uint32_t addr, uint32_t len, const uint8_t* in, char** error_msg);
void  vemu_reset(void* inst);
void  vemu_get_state(void* inst, uint32_t* regs32, uint32_t* pc, uint64_t* cycle);
void  vemu_shutdown(void* inst);
void  vemu_pause(void* inst, int p);
int   vemu_set_reg(void* inst, uint32_t idx, uint32_t val, char** error_msg);

// Vector ops
int vemu_read_vector(void* inst, uint32_t row, uint32_t elems, uint32_t* values);
int vemu_write_vector(void* inst, uint32_t row, uint32_t elems, const uint32_t* values);

// CSR access
int vemu_get_csr(void* inst, uint32_t id, uint32_t* value, char** error_msg);
int vemu_set_csr(void* inst, uint32_t id, uint32_t value, char** error_msg);

#ifdef __cplusplus
}
#endif

#endif // VEMU_CORE_BRIDGE_H 