# 下一步任务计划

## 第一步：代码重构 (Refactoring) - 指令解码与执行解耦

### 为什么要重构？
1.  **巨大的 `switch` 语句**: `venus_ext.cpp` 和 `RISCV.cpp` 中的执行函数依赖巨大的 `switch` 语句，难以维护和扩展。
2.  **解码与执行耦合**: 指令的解码和执行逻辑紧密耦合，违反“单一职责原则”。
3.  **代码重复**: 不同指令格式的字段提取逻辑存在重复。
4.  **扩展性差**: 添加新指令流程繁琐，易错。

### 如何重构？(建议方案)
1.  **创建指令类 (Instruction Classes)**:
    *   定义抽象基类 `Instruction`，包含虚函数 `virtual void execute(Emulator* state) = 0;`。
    *   为每种指令格式（R-Type, I-Type, S-Type, B-Type, U-Type, J-Type）创建派生子类。
    *   每个子类在构造时解析特有字段。
2.  **创建一个解码器 (Decoder)**:
    *   创建 `Decoder` 类或函数，职责是接收指令码，返回相应 `Instruction` 子类实例的指针。
    *   解码器内部包含 `switch`（基于 `opcode`），仅返回 `std::make_unique<SpecificInstruction>(word);`。
3.  **改造主执行循环**:
    *   `emulate()` 循环将变为：fetch指令码 → 调用 `decoder.decode()` → 调用 `instruction->execute(this)` → 更新PC。

### 重构细节和建议
*   **最小可行重构**: 从最常用、依赖最多的 R-Type 指令开始，逐步迁移。
*   **目录结构**: 在 `vemu_service/src/` 新建 `decoder/` 目录，放置 `decoder.h/.cpp`、`instruction.h/.cpp`、`r_type.h/.cpp` 等。
*   **对照测试**: 每次迁移一个 opcode 系列时，加一组对照测试，对比迁移前后的寄存器/内存快照。
*   **PC 更新**: 让每条指令类提供 `uint32_t next_pc(uint32_t old_pc) const`，处理不同指令的 PC 计算方式。

## 第二步：添加新功能 (指令)

在完成重构后，添加新指令将变得简单。

### 优先级建议
1.  **完善标准 CSR (Zicsr 扩展)**:
    *   **原因**: 实现操作系统、特权级切换、中断、异常处理的基础。
    *   **要做什么**: 实现 `CSRRW`, `CSRRS`, `CSRRC` 指令的 `Instruction` 子类；在 `Emulator` 中添加 CSR 寄存器文件；模拟关键 CSR（`mstatus`, `mie`, `mip`, `mcause`, `mepc`, `mtvec`）。先实现“最小系统”需要的几组 CSR。
2.  **实现原子操作 (A 扩展)**:
    *   **原因**: 支持多线程和同步原语的基础。
    *   **要做什么**: 为 `LR.W`, `SC.W` 及各种 `AMO` 指令创建 `Instruction` 子类；在 `Emulator` 中维护“保留集”状态。
3.  **实现浮点扩展 (F/D 扩展)**:
    *   **原因**: 运行需要浮点计算的程序。
    *   **要做什么**: 在 `Emulator` 中添加浮点寄存器文件；为所有 F 和 D 扩展的指令创建 `Instruction` 子类。这是一个庞大任务，可分步完成。
4.  **实现压缩指令 (C 扩展)**:
    *   **原因**: 提高代码密度。
    *   **要做什么**: 对**解码器**进行重大修改，`fetch` 阶段识别 16 位和 32 位指令并更新 PC；解码器需能将 16 位指令“解压”成等效的 32 位指令对象执行，或创建专门的 `Instruction` 子类。建议放在后面。

## 第三步：关于工作流：务必使用 Branch！

这是软件开发中至关重要的一步，绝不直接在 `master` 分支上进行大的改动。

### 为什么要使用 Branch？
1.  **隔离性 (Isolation)**: 在独立分支上进行重构，不影响 `master` 分支的稳定性。
2.  **代码审查 (Code Review)**: 进行代码审查（Pull Request / Merge Request）的前提。
3.  **并行开发**: 必不可少。
4.  **持续集成 (CI)**: 确保改动在合并回主线之前通过所有自动化测试。

