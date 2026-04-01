# Next Steps

Planned features and improvements for upcoming Aether releases.

> See [CHANGELOG.md](../CHANGELOG.md) for what shipped in each release.

## Language Features

### Closures and First-Class Functions

Arrow functions exist but are named functions, not values. There are no anonymous functions and no way to capture variables from an enclosing scope. This blocks higher-order patterns like `list.map()`, `list.filter()`, and callbacks.

**Planned syntax (tentative):**

```aether
import std.list

main() {
    numbers = [1, 2, 3, 4, 5]

    // Anonymous function as argument
    doubled = list.map(numbers, fn(x) { x * 2 })

    // Closure capturing a local variable
    threshold = 3
    big = list.filter(numbers, fn(x) { x > threshold })
}
```

**What's needed:**
- Anonymous function expression syntax (e.g., `fn(x) { x * 2 }`)
- `AST_CLOSURE` node with a capture list in the compiler
- Codegen emits a struct holding captured variables + a function pointer (standard closure conversion to C)
- Function types in type inference (e.g., `(int) -> int` as a first-class type)

**Design constraint:** Capture by value (copy into closure struct) is the default. No hidden heap allocation. This keeps closures predictable and compatible with manual memory management.

### Stdlib Migration to Result Types

Result types (`a, err = func()`) are implemented in the language. The stdlib I/O functions (`file.open`, `io.read_file`, `file.write`, etc.) still use the old `int` return convention. Migrating them to `(value, error)` returns is a breaking change planned for a future release.

## Quick Wins

### Package Registry — Transitive Dependencies

`ae add` supports versioned packages (`ae add github.com/user/repo@v1.0.0`) and the module resolver finds installed packages. Next: transitive dependency resolution, lock file integrity, and `ae update`.

### `or` Keyword for Error Defaults

Sugar for defaulting on error: `content = io.read_file("config.txt") or "default"`. Requires result types to be fully adopted in stdlib first.

## Future

Major features that require significant architectural work.

### WebAssembly Target — Phase 2

Phase 1 is complete: `ae build --target wasm` compiles Aether to WebAssembly via Emscripten. Multi-actor programs work cooperatively.

**What's remaining (Phase 2):**
- Multi-actor programs using Web Workers as scheduler threads with `postMessage`
- Emscripten-specific output (HTML template for browser)
- WASI support for non-browser environments

### Async I/O Integration

All I/O in Aether is currently blocking. `http.get()`, `file.read()`, `tcp_connect()`, and `sleep()` all block the OS thread. Since the scheduler places actors on the spawner's core by default (locality-aware placement), actors spawned from `main()` all land on core 0 — one OS thread. A blocking I/O call in one actor prevents ALL actors on that core from running.

**User impact:** An actor doing 5 HTTP requests will block all sibling actors for the entire duration. There is no way for the scheduler to preempt a handler that's blocked in a system call.

**What's needed:**
- I/O event loop thread(s) using platform-native async APIs (io_uring on Linux, kqueue on macOS, IOCP on Windows)
- I/O completions delivered as actor messages (send request → receive response as message)
- Scheduler awareness of I/O-blocked actors (don't count them as idle)
- Async variants of file and network operations in the stdlib
- Non-blocking `sleep` that yields to the scheduler instead of blocking the thread

### Version Management UX

`ae version list` should clearly show which versions are installed locally, which are available remotely, and which is active. Current display only marks the active version.

**What's needed:**
- `ae version list` columns: version, status (active/installed/available)
- `ae version use` should preserve the initial install in `versions/` before switching (currently loses it on Windows; POSIX needs same fix)
- `ae --version` reads `active_version` file instead of compiled-in constant (done)
- macOS quarantine removal on `install.sh` and `ae version use` (done)

## Tooling

### Planned

| Feature | Status | Notes |
|---------|--------|-------|
| `ae fmt` | Not started | Source code formatter (deferred until syntax stabilizes) |
