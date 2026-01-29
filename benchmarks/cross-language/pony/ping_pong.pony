// Pony Ping-Pong Benchmark
// Actor-based message passing with Pony's causal messaging

use "time"

actor Pong
  let _main: Main
  let _env: Env
  var _expected: U64 = 0

  new create(main: Main, env: Env) =>
    _main = main
    _env = env

  be pong(value: U64) =>
    // Validate received value matches expected sequence
    if value != _expected then
      _env.err.print("Pong validation error: expected " + _expected.string() + ", got " + value.string())
    end

    _expected = _expected + 1
    if _expected < 10_000_000 then
      // Echo back the EXACT value received
      _main.ping(value)
    else
      _main.done(_expected)
    end

actor Main
  let _env: Env
  let _pong: Pong
  var _start: U64 = 0
  var _i: U64 = 0
  let _timers: Timers

  new create(env: Env) =>
    _env = env
    _timers = Timers
    _pong = Pong(this, env)
    _env.out.print("=== Pony Ping-Pong Benchmark ===")
    _env.out.print("Messages: 10000000\n")
    _start = Time.nanos()
    ping_start()

  be ping_start() =>
    _pong.pong(_i)

  be ping(value: U64) =>
    // Validate received value matches what was sent
    if value != _i then
      _env.err.print("Ping validation error: expected " + _i.string() + ", got " + value.string())
    end

    _i = _i + 1
    _pong.pong(_i)

  be done(count: U64) =>
    let finish = Time.nanos()
    let elapsed_ns = finish - _start
    let ns_per_msg = elapsed_ns.f64() / 10_000_000.0
    let throughput = 1_000_000_000.0 / ns_per_msg
    let cycles_per_msg = ns_per_msg * 3.0  // Approximate at 3GHz

    _env.out.print("Cycles/msg:     " + cycles_per_msg.string())
    _env.out.print("Throughput:     " + (throughput / 1_000_000.0).string() + " M msg/sec")
