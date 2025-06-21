#include "csr/CsrFile.h"
#include "RISCV.h" // for Emulator

uint32_t CsrFile::read(uint16_t addr, const Emulator* cpu) const {
    switch (addr) {
    case CSR::CSR_CYCLE:
        return static_cast<uint32_t>(cpu->counter.cycle_count & 0xFFFFFFFF);
    case CSR::CSR_CYCLEH:
        return static_cast<uint32_t>((cpu->counter.cycle_count >> 32) & 0xFFFFFFFF);
    case CSR::CSR_INSTRET:
        return static_cast<uint32_t>(cpu->counter.instr_count & 0xFFFFFFFF);
    case CSR::CSR_INSTRETH:
        return static_cast<uint32_t>((cpu->counter.instr_count >> 32) & 0xFFFFFFFF);
    default: {
        auto it = storage_.find(addr);
        return it == storage_.end() ? 0 : it->second;
    }
    }
}

void CsrFile::write(uint16_t addr, uint32_t value) {
    // 对计数器 CSR 写入忽略
    if (addr == CSR::CSR_CYCLE || addr == CSR::CSR_CYCLEH ||
        addr == CSR::CSR_INSTRET || addr == CSR::CSR_INSTRETH) {
        return;
    }
    storage_[addr] = value;
} 