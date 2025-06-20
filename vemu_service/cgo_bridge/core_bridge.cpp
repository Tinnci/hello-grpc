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

// Stop reasons are now defined in core_bridge.h

static int load_hex_from_buffer(Emu* e, const char* text, int len){
    if(!e || !text || len<=0) return -1;
    size_t arraySize = 0;

    const char* ptr = text;
    const char* end = text + len;
    char line[256];
    while(ptr < end){
        // 复制一行到 line
        int idx = 0;
        while(ptr < end && *ptr!='\n' && *ptr!='\r' && idx < 255){
            line[idx++] = *ptr++;
        }
        // 跳过行结束符
        while(ptr < end && (*ptr=='\n' || *ptr=='\r')) ptr++;
        line[idx] = '\0';
        if(idx==0) continue; // 空行

        uint32_t value = 0;
        if(sscanf(line, "%x", &value) != 1){
            continue; // 非法行忽略
        }

        // 根据 Emulator::init_emulator 的逻辑决定放入位置
        if(arraySize <= (e->TEXT_SECTION_LENGTH/4) - 1){
            e->sram[arraySize] = value;
        }
        if(arraySize >= (e->DATA_SECTION_OFFSET/4) &&
           arraySize <= ((e->DATA_SECTION_LENGTH + e->DATA_SECTION_OFFSET)/4)){
            size_t dstIdx = (e->BLOCK_DSPM_OFFSET/4) + (arraySize - (e->DATA_SECTION_OFFSET/4));
            e->sram[dstIdx] = value;
        }
        arraySize++;
        if(arraySize >= CPU::SRAMSIZE){
            break;
        }
    }
    return 0;
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
    Emu* e = reinterpret_cast<Emu*>(inst);
    // 先初始化参数，与 init_emulator 中的行为保持一致
    e->init_param();
    e->pc = e->PROGADDR_RESET;
    e->trap = false;
    e->ebreak = false;

    return load_hex_from_buffer(e, text, len);
}

int vemu_run(void* inst, uint64_t n, const uint32_t* bps, int bps_len, uint64_t* executed, trace_cb cb, void* user_data) {
    if (!inst) return -1;
    Emu* e = reinterpret_cast<Emu*>(inst);
    uint64_t done = 0;
    std::unordered_set<uint32_t> breakpoints(bps, bps + bps_len);

    for (; done < n || n == 0; ++done) {
        if (e->paused_.load(std::memory_order_relaxed)) {
            if (executed) *executed = done;
            return VEMU_PAUSED;
        }

        e->emulate();

        if (cb) {
            cb(user_data, e->counter.cycle_count, e->pc);
        }

        if (e->ebreak || e->trap) {
            ++done;
            if (executed) *executed = done;
            return e->ebreak ? VEMU_EBREAK : VEMU_TRAP;
        }

        if (!breakpoints.empty() && breakpoints.count(e->pc)) {
            ++done;
            if (executed) *executed = done;
            return VEMU_BP;
        }
    }

    if (executed) *executed = done;
    return VEMU_OK;
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