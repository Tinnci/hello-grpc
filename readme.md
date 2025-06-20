下面给出一个更具可操作性的 Roadmap，用来把 `/vemu_service` 中基于 C++ 的 Venus/RISC-V 仿真器迁移为 **纯 Go + gRPC** 的微服务。整条路线分三阶段，每阶段都可落地到独立 PR，便于在 CI/CD 中逐步验证。  

---

## 阶段 0 准备工作（∼1 天）

1. 新建模块  
   ```bash
   mkdir -p go-vemu/cmd go-vemu/internal
   cd go-vemu
   go mod init github.com/yourorg/go-vemu
   ```
2. 拆出 proto  
   - 把旧 C++ 里的 `*.proto` 移到 `api/v1/`，若没有就按 VemuService 接口草拟（后面会扩展）。  
   - 安装生成器  
     ```bash
     go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
     go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest
     ```
   - 生成代码 `protoc --go_out=. --go-grpc_out=. api/v1/vemu_service.proto`。  

3. 引入脚手架依赖到 `go.mod`：  
   ```bash
   require (
       google.golang.org/grpc v1.63.0
       github.com/rs/zerolog  v1.33.0   // 可换 logrus
   )
   ```

---

## 阶段 1 "外壳先行"——空核心 + gRPC（∼2 天）

1. 在 `internal/core/dummy.go` 定义 **空实现**：  
   ```go
   type Core struct{}
   func (c *Core) LoadHex(_ string) error          { return nil }
   func (c *Core) Step(_ uint64) error             { time.Sleep(1*time.Ms); return nil }
   func (c *Core) ReadMem(addr uint32, n int) []byte  { return make([]byte, n) }
   func (c *Core) WriteMem(addr uint32, b []byte)  {}
   func (c *Core) Reset() {}
   ```
   先跑通 RPC 流程，不关注真实执行。

2. 写 gRPC Server：`cmd/server/main.go`  
   ```go
   lis, _ := net.Listen("tcp", ":50051")
   grpcServer := grpc.NewServer(
        grpc.UnaryInterceptor(loggingInterceptor),
   )
   pb.RegisterVemuServiceServer(grpcServer, server.New(&core.Core{}))
   log.Fatal(grpcServer.Serve(lis))
   ```
3. 写小型 e2e 测试：用 `grpcurl` 或 `buf curl` 调 `LoadFirmware → Execute → ReadMemory`，确保编译、容器化、CI 均通过。  

> **验收标准**：GitHub Action 能编译镜像并跑通 3 个空核心 RPC；Prometheus 指标 /healthz 返回 200。  

---

## 阶段 2 "核心移植"——基于 Go Emulator 模板（∼1–2 周）

1. 选择底座  
   - 建议 fork `deadsy/rv32`（代码量小，授权 MIT）到 `internal/rvcore`。  

2. 覆盖指令集  
   1. 先补齐 RV32IM + CSR：  
      - 对照 `RISCV.cpp` 的大 `switch(opcode)` 表，每遇到缺的就在 Go 里写 `func exec##()`，并注册到解码表：  
        ```go
        decoder[opcodeMask(inst)] = execANDI
        ```  
   2. 再迁 Venus 扩展：  
      - 根据 `venus_ext.cpp` 中 `execute_vector()` 的 `case` 语句，把每个矢量指令拆成独立函数 `execVADD`、`execVSHUFFLE` 等。  
      - 将 DSPM / MASK 等全局数组改为包级变量，类型用 `[]uint16 / []bool`。  

3. 内存与外设  
   - 用一个大 `[]byte ram` 处理 SRAM；  
   - 用接口  
     ```go
     type Device interface{ Load(addr uint32) uint32; Store(addr uint32, val uint32) }
     ```  
     注册 UART、Timer 设备；读写时查表分发。  

4. 性能小技巧  
   - 热路径函数贴 `//go:nosplit` 可减少调度开销；  
   - 把解码表从 `map` 换成 `[128]func()` 的直接索引。  
   - benchmark：`go test -bench=. ./internal/rvcore`。  

> **验收标准**：能跑 riscv-tests `isa/rv32ui-p-add` 全通过；速度 ≥ 50 MIPS（Apple M1 单核）。  

---

## 阶段 3 "接口对齐 + 灰度"——替换原 C++ 服务（∼1 周）

1. 与旧进程对拍  
   - 写 comparer：同时跑 C++ core 和 Go core；喂同样 ELF，每步比对 PC、寄存器、内存，直到成功跑完 UART 输出 "PASS"。  
   - 若差异，则在 Go 侧开 trace (`-tags debug`) 定位。  

