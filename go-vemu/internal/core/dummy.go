package core

import (
	"time"

	v1 "github.com/yourorg/go-vemu/api/v1"
)

// Dummy 实现，方便早期联调
// 所有操作都做最小动作或返回静态数据

type Dummy struct{}

func NewDummy() Core { return &Dummy{} }

func (d *Dummy) Reset() {}

func (d *Dummy) Shutdown() {}

func (d *Dummy) LoadHex(_ []byte) error { return nil }

func (d *Dummy) Step(_ uint64) (uint64, error) {
	time.Sleep(1 * time.Millisecond)
	return 1, nil
}

func (d *Dummy) ReadMem(_ uint32, l uint32) ([]byte, error) { return make([]byte, l), nil }
func (d *Dummy) WriteMem(_ uint32, _ []byte) error          { return nil }

func (d *Dummy) GetState() *v1.CpuState {
	return &v1.CpuState{Pc: 0x0, Cycle: 0}
}
