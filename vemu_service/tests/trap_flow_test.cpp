#include "RISCV.h"
#include <cassert>
#include <iostream>

class TestEmu : public Emulator {
public:
    void init_param() override {}
};

int main() {
    TestEmu emu;
    emu.verbose = false;

    // 设置 mtvec 到 0x100
    emu.csr.write(CSR::CSR_MTVEC, 0x100);

    // 指令存储：0x0 处 ebreak，0x100 处 mret
    emu.sram[0] = 0x00100073;          // ebreak
    emu.sram[0x100 / 4] = 0x30200073;  // mret

    emu.pc = 0;
    emu.next_pc = 0;

    // 第一次 emulate，触发 trap
    emu.emulate();
    assert(emu.trap == true);
    assert(emu.csr.read(CSR::CSR_MCAUSE, &emu) == 3);
    assert(emu.csr.read(CSR::CSR_MEPC, &emu) == 0);
    assert(emu.next_pc == 0x100);

    // 跳转到 handler 并执行 mret
    emu.pc = emu.next_pc;
    emu.emulate();
    assert(emu.trap == false);
    assert(emu.next_pc == 0);  // mret 应恢复到 mepc

    std::cout << "Trap flow test passed!" << std::endl;
    return 0;
} 