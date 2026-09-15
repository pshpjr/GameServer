package main

import (
	"bytes"
	"context"
	"encoding/binary"
	"io"
	"net"
	"strings"
	"testing"
	"time"
)

func TestDefaultsAndLimits(t *testing.T) {
	c, err := readConfig(nil, strings.NewReader("\n\n\n\n\n\n"), io.Discard)
	if err != nil || c.ip != "127.0.0.1" || c.port != 6000 || c.sessions != 1 || c.batch != 32 || c.delay != time.Millisecond || c.reconnect {
		t.Fatalf("defaults: %+v, %v", c, err)
	}
	c, err = readConfig(nil, strings.NewReader("::1\n6001\n32\ny\n2\n4096\n"), io.Discard)
	if err != nil || !c.reconnect || c.delay != 2*time.Millisecond || c.sessions != 32 || c.batch != 4096 {
		t.Fatalf("interactive: %+v, %v", c, err)
	}
	for _, args := range [][]string{
		{"-sessions", "33"}, {"-sessions", "0"}, {"-batch", "0"}, {"-batch", "4097"},
		{"-port", "65536"}, {"-ip", "invalid"}, {"-delay", "-1ms"}, {"-timeout", "0"},
	} {
		if _, err := readConfig(args, strings.NewReader(""), io.Discard); err == nil {
			t.Fatalf("accepted %v", args)
		}
	}
}

func testConfig() config {
	return config{ip: "127.0.0.1", sessions: 1, batch: 4, timeout: time.Second, reconnectEvery: 30 * time.Millisecond}
}

func TestFragmentedEchoAndSequence(t *testing.T) {
	client, server := net.Pipe()
	defer client.Close()
	defer server.Close()
	c := testConfig()
	c.reconnect = true
	serverDone := make(chan error, 1)
	go func() {
		buf := make([]byte, c.batch*packetBytes)
		var sequence uint64
		for {
			if _, err := io.ReadFull(server, buf); err != nil {
				serverDone <- nil
				return
			}
			for i := 0; i < c.batch; i++ {
				if binary.LittleEndian.Uint16(buf[i*10:]) != 8 || binary.LittleEndian.Uint64(buf[i*10+2:]) != sequence {
					serverDone <- io.ErrUnexpectedEOF
					return
				}
				sequence++
			}
			// Deliberately split both the header and body across reads.
			for _, b := range buf {
				if _, err := server.Write([]byte{b}); err != nil {
					serverDone <- nil
					return
				}
			}
		}
	}()
	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	s := &counters{}
	err := exchange(ctx, client, c, s, make([]byte, c.batch*10), make([]byte, c.batch*10))
	client.Close()
	if err != nil {
		t.Fatal(err)
	}
	if err := <-serverDone; err != nil {
		t.Fatal("wire sequence:", err)
	}
	if s.batches.Load() < 2 || s.tx.Load() != s.rx.Load() || s.rtt.Load() == 0 {
		t.Fatalf("invalid counters: %+v", collect([]*counters{s}))
	}
}

func TestCorruptionTruncationAndStall(t *testing.T) {
	for _, mode := range []string{"body", "header", "truncate", "stall"} {
		t.Run(mode, func(t *testing.T) {
			client, server := net.Pipe()
			defer client.Close()
			defer server.Close()
			c := testConfig()
			c.timeout = 40 * time.Millisecond
			done := make(chan struct{})
			go func() {
				defer close(done)
				buf := make([]byte, c.batch*10)
				if _, err := io.ReadFull(server, buf); err != nil {
					return
				}
				switch mode {
				case "body":
					buf[2] ^= 1
				case "header":
					buf[0] = 0
				case "truncate":
					server.Write(buf[:7])
					server.Close()
					return
				case "stall":
					<-time.After(70 * time.Millisecond)
					return
				}
				server.Write(buf)
			}()
			s := &counters{}
			err := exchange(context.Background(), client, c, s, make([]byte, c.batch*10), make([]byte, c.batch*10))
			client.Close()
			<-done
			if err == nil || s.rx.Load() != 0 {
				t.Fatalf("undetected %s: %v", mode, err)
			}
		})
	}
}

func TestReconnectAndCancellation(t *testing.T) {
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	defer listener.Close()
	serverDone := make(chan struct{})
	go func() {
		defer close(serverDone)
		for {
			conn, err := listener.Accept()
			if err != nil {
				return
			}
			io.Copy(conn, conn)
			conn.Close()
		}
	}()
	c := testConfig()
	c.port = listener.Addr().(*net.TCPAddr).Port
	c.reconnect = true
	ctx, cancel := context.WithTimeout(context.Background(), 1400*time.Millisecond)
	defer cancel()
	s := &counters{}
	done, errors := make(chan struct{}, 1), make(chan string, 8)
	go runSession(ctx, c, s, errors, done)
	select {
	case <-done:
	case <-time.After(3 * time.Second):
		t.Fatal("worker did not stop")
	}
	listener.Close()
	<-serverDone
	if s.connects.Load() < 2 || s.failures.Load() != 0 || s.active.Load() {
		t.Fatalf("reconnect counters: %+v", collect([]*counters{s}))
	}
}

func TestWriteAllHandlesShortWrite(t *testing.T) {
	// net.Pipe fragments reads, but implements full writes; wrap to force short writes.
	a, b := net.Pipe()
	defer a.Close()
	defer b.Close()
	want := []byte("0123456789")
	result := make(chan []byte, 1)
	go func() { got := make([]byte, len(want)); io.ReadFull(b, got); result <- got }()
	if err := writeAll(shortConn{a}, want); err != nil {
		t.Fatal(err)
	}
	if got := <-result; !bytes.Equal(got, want) {
		t.Fatalf("short write: %q", got)
	}
}

type shortConn struct{ net.Conn }

func (c shortConn) Write(p []byte) (int, error) { return c.Conn.Write(p[:min(3, len(p))]) }
