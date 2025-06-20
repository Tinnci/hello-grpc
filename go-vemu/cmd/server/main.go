package main

import (
	"context"
	"log"
	"net"
	"sync"

	"google.golang.org/grpc"

	pb "github.com/yourorg/go-vemu/api/v1"
	"github.com/yourorg/go-vemu/internal/core"
)

type execCmd struct {
	cycles      uint64
	single_step bool
	reply       chan<- execResult
	ctx         context.Context
}

type execResult struct {
	executed uint64
	err      error
	reason   int
}

type vemuServer struct {
	pb.UnimplementedVemuServiceServer
	mu          sync.Mutex
	core        core.Core
	execCh      chan execCmd
	traceCh     chan *pb.TraceEvent
	stopCh      chan struct{}
	breakpoints map[uint32]struct{}
}

const runBatchSize = 1000

func newServer() *vemuServer {
	s := &vemuServer{
		core:        core.New(),
		execCh:      make(chan execCmd, 1),
		traceCh:     make(chan *pb.TraceEvent, 100),
		stopCh:      make(chan struct{}),
		breakpoints: make(map[uint32]struct{}),
	}
	s.core.SetTraceChan(s.traceCh)
	go s.execLoop()
	return s
}

func (s *vemuServer) execLoop() {
	for {
		select {
		case cmd := <-s.execCh:
			s.mu.Lock()
			currentBreakpoints := s.breakpoints
			s.mu.Unlock()

			var totalExecuted uint64
			var finalReason int
			var finalErr error

			cyclesToRun := cmd.cycles
			if cyclesToRun == 0 && !cmd.single_step {
				cyclesToRun = ^uint64(0) // "infinite"
			} else if cmd.single_step {
				cyclesToRun = 1
			}

		runLoop:
			for totalExecuted < cyclesToRun {
				// Check for cancellation before starting a new batch
				if cmd.ctx != nil && cmd.ctx.Err() != nil {
					finalReason = core.StopReasonPaused // Treat cancellation like a pause
					break runLoop
				}

				batchCycles := cyclesToRun - totalExecuted
				if !cmd.single_step && batchCycles > runBatchSize {
					batchCycles = runBatchSize
				}
				if batchCycles == 0 {
					break
				}

				executed, reason, err := s.core.Run(batchCycles, currentBreakpoints)
				totalExecuted += executed
				finalReason = reason
				finalErr = err

				if reason != core.StopReasonDone || err != nil {
					break runLoop
				}
			}

			if cmd.reply != nil {
				cmd.reply <- execResult{executed: totalExecuted, err: finalErr, reason: finalReason}
			}

		case <-s.stopCh:
			return
		}
	}
}

// ────────────── 生命周期 ──────────────
func (s *vemuServer) Reset(ctx context.Context, _ *pb.Empty) (*pb.Status, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.core.Pause(true)
	s.core.Reset()
	s.breakpoints = make(map[uint32]struct{})
	return &pb.Status{Ok: true}, nil
}

func (s *vemuServer) Shutdown(ctx context.Context, _ *pb.Empty) (*pb.Status, error) {
	s.core.Shutdown()
	close(s.stopCh)
	return &pb.Status{Ok: true}, nil
}

// ────────────── LoadFirmware ──────────────
func (s *vemuServer) LoadFirmware(ctx context.Context, r *pb.LoadFirmwareRequest) (*pb.Status, error) {
	if err := s.core.LoadHex(r.HexText); err != nil {
		return &pb.Status{Ok: false, Message: err.Error()}, nil
	}
	return &pb.Status{Ok: true}, nil
}

// ────────────── Step ──────────────
func (s *vemuServer) Step(ctx context.Context, r *pb.StepRequest) (*pb.StepResponse, error) {
	s.core.Pause(false)
	replyCh := make(chan execResult, 1)
	s.execCh <- execCmd{cycles: r.Cycles, single_step: true, reply: replyCh, ctx: ctx}
	res := <-replyCh
	return &pb.StepResponse{CyclesExecuted: res.executed}, res.err
}

// ────────────── QueryState ──────────────
func (s *vemuServer) QueryState(ctx context.Context, _ *pb.Empty) (*pb.CpuState, error) {
	return s.core.GetState(), nil
}

// ────────────── Memory ──────────────
func (s *vemuServer) ReadMemory(ctx context.Context, r *pb.ReadMemRequest) (*pb.ReadMemResponse, error) {
	data, err := s.core.ReadMem(r.Addr, r.Length)
	if err != nil {
		return nil, err
	}
	return &pb.ReadMemResponse{Data: data}, nil
}

func (s *vemuServer) WriteMemory(ctx context.Context, r *pb.WriteMemRequest) (*pb.Status, error) {
	if err := s.core.WriteMem(r.Addr, r.Data); err != nil {
		return &pb.Status{Ok: false, Message: err.Error()}, nil
	}
	return &pb.Status{Ok: true}, nil
}

