package main

import (
	"bufio"
	"context"
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"math/bits"
	"net"
	"os"
	"os/signal"
	"runtime"
	"strings"
	"sync/atomic"
	"time"
)

const maxSessions = 32
const packetBytes = 10 // uint16 payload length + uint64 value, little endian.

type config struct {
	ip                                       string
	port, sessions, batch                    int
	reconnect                                bool
	delay, reconnectEvery, timeout, duration time.Duration
}

func readConfig(args []string, input io.Reader, output io.Writer) (config, error) {
	c := config{}
	f := flag.NewFlagSet("echo-client", flag.ContinueOnError)
	f.SetOutput(output)
	f.StringVar(&c.ip, "ip", "127.0.0.1", "server IP")
	f.IntVar(&c.port, "port", 6000, "server port")
	f.IntVar(&c.sessions, "sessions", 1, "sessions per process (1..32)")
	f.BoolVar(&c.reconnect, "reconnect", false, "periodically close and reconnect")
	f.DurationVar(&c.delay, "delay", time.Millisecond, "pause after each echoed batch; 0 = no pause")
	f.IntVar(&c.batch, "batch", 32, "packets sent together before receiving (1..4096)")
	f.DurationVar(&c.reconnectEvery, "reconnect-every", 10*time.Second, "connection lifetime when reconnect is enabled")
	f.DurationVar(&c.timeout, "timeout", 3*time.Second, "deadline for one complete batch or connection attempt")
	f.DurationVar(&c.duration, "duration", 0, "run time; 0 = until Ctrl+C")
	if err := f.Parse(args); err != nil {
		return c, err
	}
	if f.NArg() != 0 {
		return c, fmt.Errorf("unexpected arguments: %v", f.Args())
	}
	if len(args) == 0 {
		scanner := bufio.NewScanner(input)
		for _, name := range []string{"ip", "port", "sessions", "reconnect", "delay", "batch"} {
			for {
				fmt.Fprintf(output, "%s [%s]: ", name, f.Lookup(name).Value.String())
				if !scanner.Scan() {
					break
				}
				value := strings.TrimSpace(scanner.Text())
				if value == "" {
					break
				}
				if name == "reconnect" {
					switch strings.ToLower(value) {
					case "y", "yes":
						value = "true"
					case "n", "no":
						value = "false"
					}
				}
				if name == "delay" && !strings.ContainsAny(value, "abcdefghijklmnopqrstuvwxyzµ") {
					value += "ms"
				}
				if err := f.Set(name, value); err != nil {
					fmt.Fprintln(output, err)
					continue
				}
				break
			}
		}
		if err := scanner.Err(); err != nil {
			return c, err
		}
	}
	if net.ParseIP(c.ip) == nil || c.port < 1 || c.port > 65535 {
		return c, fmt.Errorf("valid IP and port 1..65535 required")
	}
	if c.sessions < 1 || c.sessions > maxSessions {
		return c, fmt.Errorf("sessions must be 1..%d; start another client for more load", maxSessions)
	}
	if c.batch < 1 || c.batch > 4096 {
		return c, fmt.Errorf("batch must be 1..4096")
	}
	if c.delay < 0 || c.timeout <= 0 || c.reconnectEvery <= 0 || c.duration < 0 {
		return c, fmt.Errorf("invalid duration")
	}
	return c, nil
}

type counters struct {
	tx, rx, batches, connects, failures    atomic.Uint64
	rtt, write, read, local, pause, maxRTT atomic.Uint64
	hist                                   [32]atomic.Uint64
	active                                 atomic.Bool
}

type snapshot struct {
	tx, rx, batches, connects, failures    uint64
	rtt, write, read, local, pause, maxRTT uint64
	hist                                   [32]uint64
	active                                 int
}

func collect(all []*counters) (v snapshot) {
	for _, s := range all {
		v.tx += s.tx.Load()
		v.rx += s.rx.Load()
		v.batches += s.batches.Load()
		v.connects += s.connects.Load()
		v.failures += s.failures.Load()
		v.rtt += s.rtt.Load()
		v.write += s.write.Load()
		v.read += s.read.Load()
		v.local += s.local.Load()
		v.pause += s.pause.Load()
		if m := s.maxRTT.Load(); m > v.maxRTT {
			v.maxRTT = m
		}
		if s.active.Load() {
			v.active++
		}
		for i := range v.hist {
			v.hist[i] += s.hist[i].Load()
		}
	}
	return
}

