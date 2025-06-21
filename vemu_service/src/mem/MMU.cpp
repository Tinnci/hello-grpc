#include "mem/MMU.h"
#include "RISCV.h"
#include <cstdio> // Added for fputc, fprintf, stderr

using namespace Mem;

static inline uint32_t normalize(uint32_t addr) {
    return (addr >= 0x80000000u) ? addr - 0x80000000u : addr;
}

uint32_t MMU::read_word(uint32_t addr) const {
    if (addr & 0x3) {
        // Load address misaligned (cause 4)
        const_cast<Emulator*>(cpu_)->raise_trap(4);
        return 0;
    }
    return cpu_->read_word(addr);
}

void MMU::write_word(uint32_t addr, uint32_t data) {
    if (addr & 0x3) {
        cpu_->raise_trap(6); // Store/AMO address misaligned (cause 6)
        return;
    }
    cpu_->write_word(addr, data);
}

int8_t MMU::read_byte(int32_t addr) const {
    return cpu_->read_byte(addr);
}

uint8_t MMU::read_byte_u(uint32_t addr) const {
    return cpu_->read_byte_u(addr);
}

int16_t MMU::read_half(int32_t addr) const {
    if (addr & 0x1) {
        const_cast<Emulator*>(cpu_)->raise_trap(4);
        return 0;
    }
    return cpu_->read_half(addr);
}

uint16_t MMU::read_half_u(uint32_t addr) const {
    return cpu_->read_half_u(addr);
}

void MMU::write_byte(uint32_t addr, uint8_t data) {
    cpu_->write_byte(addr, data);
}

void MMU::write_half(uint32_t addr, uint16_t data) {
    if (addr & 0x1) {
        cpu_->raise_trap(6);
        return;
    }
    cpu_->write_half(addr, data);
} 