#include "core_bridge.h"
#include "venus_ext.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>

using Emu = Venus_Emulator;

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

int vemu_step(void* inst, uint64_t n, uint64_t* executed){
    if(!inst) return -1;
    Emu* e = reinterpret_cast<Emu*>(inst);
    uint64_t done=0;
    for(; done<n || n==0; ++done){
        e->emulate();
        if(e->ebreak || e->trap) { ++done; break; }
    }
    if(executed) *executed = done;
    return 0;
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

// --- Stubs for new APIs ---

int vemu_read_vector(void* inst, uint32_t row, uint32_t elems, uint32_t* values) {
    (void)inst; (void)row; (void)elems; (void)values;
    return -1; // Not implemented
}
int vemu_write_vector(void* inst, uint32_t row, uint32_t elems, const uint32_t* values) {
    (void)inst; (void)row; (void)elems; (void)values;
    return -1; // Not implemented
}
int vemu_get_csr(void* inst, uint32_t id, uint32_t* value) {
    (void)inst; (void)id; (void)value;
    return -1; // Not implemented
}
int vemu_set_csr(void* inst, uint32_t id, uint32_t value) {
    (void)inst; (void)id; (void)value;
    return -1; // Not implemented
} 