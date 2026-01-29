// Zig ping-pong benchmark using std.Thread and Mutex
const std = @import("std");
const Thread = std.Thread;
const Mutex = std.Thread.Mutex;
const Condition = std.Thread.Condition;

const MESSAGES = 10_000_000;

const Channel = struct {
    mutex: Mutex = .{},
    cond: Condition = .{},
    ready: bool = false,
    value: i32 = 0,
};

var chan_a: Channel = .{};
var chan_b: Channel = .{};

fn pingThread(_: *anyopaque) u8 {
    var i: i32 = 0;
    while (i < MESSAGES) : (i += 1) {
        // Send to A
        chan_a.mutex.lock();
        chan_a.value = i;
        chan_a.ready = true;
        chan_a.cond.signal();
        chan_a.mutex.unlock();

        // Wait for B
        chan_b.mutex.lock();
        while (!chan_b.ready) {
            chan_b.cond.wait(&chan_b.mutex);
        }
        const received = chan_b.value;
        chan_b.ready = false;
        chan_b.mutex.unlock();

        // Validate received value matches what was sent
        if (received != i) {
            std.debug.print("Ping validation error: expected {}, got {}\n", .{i, received});
        }
    }
    return 0;
}

fn pongThread(_: *anyopaque) u8 {
    var expected: i32 = 0;
    while (expected < MESSAGES) : (expected += 1) {
        // Wait for A
        chan_a.mutex.lock();
        while (!chan_a.ready) {
            chan_a.cond.wait(&chan_a.mutex);
        }
        const received = chan_a.value;
        chan_a.ready = false;
        chan_a.mutex.unlock();

        // Validate received value matches expected sequence
        if (received != expected) {
            std.debug.print("Pong validation error: expected {}, got {}\n", .{expected, received});
        }

        // Send back the EXACT value received to B
        chan_b.mutex.lock();
        chan_b.value = received;
        chan_b.ready = true;
        chan_b.cond.signal();
        chan_b.mutex.unlock();
    }
    return 0;
}

fn rdtsc() u64 {
    // For ARM (Apple Silicon), we'll use timer
    if (@import("builtin").cpu.arch == .aarch64) {
        const ts = std.posix.clock_gettime(std.posix.CLOCK.MONOTONIC) catch return 0;
        return @as(u64, @intCast(ts.sec)) * 1_000_000_000 + @as(u64, @intCast(ts.nsec));
    }
    // For x86_64, use RDTSC
    return asm volatile ("rdtsc"
        : [ret] "={eax},{edx}" (-> u64),
    );
}

pub fn main() !void {
    var stdout_buffer: [4096]u8 = undefined;
    var stdout_writer = std.fs.File.stdout().writer(&stdout_buffer);
    const stdout = &stdout_writer.interface;

    try stdout.print("=== Zig Ping-Pong Benchmark ===\n", .{});
    try stdout.print("Messages: {}\n", .{MESSAGES});
    try stdout.print("Using std.Thread with Mutex and Condition\n\n", .{});
    try stdout.flush();

    const start = rdtsc();

    var thread1 = try Thread.spawn(.{}, pingThread, .{@as(*anyopaque, undefined)});
    var thread2 = try Thread.spawn(.{}, pongThread, .{@as(*anyopaque, undefined)});

    thread1.join();
    thread2.join();

    const end = rdtsc();
    const total_cycles = end - start;

    if (@import("builtin").cpu.arch == .aarch64) {
        // ARM: nanoseconds
        const ns_per_msg = @as(f64, @floatFromInt(total_cycles)) / @as(f64, MESSAGES);
        const throughput = 1e9 / ns_per_msg;
        const cycles_per_msg = ns_per_msg * 3.0; // Approximate at 3GHz

        try stdout.print("Cycles/msg:     {d:.2}\n", .{cycles_per_msg});
        try stdout.print("Throughput:     {d:.2} M msg/sec\n", .{throughput / 1e6});
        try stdout.flush();
    } else {
        // x86_64: actual cycles
        const cycles_per_msg = @as(f64, @floatFromInt(total_cycles)) / @as(f64, MESSAGES);
        const freq = 3.0e9; // Approximate
        const throughput = freq / cycles_per_msg;

        try stdout.print("Cycles/msg:     {d:.2}\n", .{cycles_per_msg});
        try stdout.print("Throughput:     {d:.2} M msg/sec\n", .{throughput / 1e6});
        try stdout.flush();
    }
}