### 建议的工作流程
1.  **创建重构分支**: `git checkout -b refactor/instruction-dispatch` (或更具体的名称)。
2.  **进行重构**: 在新分支上，按照建议进行代码重构，频繁地、小批量地提交。
3.  **推送并创建PR**: 重构完成后，将分支推送到远程仓库，并创建一个 Pull Request 到 `main` 分支。
4.  **审查和合并**: 等待 CI 通过，邀请他人审查代码，通过后合并回 `main`。
5.  **为新功能创建新分支**: `git checkout main` → `git pull` → `git checkout -b feature/zicsr-extension`。
6.  在此新分支上，基于已重构的代码添加功能。
7.  **重复此过程**: 每个新功能或大的 bug 修复都应在独立分支中进行。
8.  **CI 验证**: 在 PR 阶段跑 `go test -cover` + `ctest`（若 C++ 有单测）；对性能敏感的指令可加基准测试。

## 与 CGo 的集成

重构不会破坏现有的 Go/C++ 混合架构。

1.  **确保和 CGo 的正常相互调用**:
    *   **核心思想**: 接口与实现分离。
    *   **`core_bridge.h` 稳定**: 这个头文件是公共 API，函数签名保持不变。建议在文件顶部加 WARNING 注释。
    *   **`core_bridge.cpp` 内部实现修改**: 重构将改变这个文件的内部实现，但不会改变它暴露给外面的接口。Go 代码通过 CGo 调用 `vemu_run` 时，不会感知到 C++ 内部的变化。
2.  **还会编译成 `libvemu.a` 吗？**:
    *   **是的，最终产物依然是 `libvemu.a`。**
    *   **构建目标不变**: 编译产物由构建系统（CMake 或 Make）决定，只要不修改构建脚本的目标，就会一直生成 `libvemu.a`。
    *   **更新源文件列表**: 在构建脚本（`Makefile` 或 `CMakeLists.txt`）中，将所有新创建的 `.cpp` 文件添加到编译列表里。建议使用 `target_sources(libvemu PRIVATE decoder/*.cpp instruction/*.cpp ...)`。
    *   **Go 的链接不受影响**: Go 代码中的 CGo 指令指定链接 `libvemu.a`，只要库文件正确生成和放置，Go 的链接过程完全一样。
    *   **代码规范**: 建议顺手加 `clang-tidy` / `cpplint` 到 CI。

## ✅ 进度概览（2025-06）
目前已完成的重要里程碑：
1. **Task 5 － 标准陷阱流程**
   * 清理旧 `rdcycle`/`decode_IRQ` 等死代码。
   * 新增 `raise_trap()`，实现 `EBREAK/MRET`，并扩展 `mepc/mcause/mtvec/mstatus` 等 CSR。
   * 端到端 `trap_flow_test` 全绿通过。
2. **Task 6 － 内存子系统与 Load/Store 迁移**
   * 引入 `Mem::MMU`（支持字节/半字/字访问，含 `PRINTF_ADDR` MMIO）。
   * 迁移全部 `LB/LH/LW/LBU/LHU/SB/SH/SW` 指令到新解码器。
   * `memory_access_test` 1000 轮随机验证通过。
3. **Task 7 － Venus 地址空间整合与旧代码清理**
   * 通过在 `Emulator` 增加虚函数读写接口 + `MMU` 委托，实现统一的多态内存访问模型。
   * `Venus_Emulator` 覆写内存接口，透明支持 VSPM / SPM 地址；`decode_load/store` 旧实现全部下线。
   * 合并 `memory_access_test`，随机验证 SRAM/MMIO 与 VSPM 均 1000 轮通过。
   * 编译警告清零，代码量精简 >2 K 行，仓库更加整洁。

> 以上改动均已合并到 `refactor/instruction-dispatch` 分支并通过全部 Go/C++ 回归测试。

---

## 总结与行动计划

**行动路线图:**
1.  **立即新建一个 `refactor/instruction-dispatch` 分支。**
2.  **在该分支上，投入时间进行代码重构**，将指令解码和执行分离。
3.  **重构完成后，通过 Pull Request 合并回 `main`。**
4.  **接着，为下一个你想实现的功能（例如 CSR）再创建一个新分支**，享受在清晰架构上添加功能的乐趣。

### 渐进式里程碑
*   **里程碑 1**: R-Type 重构 + 旧流程 fallback，通过所有单元测试。
*   **里程碑 2**: I-Type/B-Type/S-Type 完成迁移，删除旧 switch。
*   **里程碑 3**: CSR 基础子集实现，可跑简易 OS 的 `ecall`, `mret`。
*   **里程碑 4**: LR/SC + AMO.w，支持多线程同步原语。 