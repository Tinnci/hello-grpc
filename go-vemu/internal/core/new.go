//go:build !cgo
// +build !cgo

package core

// New 返回默认 Core 实现。
// 目前返回 Dummy；当编译时启用 cgo 标签且实现完成后，可切换为 cgoCore。
func New() Core {
	return NewDummy()
}
