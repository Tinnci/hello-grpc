//go:build cgo
// +build cgo

package core

func New() Core {
	c, err := newCGO()
	if err != nil {
		// 在初始化时遇到错误属于严重情况，直接 panic。
		panic(err)
	}
	return c
}
