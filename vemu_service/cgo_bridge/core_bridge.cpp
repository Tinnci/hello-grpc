#include "core_bridge.h"
#include <cstring>
#include <vector>

struct DummyCore {
    uint32_t regs[32] = {0};
    uint32_t pc = 0;
    uint64_t cycle = 0;
    std::vector<uint8_t> mem;
};

void* vemu_new() {
    return new DummyCore();
}

void vemu_delete(void* inst) {
    delete static_cast<DummyCore*>(inst);
}

int vemu_load_hex(void* inst, const char* text, int len) {
    (void)inst; (void)text; (void)len;
    return 0; // success
}

int vemu_step(void* inst, uint64_t n, uint64_t* executed) {
    auto* core = static_cast<DummyCore*>(inst);
    core->cycle += n;
    core->pc += 4 * n;
    if (executed) *executed = n;
    return 0;
}

int vemu_read(void* inst, uint32_t addr, uint32_t len, uint8_t* out) {
    auto* core = static_cast<DummyCore*>(inst);
    if (addr + len > core->mem.size()) core->mem.resize(addr + len);
    std::memcpy(out, core->mem.data() + addr, len);
    return 0;
}

int vemu_write(void* inst, uint32_t addr, uint32_t len, const uint8_t* in) {
    auto* core = static_cast<DummyCore*>(inst);
    if (addr + len > core->mem.size()) core->mem.resize(addr + len);
    std::memcpy(core->mem.data() + addr, in, len);
    return 0;
}

void vemu_reset(void* inst) {
    auto* core = static_cast<DummyCore*>(inst);
    std::memset(core->regs, 0, sizeof(core->regs));
    core->pc = 0;
    core->cycle = 0;
}

void vemu_get_state(void* inst, uint32_t* regs32, uint32_t* pc, uint64_t* cycle) {
    auto* core = static_cast<DummyCore*>(inst);
    std::memcpy(regs32, core->regs, sizeof(core->regs));
    if (pc) *pc = core->pc;
    if (cycle) *cycle = core->cycle;
}

void vemu_shutdown(void* /*inst*/) {} 