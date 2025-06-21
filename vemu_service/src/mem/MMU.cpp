#include "mem/MMU.h"
#include "RISCV.h"
#include <cstdio> // Added for fputc, fprintf, stderr

using namespace Mem;

static inline uint32_t normalize(uint32_t addr) {
    return (addr >= 0x80000000u) ? addr - 0x80000000u : addr;
}

uint32_t MMU::read_word(uint32_t addr) const {
    if (addr == cpu_->PRINTF_ADDR || addr == cpu_->TEST_PRINTF_ADDR) {
        return 0; // MMIO write-only regions, read returns 0
    }
    uint32_t norm = normalize(addr);
    return cpu_->sram[norm >> 2];
}

void MMU::write_word(uint32_t addr, uint32_t data) {
    if (addr == cpu_->PRINTF_ADDR) {
        fputc(static_cast<char>(data & 0xFF), stderr);
        return;
    }
    if (addr == cpu_->TEST_PRINTF_ADDR) {
        if (data == 123456789) fprintf(stderr, "ALL TESTS PASSED.\n");
        else fprintf(stderr, "ERROR!\n");
        return;
    }
    uint32_t norm = normalize(addr);
    cpu_->sram[norm >> 2] = data;
}

int8_t MMU::read_byte(int32_t addr) const {
    uint32_t norm = normalize(addr);
    uint32_t word = cpu_->sram[norm >> 2];
    uint8_t shift = (norm & 0x3) * 8;
    return static_cast<int8_t>((word >> shift) & 0xFF);
}

uint8_t MMU::read_byte_u(uint32_t addr) const {
    return static_cast<uint8_t>(read_byte(addr));
}

int16_t MMU::read_half(int32_t addr) const {
    uint32_t norm = normalize(addr);
    uint32_t word = cpu_->sram[norm >> 2];
    uint8_t shift = (norm & 0x2) * 8;
    return static_cast<int16_t>((word >> shift) & 0xFFFF);
}

uint16_t MMU::read_half_u(uint32_t addr) const {
    return static_cast<uint16_t>(read_half(addr));
}

void MMU::write_byte(uint32_t addr, uint8_t data) {
    if (addr == cpu_->PRINTF_ADDR) { fputc(static_cast<char>(data), stderr); return; }
    uint32_t norm = normalize(addr);
    uint32_t &word = cpu_->sram[norm >> 2];
    uint8_t shift = (norm & 0x3) * 8;
    word = (word & ~(0xFFu << shift)) | (static_cast<uint32_t>(data) << shift);
}

void MMU::write_half(uint32_t addr, uint16_t data) {
    uint32_t norm = normalize(addr);
    uint32_t &word = cpu_->sram[norm >> 2];
    uint8_t shift = (norm & 0x2) * 8;
    word = (word & ~(0xFFFFu << shift)) | (static_cast<uint32_t>(data) << shift);
} 