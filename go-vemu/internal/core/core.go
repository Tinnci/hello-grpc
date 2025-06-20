package core

import v1 "github.com/yourorg/go-vemu/api/v1"

// Core 抽象，RPC 层只依赖该接口
// 后续可替换为纯 Go、cgo、JIT 等不同实现
// 所有方法必须是并发安全的(由实现自行保证或由调用方序列化)
type Core interface {
	Reset()
	LoadHex(hexText []byte) error
	// Step 执行 cycles，返回真正执行的周期数
	Step(cycles uint64) (executed uint64, err error)
	ReadMem(addr uint32, length uint32) ([]byte, error)
	WriteMem(addr uint32, data []byte) error
	GetState() *v1.CpuState
	Shutdown()
	// Register access
	GetRegs() ([]uint32, error)
	SetReg(index uint32, value uint32) error

	// Vector ops
	ReadVector(row, elems uint32) ([]uint32, error)
	WriteVector(row uint32, values []uint32) error

	// CSR access
	GetCSR(id uint32) (uint32, error)
	SetCSR(id, value uint32) error
}
