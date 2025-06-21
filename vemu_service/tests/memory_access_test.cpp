#include "RISCV.h"
#include "venus_ext.h"
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory> // Added for std::make_unique

class TestEmu : public Emulator {
public:
    void init_param() override {}
};

static uint32_t rand32() {
    return (static_cast<uint32_t>(std::rand()) << 16) ^ std::rand();
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    const int iterations = 1000;
    for (int i = 0; i < iterations; ++i) {
        auto emu = std::make_unique<TestEmu>();
        emu->verbose = false;

        // random base address within SRAM (byte addressable) but leave last 8 bytes safe
        uint32_t byte_addr = (rand32() % (CPU::SRAMSIZE * 4 - 8));
        uint32_t half_addr = byte_addr & ~0x1u; // ensure half align varies
        uint32_t word_addr = byte_addr & ~0x3u;

        uint8_t  b_val  = static_cast<uint8_t>(rand32() & 0xFFu);
        uint16_t h_val  = static_cast<uint16_t>(rand32() & 0xFFFFu);
        uint32_t w_val  = rand32();

        // --- store byte (SB) then LB/LBU ---
        emu->mmu.write_byte(byte_addr, b_val);
        int8_t   lb  = emu->mmu.read_byte(byte_addr);
        uint8_t  lbu = emu->mmu.read_byte_u(byte_addr);
        assert(static_cast<uint8_t>(lb) == b_val);
        assert(lbu == b_val);

        // --- store half (SH) then LH/LHU ---
        emu->mmu.write_half(half_addr, h_val);
        int16_t  lh  = emu->mmu.read_half(half_addr);
        uint16_t lhu = emu->mmu.read_half_u(half_addr);
        assert(static_cast<uint16_t>(lh) == h_val);
        assert(lhu == h_val);

        // --- store word (SW) then LW ---
        emu->mmu.write_word(word_addr, w_val);
        uint32_t lw = emu->mmu.read_word(word_addr);
        assert(lw == w_val);

        // Verify SRAM content reflects expected composites
        uint32_t check_word = emu->sram[word_addr >> 2];
        assert(check_word == emu->mmu.read_word(word_addr));

        /* -------- VSPM 测试 -------- */
        {
            auto vemu = std::make_unique<Venus_Emulator>();
            vemu->verbose = false;
            uint32_t word_index = rand32() % 1024; // 随机选取 4KB 以内 word
            uint32_t base_addr = vemu->BLOCK_PERSPECTIVE_BASEADDR + vemu->BLOCK_VRF_OFFSET + word_index * 4;

            uint32_t vw_val = rand32();
            uint16_t vh_val = static_cast<uint16_t>(rand32() & 0xFFFFu);
            uint8_t  vb_val = static_cast<uint8_t>(rand32() & 0xFFu);

            vemu->mmu.write_word(base_addr, vw_val);
            assert(vemu->mmu.read_word(base_addr) == vw_val);

            vemu->mmu.write_half(base_addr, vh_val);
            assert(vemu->mmu.read_half_u(base_addr) == vh_val);

            vemu->mmu.write_byte(base_addr, vb_val);
            assert(vemu->mmu.read_byte_u(base_addr) == vb_val);
        }
    }

    std::cout << "Memory + VSPM access test passed (" << iterations << " iterations)." << std::endl;
    return 0;
} 