// ────────────── Register access ──────────────
func (s *vemuServer) GetRegs(ctx context.Context, _ *pb.Empty) (*pb.RegisterFile, error) {
	regs, err := s.core.GetRegs()
	if err != nil {
		return nil, err
	}
	return &pb.RegisterFile{Regs: regs, Pc: s.core.GetState().Pc}, nil
}

func (s *vemuServer) SetReg(ctx context.Context, req *pb.SetRegRequest) (*pb.Status, error) {
	if err := s.core.SetReg(req.Index, req.Value); err != nil {
		return &pb.Status{Ok: false, Message: err.Error()}, nil
	}
	return &pb.Status{Ok: true}, nil
}

// ────────────── Vector / CSR ops ──────────────
func (s *vemuServer) ReadVector(ctx context.Context, r *pb.ReadVectorRequest) (*pb.ReadVectorResponse, error) {
	vals, err := s.core.ReadVector(r.Row, r.Elems)
	if err != nil {
		return nil, err
	}
	return &pb.ReadVectorResponse{Values: vals}, nil
}

func (s *vemuServer) WriteVector(ctx context.Context, r *pb.WriteVectorRequest) (*pb.Status, error) {
	err := s.core.WriteVector(r.Row, r.Values)
	if err != nil {
		return &pb.Status{Ok: false, Message: err.Error()}, nil
	}
	return &pb.Status{Ok: true}, nil
}

func (s *vemuServer) GetCSR(ctx context.Context, r *pb.GetCsrRequest) (*pb.GetCsrResponse, error) {
	val, err := s.core.GetCSR(r.Id)
	if err != nil {
		return nil, err
	}
	return &pb.GetCsrResponse{Value: val}, nil
}

func (s *vemuServer) SetCSR(ctx context.Context, r *pb.SetCsrRequest) (*pb.Status, error) {
	err := s.core.SetCSR(r.Id, r.Value)
	if err != nil {
		return &pb.Status{Ok: false, Message: err.Error()}, nil
	}
	return &pb.Status{Ok: true}, nil
}

// ────────────── Run / Pause ──────────────
func (s *vemuServer) Run(ctx context.Context, r *pb.RunRequest) (*pb.RunResponse, error) {
	s.core.Pause(false)
	replyCh := make(chan execResult, 1)
	s.execCh <- execCmd{cycles: r.MaxCycles, reply: replyCh, ctx: ctx}

	res := <-replyCh

	// Keep computing the deprecated ebreak for backward compatibility
	ebreak := res.reason == core.StopReasonBp || res.reason == core.StopReasonEbreak || res.reason == core.StopReasonPaused

	return &pb.RunResponse{
		CyclesExecuted: res.executed,
		Reason:         uint32(res.reason),
		Ebreak:         ebreak,
	}, res.err
}

func (s *vemuServer) Pause(ctx context.Context, _ *pb.Empty) (*pb.Status, error) {
	s.core.Pause(true)
	return &pb.Status{Ok: true}, nil
}

// ────────────── TraceStream ──────────────
func (s *vemuServer) TraceStream(req *pb.Empty, stream pb.VemuService_TraceStreamServer) error {
	for {
		select {
		case event := <-s.traceCh:
			if err := stream.Send(event); err != nil {
				return err
			}
		case <-stream.Context().Done():
			return nil
		case <-s.stopCh:
			return nil
		}
	}
}

// ────────────── LoadBinaryBlob ──────────────
func (s *vemuServer) LoadBinaryBlob(ctx context.Context, r *pb.LoadBinaryRequest) (*pb.Status, error) {
	if err := s.core.WriteMem(r.Addr, r.Data); err != nil {
		return &pb.Status{Ok: false, Message: err.Error()}, nil
	}
	return &pb.Status{Ok: true}, nil
}

// ────────────── 断点 ──────────────
func (s *vemuServer) SetBreakpoint(ctx context.Context, r *pb.BreakpointRequest) (*pb.Status, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.breakpoints[r.Pc] = struct{}{}
	return &pb.Status{Ok: true}, nil
}

func (s *vemuServer) ClearBreakpoint(ctx context.Context, r *pb.BreakpointRequest) (*pb.Status, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	delete(s.breakpoints, r.Pc)
	return &pb.Status{Ok: true}, nil
}

// TODO: 其余 RPC (Run, Pause, TraceStream, etc.)

func main() {
	lis, err := net.Listen("tcp", ":50051")
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}

	grpcSrv := grpc.NewServer()
	pb.RegisterVemuServiceServer(grpcSrv, newServer())

	log.Println("Vemu gRPC server listening on :50051")
	if err := grpcSrv.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