2. 微服务平滑切换  
   1. 在 Helm / docker-compose 里并排启动 `vemu-cpp`、`vemu-go`；  
   2. 上层调用方通过 envoy / nginx 根据 Header `X-Vemu-Canary: true` 转到 Go 版；  
   3. 30 min 无异常后，100 % 流量切至 Go，删除 C++ Pod。  

3. 清理 & 文档  
   - 移除 cgo / old service；  
   - 在 `README.md` 记录编译、调试、性能数据、与 Venus 原论文的兼容表；  
   - 在 `buf.yaml` 声明 proto 兼容规则。  

> **最终交付**：`go-vemu` 镜像 < 15 MB（distroless）；平均请求延迟 < 1 ms；所有老客户端无感迁移。  

---

### 常见坑与解决

| 问题 | 现象 | 对策 |
|------|------|------|
| 解码误差 | PC 跳飞、陷入死循环 | 用 `cpu.Trace` 打印 inst/pc，对照 C++ `printf` 结果 |
| CSR 写入发生竞态 | 并发 RPC 时寄存器意外变 | `server` 中对 `core` 加 `sync.Mutex`，或 Channel 串行 |
| gRPC 大包复制多 | `ReadMemory 16 MB` 占用内存 | 开流式 `rpc ReadMemory(stream Req) returns (stream Resp)` |
| Slice 越界 | panic 出 nowritebarrier | 建议 `-gcflags="all=-d=checkptr"` 提前发现 |

## 附录 A RISC-V RV32IM / IRQ / Venus ISA 速览

### 1. 标准 RV32IM 执行路径
Emulator 每拍按以下流程工作：
1. 取指：`instr = sram[pc>>2]`
2. 依据低 7 位 *opcode* 进入 `decode_*`（LUI / JAL / LOAD / MUL …）。
3. 解析寄存器字段与立即数，写回 `cpuregs` 并计算 `next_pc`。
4. 自增 `cycle_count / instr_count`。

### 2. IRQ 微扩展（opcode `0001011`）
6 条 SoC 辅助指令简化了中断/队列模型：
| `funct7` | 助记符 | 作用 |
|----------|--------|------|
| 0000000  | `getq`   | 读软件任务队列→通用寄存器 |
| 0000001  | `setq`   | 写队列（rd+32） |
| 0000010  | `retirq` | `pc ← cpuregs`; 清 `irq_active` |
| 0000100  | `waitirq`| 低功耗等待直到中断到来 |
| 0000011  | `maskirq`| 交换 `irq_mask` 与寄存器，实现全局屏蔽 |
| 0000101  | `timer`  | 读/写 软定时器计数器 |

### 3. Venus 向量/定点扩展（opcode `1011011`）
Venus 自定义扩展在 `venus_execute()` 中解释执行，核心要点如下。

#### 3.1 片上向量 SRAM (DSPM)
```c
uint16_t VENUS_DSPM[ROW][LANE];   // 数据
bool     VENUS_MASK_DSPM[ROW][LANE]; // 掩码位
```

#### 3.2 解码后的参数结构
```
avl, vmask_read, vew, function3,
function5, scalar_op, vfu_shamt,
vs1/2_head, vd1/2_head
```

#### 3.3 指令族概览（一行 = 8/16-bit 并行）
- **Bit-ALU**：VAND / VOR / VXOR / VSLL / VSR{L,A} / VMNOT / VSHUFFLE
- **比较/掩码**：VSEQ / VSNE / VS{L,G}T(U) / VS{L,G}E(U)
- **饱和 & 普通算术**：VADD / VSADD(U) / VSUB / VSSUB(U) / VRSUB / VRANGE
- **乘法 & MAC**：VMUL* / VMULADD / VMULSUB / VADDMUL（`vfu_shamt` 决定定点右移）

#### 3.4 执行示例：`VADD (IVV)`
```
for i := 0; i < avl; i++ {
    if !mask[i] && vmask_read == 1 { continue }
    res := vs1[i] + vs2[i]   // 8/16-bit, 可饱和
    if mask[i] { vd[i] = res }
}
```

### 4. 与官方 RISC-V Vector(V) 的差异
1. 固定 SEW = 8 / 16，无 LMUL/GROUP 概念。
2. 向量数据存放于片上 SRAM 而非寄存器堆。
3. 指令语义极简，无 strip-mine/Ta/Tu；mask 仅一位布尔。
4. 编码完全私有，仅占用单一 *opcode*，便于硬件快速解码。

---

## 小结

1. 先跑通 **空核心 + gRPC** 框架，再逐步把指令集、外设、内存"填肉"；  
2. 核心复用现有 Go RISC-V 项目可大幅缩短迁移时间；  
3. 通过并行对拍、灰度发布可把风险控制在一个 Sprint 内；  
4. 最终得到 **全 Go、易于容器化、可水平扩缩的仿真云服务**，彻底摆脱 C++ 交叉编译和 ABI 烦恼。祝迁移顺利!