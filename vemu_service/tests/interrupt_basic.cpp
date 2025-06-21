#include "RISCV.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <cstdlib>
#include <ctime>

using namespace CSR;
class TestEmu : public Emulator { public: void init_param() override {} };

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    const int iterations = 300;

    for (int i = 0; i < iterations; ++i) {
        auto em = std::make_unique<TestEmu>();
        // 写入一条 NOP 指令到 SRAM[0]
        em->sram[0] = 0x00000013; // ADDI x0,x0,0
        em->pc = 0;

        // 随机决定是否启用全局中断
        bool enable_mie = std::rand() & 1;
        uint32_t mstatus = enable_mie ? MSTATUS_MIE : 0;
        em->csr.write(CSR_MSTATUS, mstatus);

        // 设置 mtvec 为 0x100
        em->csr.write(CSR_MTVEC, 0x100);

        // 随机决定是否产生外部中断
        bool set_irq = std::rand() & 1;
        if (set_irq) {
            // 使能 MEIE
            em->csr.write(CSR_MIE, MIE_MEIE);
            // 置位挂起 MEIP
            em->csr.write(CSR_MIP, MIP_MEIP);
        } else {
            em->csr.write(CSR_MIE, 0);
            em->csr.write(CSR_MIP, 0);
        }

        em->emulate(); // 执行一条指令并进行中断检查

        if (enable_mie && set_irq) {
            // 期望跳转到 mtvec
            assert(em->next_pc == 0x100);
            // mepc 应保存原 PC
            assert(em->csr.read(CSR_MEPC, em.get()) == 0);
            // mcause 要有最高位 =1 且低位 11
            assert(em->csr.read(CSR_MCAUSE, em.get()) == 0x8000000B);
        } else {
            // 无中断，PC 应正常递增 4
            assert(em->next_pc == 4);
        }
    }

    std::cout << "Interrupt basic test passed!" << std::endl;
    return 0;
} 