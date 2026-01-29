.PHONY: all clean ping_pong

all: ping_pong

ping_pong:
	zig build -Doptimize=ReleaseFast
	cp zig-out/bin/ping_pong .

clean:
	rm -rf .zig-cache zig-out ping_pong *.o
