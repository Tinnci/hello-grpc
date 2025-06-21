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

    /******** Misaligned load test ********/
    // 设置 mtvec = 0x100 再次
    emu.csr.write(CSR::CSR_MTVEC, 0x100);
    // 在 0x0 处写入 lw x1, 1(x0) (不对齐)
    emu.sram[0] = 0x00102083; // imm=1, rd=1, funct3=010, rs1=0, opcode=0000011
    emu.pc = 0;
    emu.trap = false;
    emu.next_pc = 0;
    emu.emulate();
    assert(emu.trap == true);
    assert(emu.csr.read(CSR::CSR_MCAUSE, &emu) == 4); // load misaligned
    assert(emu.csr.read(CSR::CSR_MEPC, &emu) == 0);

    /******** Misaligned store test ********/
    // sw x1, 1(x0)
    emu.sram[0] = 0x001020A3; // imm=1, rs2=1, rs1=0, funct3=010, opcode=0100011
    emu.pc = 0;
    emu.trap = false;
    emu.csr.write(CSR::CSR_MCAUSE, 0);
    emu.emulate();
    assert(emu.trap == true);
    assert(emu.csr.read(CSR::CSR_MCAUSE, &emu) == 6); // store misaligned

    std::cout << "Trap flow test passed!" << std::endl;
    return 0;
} 