func latencyBucket(ns uint64) int {
	return min(bits.Len64(max(uint64(1), (ns+999)/1000)-1), 31)
}

func report(output io.Writer, now, prev snapshot, elapsed time.Duration) {
	if elapsed <= 0 {
		return
	}
	seconds := elapsed.Seconds()
	n := now.batches - prev.batches
	averageMS := func(total, before uint64) float64 {
		if n == 0 {
			return 0
		}
		return float64(total-before) / float64(n) / 1e6
	}
	var p95, seen uint64
	for i := range now.hist {
		seen += now.hist[i] - prev.hist[i]
		if n != 0 && seen >= (n*95+99)/100 {
			p95 = uint64(1) << i
			if i == 31 {
				p95 = max(p95, (now.maxRTT+999)/1000)
			}
			break
		}
	}
	fmt.Fprintf(output, "active=%d connections=%d errors=%d tx=%.0f pkt/s rx=%.0f pkt/s rx=%.2f MiB/s goroutines=%d\n",
		now.active, now.connects, now.failures, float64(now.tx-prev.tx)/seconds, float64(now.rx-prev.rx)/seconds,
		float64(now.rx-prev.rx)*packetBytes/seconds/(1<<20), runtime.NumGoroutine())
	fmt.Fprintf(output, "  batch RTT avg=%.3fms p95<=%.3fms max(all)=%.3fms | write=%.3fms echo-wait=%.3fms local=%.3fms pause=%.3fms\n",
		averageMS(now.rtt, prev.rtt), float64(p95)/1000, float64(now.maxRTT)/1e6,
		averageMS(now.write, prev.write), averageMS(now.read, prev.read), averageMS(now.local, prev.local), averageMS(now.pause, prev.pause))
}

// Reused timer: no per-packet goroutine, timer allocation, or logging.
func pause(ctx context.Context, timer *time.Timer, delay time.Duration) bool {
	if delay <= 0 {
		return ctx.Err() == nil
	}
	timer.Reset(delay)
	select {
	case <-ctx.Done():
		timer.Stop()
		return false
	case <-timer.C:
		return true
	}
}

func writeAll(conn net.Conn, data []byte) error {
	for len(data) != 0 {
		n, err := conn.Write(data)
		data = data[n:]
		if err != nil {
			return err
		}
		if n == 0 {
			return io.ErrNoProgress
		}
	}
	return nil
}

// One bounded worker per session. One batch is in flight; all its packets are sent
// before reading. Never creates a goroutine for individual packets or send/receive.
func exchange(ctx context.Context, conn net.Conn, c config, s *counters, send, recv []byte) error {
	stop := context.AfterFunc(ctx, func() { conn.Close() })
	defer stop()
	timer := time.NewTimer(time.Hour)
	timer.Stop()
	defer timer.Stop()
	connected := time.Now()
	var sequence uint64
	for ctx.Err() == nil {
		preparing := time.Now()
		for i := 0; i < c.batch; i++ {
			frame := send[i*packetBytes:]
			binary.LittleEndian.PutUint16(frame, 8)
			binary.LittleEndian.PutUint64(frame[2:], sequence+uint64(i))
		}
		start := time.Now()
		if err := conn.SetDeadline(start.Add(c.timeout)); err != nil {
			return err
		}
		if err := writeAll(conn, send); err != nil {
			return fmt.Errorf("write: %w", err)
		}
		written := time.Now()
		s.tx.Add(uint64(c.batch)) // Complete batch accepted by the local socket, not delivery.
		if _, err := io.ReadFull(conn, recv); err != nil {
			return fmt.Errorf("echo read: %w", err)
		}
		received := time.Now()
		for i := 0; i < c.batch; i++ {
			frame := recv[i*packetBytes:]
			if binary.LittleEndian.Uint16(frame) != 8 || binary.LittleEndian.Uint64(frame[2:]) != sequence+uint64(i) {
				return fmt.Errorf("echo mismatch at sequence %d", sequence+uint64(i))
			}
		}
		s.rx.Add(uint64(c.batch))
		s.rtt.Add(uint64(received.Sub(start)))
		s.write.Add(uint64(written.Sub(start)))
		s.read.Add(uint64(received.Sub(written)))
		s.local.Add(uint64(start.Sub(preparing) + time.Since(received)))
		rtt := uint64(received.Sub(start))
		s.hist[latencyBucket(rtt)].Add(1)
		if rtt > s.maxRTT.Load() {
			s.maxRTT.Store(rtt)
		} // Single writer.
		s.batches.Add(1)
		sequence += uint64(c.batch)
		if c.reconnect && time.Since(connected) >= c.reconnectEvery {
			return nil
		}
		waiting := time.Now()
		if !pause(ctx, timer, c.delay) {
			return ctx.Err()
		}
		s.pause.Add(uint64(time.Since(waiting)))
	}
	return ctx.Err()
}

