#include "venus_ext.h"

// Task 7.2: Venus_Emulator specialized MMU routing
// 对于地址位于 BLOCK_VRF_OFFSET 及以上的区域，映射到 VSPM；
// 其他地址回退到基类 Emulator 的实现（即 SRAM/MMIO）。

namespace {
// 帮助函数：统一物理地址归一化
static inline uint32_t to_physical(const Venus_Emulator* cpu, uint32_t addr) {
    return (addr >= cpu->BLOCK_PERSPECTIVE_BASEADDR) ?
           (addr - cpu->BLOCK_PERSPECTIVE_BASEADDR) : addr;
}
}

uint32_t Venus_Emulator::read_word(uint32_t addr) {
    uint32_t phys = to_physical(this, addr);
    if (phys >= this->BLOCK_VRF_OFFSET) {
        return static_cast<uint32_t>(lw_from_vspm((phys - this->BLOCK_VRF_OFFSET) >> 2));
    }
    // 默认：走基类 SRAM / MMIO
    return Emulator::read_word(addr);
}

void Venus_Emulator::write_word(uint32_t addr, uint32_t data) {
    uint32_t phys = to_physical(this, addr);
    if (phys >= this->BLOCK_VRF_OFFSET) {
        st_to_vspm((phys - this->BLOCK_VRF_OFFSET) >> 2, static_cast<int32_t>(data));
        return;
    }
    Emulator::write_word(addr, data);
}

int8_t Venus_Emulator::read_byte(int32_t addr) {
    uint32_t uaddr = static_cast<uint32_t>(addr);
    uint32_t phys = to_physical(this, uaddr);
    if (phys >= this->BLOCK_VRF_OFFSET) {
        uint32_t word = lw_from_vspm((phys - this->BLOCK_VRF_OFFSET) >> 2);
        uint8_t shift = (phys & 0x3u) * 8;
        return static_cast<int8_t>((word >> shift) & 0xFF);
    }
    return Emulator::read_byte(addr);
}

uint8_t Venus_Emulator::read_byte_u(uint32_t addr) {
    return static_cast<uint8_t>(read_byte(static_cast<int32_t>(addr)));
}

int16_t Venus_Emulator::read_half(int32_t addr) {
    uint32_t uaddr = static_cast<uint32_t>(addr);
    uint32_t phys = to_physical(this, uaddr);
    if (phys >= this->BLOCK_VRF_OFFSET) {
        uint32_t word = lw_from_vspm((phys - this->BLOCK_VRF_OFFSET) >> 2);
        uint8_t shift = (phys & 0x2u) * 8; // half-word alignment
        return static_cast<int16_t>((word >> shift) & 0xFFFF);
    }
    return Emulator::read_half(addr);
}

uint16_t Venus_Emulator::read_half_u(uint32_t addr) {
    return static_cast<uint16_t>(read_half(static_cast<int32_t>(addr)));
}

void Venus_Emulator::write_byte(uint32_t addr, uint8_t data) {
    uint32_t phys = to_physical(this, addr);
    if (phys >= this->BLOCK_VRF_OFFSET) {
        uint32_t idx = (phys - this->BLOCK_VRF_OFFSET) >> 2;
        uint32_t word = lw_from_vspm(static_cast<int>(idx));
        uint8_t shift = (phys & 0x3u) * 8;
        uint32_t newword = (word & ~(0xFFu << shift)) | (static_cast<uint32_t>(data) << shift);
        st_to_vspm(static_cast<int>(idx), static_cast<int32_t>(newword));
        return;
    }
    Emulator::write_byte(addr, data);
}

void Venus_Emulator::write_half(uint32_t addr, uint16_t data) {
    uint32_t phys = to_physical(this, addr);
    if (phys >= this->BLOCK_VRF_OFFSET) {
        uint32_t idx = (phys - this->BLOCK_VRF_OFFSET) >> 2;
        uint32_t word = lw_from_vspm(static_cast<int>(idx));
        uint8_t shift = (phys & 0x2u) * 8;
        uint32_t newword = (word & ~(0xFFFFu << shift)) | (static_cast<uint32_t>(data) << shift);
        st_to_vspm(static_cast<int>(idx), static_cast<int32_t>(newword));
        return;
    }
    Emulator::write_half(addr, data);
}
