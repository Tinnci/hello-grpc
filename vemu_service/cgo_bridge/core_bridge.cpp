#include "core_bridge.h"
#include "venus_ext.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>
#include <unordered_set>
#include <vector>

using Emu = Venus_Emulator;

// Stop reasons for vemu_run, must match core.go
const int STOP_REASON_DONE = 0;
const int STOP_REASON_BREAKPOINT = 1;
const int STOP_REASON_PAUSED = 2;
const int STOP_REASON_EBREAK = 3;
const int STOP_REASON_HOST_TRAP = 4;

static std::string write_hex_to_tmp(const char* txt, int len) {
    char tmpl[] = "/tmp/vemu_hex_XXXXXX";
    int fd = mkstemp(tmpl);
    if(fd == -1) return "";
    FILE* fp = fdopen(fd, "w");
    fwrite(txt, 1, len, fp);
    fclose(fp);
    return std::string(tmpl);
}

void* vemu_new(){
    Emu* e = new Emu();
    e->init_param();
    return reinterpret_cast<void*>(e);
}

void vemu_delete(void* inst){
    delete reinterpret_cast<Emu*>(inst);
}

int vemu_load_hex(void* inst, const char* text, int len){
    if(!inst) return -1;
    std::string path = write_hex_to_tmp(text,len);
    if(path.empty()) return -2;
    reinterpret_cast<Emu*>(inst)->init_emulator(const_cast<char*>(path.c_str()));
    return 0;
}

int vemu_run(void* inst, uint64_t n, const uint32_t* bps, int bps_len, uint64_t* executed, trace_cb cb, void* user_data) {
    if (!inst) return -1;
    Emu* e = reinterpret_cast<Emu*>(inst);
    uint64_t done = 0;
    std::unordered_set<uint32_t> breakpoints(bps, bps + bps_len);

    for (; done < n || n == 0; ++done) {
        if (e->paused_.load(std::memory_order_relaxed)) {
            if (executed) *executed = done;
            return STOP_REASON_PAUSED;
        }

        e->emulate();

        if (cb) {
            cb(user_data, e->counter.cycle_count, e->pc);
        }

        if (e->ebreak || e->trap) {
            ++done;
            if (executed) *executed = done;
            return e->ebreak ? STOP_REASON_EBREAK : STOP_REASON_HOST_TRAP;
        }

        if (!breakpoints.empty() && breakpoints.count(e->pc)) {
            ++done;
            if (executed) *executed = done;
            return STOP_REASON_BREAKPOINT;
        }
    }

    if (executed) *executed = done;
    return STOP_REASON_DONE;
}

static inline bool is_scalar(uint32_t addr){ return addr < 0x100000; }

int vemu_read(void* inst, uint32_t addr, uint32_t len, uint8_t* out){
    Emu* e = reinterpret_cast<Emu*>(inst);
    for(uint32_t i=0;i<len;++i){
        uint32_t byte_addr = addr + i;
        uint32_t word = e->sram[byte_addr/4];
        out[i] = (word >> ((byte_addr%4)*8)) & 0xFF;
    }
    return 0;
}

int vemu_write(void* inst, uint32_t addr, uint32_t len, const uint8_t* in){
    Emu* e = reinterpret_cast<Emu*>(inst);
    for(uint32_t i=0;i<len;++i){
        uint32_t byte_addr = addr + i;
        uint32_t& word = e->sram[byte_addr/4];
        uint32_t off = (byte_addr%4)*8;
        word &= ~(0xFFu<<off);
        word |= (static_cast<uint32_t>(in[i])<<off);
    }
    return 0;
}

void vemu_reset(void* inst){
    Emu* e = reinterpret_cast<Emu*>(inst);
    e->init_param();
}

void vemu_get_state(void* inst, uint32_t* regs32, uint32_t* pc, uint64_t* cycle){
    Emu* e = reinterpret_cast<Emu*>(inst);
    for(int i=0;i<32;++i) regs32[i]=e->cpuregs[i];
    if(pc) *pc = e->pc;
    if(cycle) *cycle = e->counter.cycle_count;
}

void vemu_shutdown(void* inst){ (void)inst; }

void vemu_pause(void* inst, int p) {
    if (!inst) return;
    reinterpret_cast<Emu*>(inst)->paused_.store(p != 0, std::memory_order_relaxed);
}

int vemu_set_reg(void* inst, uint32_t idx, uint32_t val) {
    if (!inst || idx >= 32) return -1;
    Emu* e = reinterpret_cast<Emu*>(inst);
    e->cpuregs[idx] = val;
    return 0;
}

// --- Stubs for new APIs ---

int vemu_read_vector(void* inst, uint32_t row, uint32_t elems, uint32_t* values) {
    Emu* e = reinterpret_cast<Emu*>(inst);
    for (uint32_t i = 0; i < elems; ++i) {
        values[i] = e->lw_from_vspm(row * 128 + i); // Assuming row-major layout
    }
    return 0; 
}
int vemu_write_vector(void* inst, uint32_t row, uint32_t elems, const uint32_t* values) {
    Emu* e = reinterpret_cast<Emu*>(inst);
    for (uint32_t i = 0; i < elems; ++i) {
        e->st_to_vspm(row * 128 + i, values[i]);
    }
    return 0;
}
int vemu_get_csr(void* inst, uint32_t id, uint32_t* value) {
    Emu* e = reinterpret_cast<Emu*>(inst);
    if (id < 32) { // General registers
        *value = e->cpuregs[id];
        return 0;
    }
    // TODO: Handle other CSRs
    return -1;
}
int vemu_set_csr(void* inst, uint32_t id, uint32_t value) {
    Emu* e = reinterpret_cast<Emu*>(inst);
     if (id < 32) {
        e->cpuregs[id] = value;
        return 0;
    }
    // TODO: Handle other CSRs
    return -1;
} 