func runSession(ctx context.Context, c config, s *counters, errors chan<- string, done chan<- struct{}) {
	defer func() { done <- struct{}{} }()
	send, recv := make([]byte, c.batch*packetBytes), make([]byte, c.batch*packetBytes)
	dialer := net.Dialer{Timeout: c.timeout}
	timer := time.NewTimer(time.Hour)
	timer.Stop()
	defer timer.Stop()
	for ctx.Err() == nil {
		conn, err := dialer.DialContext(ctx, "tcp", net.JoinHostPort(c.ip, fmt.Sprint(c.port)))
		if err == nil {
			s.connects.Add(1)
			s.active.Store(true)
			err = exchange(ctx, conn, c, s, send, recv)
			conn.Close()
			s.active.Store(false)
		}
		if ctx.Err() != nil {
			return
		}
		if err != nil {
			s.failures.Add(1)
			select {
			case errors <- err.Error():
			default:
			}
		}
		if !c.reconnect {
			return
		}
		if !pause(ctx, timer, time.Second) {
			return
		} // Fixed backoff prevents a reconnect storm.
	}
}

func run(c config) int {
	ctx, cancel := signal.NotifyContext(context.Background(), os.Interrupt)
	defer cancel()
	if c.duration > 0 {
		var stop context.CancelFunc
		ctx, stop = context.WithTimeout(ctx, c.duration)
		defer stop()
	}
	fmt.Printf("target=%s sessions=%d/%d batch=%d delay=%s reconnect=%t timeout=%s GOMAXPROCS=%d\n",
		net.JoinHostPort(c.ip, fmt.Sprint(c.port)), c.sessions, maxSessions, c.batch, c.delay, c.reconnect, c.timeout, runtime.GOMAXPROCS(0))
	fmt.Println("RTT measures a complete batch, including client/OS/network time. Ctrl+C stops.")
	all := make([]*counters, c.sessions)
	done, errors := make(chan struct{}, c.sessions), make(chan string, c.sessions)
	for i := range all {
		all[i] = &counters{}
		go runSession(ctx, c, all[i], errors, done)
	}
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()
	previous, last := snapshot{}, time.Now()
	for remaining := c.sessions; remaining > 0; {
		select {
		case <-done:
			remaining--
		case message := <-errors:
			fmt.Fprintln(os.Stderr, "session:", message)
		case <-ticker.C:
			current, at := collect(all), time.Now()
			report(os.Stdout, current, previous, at.Sub(last))
			previous, last = current, at
		}
	}
	final := collect(all)
	report(os.Stdout, final, previous, time.Since(last))
	fmt.Printf("total sent=%d validated=%d batches=%d connections=%d errors=%d\n", final.tx, final.rx, final.batches, final.connects, final.failures)
	if final.failures > 0 {
		return 1
	}
	return 0
}

func main() {
	c, err := readConfig(os.Args[1:], os.Stdin, os.Stdout)
	if err == flag.ErrHelp {
		return
	}
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(2)
	}
	os.Exit(run(c))
}
