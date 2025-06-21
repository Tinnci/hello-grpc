#include "RISCV.h"
#include "decoder/CsrInstruction.h"
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>

using namespace Decoder;
class TestEmu : public Emulator { public: void init_param() override {} };

struct Case { uint8_t funct3; const char* name; };
static const Case cases[] = {
    {0b001, "csrrw"}, {0b010, "csrrs"}, {0b011, "csrrc"},
    {0b101, "csrrwi"}, {0b110, "csrrsi"}, {0b111, "csrrci"},
};

static uint16_t random_csr() {
    return 0x300 + (std::rand() % 0xFF);
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    const int iterations = 300;

    for (const auto &c : cases) {
        for (int i = 0; i < iterations; ++i) {
            auto em = std::make_unique<TestEmu>();
            em->verbose = false;

            uint16_t csrAddr = random_csr();
            uint32_t initVal = static_cast<uint32_t>(std::rand());
            em->csr.write(csrAddr, initVal);

            uint32_t rs1Val = static_cast<uint32_t>(std::rand());
            uint8_t rs1 = 1;
            uint8_t rd = 5;
            em->cpuregs[rs1] = rs1Val;
            uint8_t zimm = std::rand() & 0x1F;

            uint32_t word = (c.funct3 << 12) | (csrAddr << 20) | (rd << 7) | 0b1110011;
            if (c.funct3 >= 0b101) {
                word |= (zimm << 15);
            } else {
                word |= (rs1 << 15);
            }

            CsrInstruction inst(word);
            inst.execute(em.get());

            uint32_t oldVal = initVal;
            uint32_t expectedNew = initVal;
            bool doWrite = false;
            switch (c.funct3) {
            case 0b001: expectedNew = rs1Val; doWrite = true; break;
            case 0b010: expectedNew = initVal | rs1Val; doWrite = (rs1Val!=0); break;
            case 0b011: expectedNew = initVal & (~rs1Val); doWrite = (rs1Val!=0); break;
            case 0b101: expectedNew = zimm; doWrite = true; break;
            case 0b110: expectedNew = initVal | zimm; doWrite = (zimm!=0); break;
            case 0b111: expectedNew = initVal & (~zimm); doWrite = (zimm!=0); break;
            }
            uint32_t csrAfter = em->csr.read(csrAddr, em.get());
            if (doWrite) assert(csrAfter == expectedNew);
            else assert(csrAfter == oldVal);
            assert(em->cpuregs[rd] == oldVal);
        }
        std::cout << "Case " << c.name << " passed." << std::endl;
    }
    std::cout << "All CSR tests passed!" << std::endl;
    return 0;
} 