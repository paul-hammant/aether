---
name: Aether Reddit Polish v3
overview: Fix verified real bugs (NUMA free mismatch, stale comments, JSON/HTTP buffers), add println and string interpolation for ergonomics, clean up README accuracy, and update examples.
todos:
  - id: fix-numa-free
    content: Fix aether_numa_free mismatched allocator on Linux NUMA path
    status: pending
  - id: fix-stale-comments
    content: Fix stale 'NO work stealing' comments in multicore_scheduler.c
    status: pending
  - id: fix-json-buffer
    content: Replace fixed 4KB string buffer in JSON parser with dynamic allocation
    status: pending
  - id: fix-http-response-buffer
    content: Replace static 64KB response buffer in HTTP server with dynamic allocation
    status: pending
  - id: fix-http-request-buffer
    content: Replace 8KB request read buffer with proper Content-Length-aware reading
    status: pending
  - id: add-println
    content: Add println() builtin to codegen
    status: pending
  - id: add-string-interp
    content: Add string interpolation ${expr} to lexer, parser, AST, and codegen
    status: pending
  - id: readme-accuracy
    content: Fix README work-stealing description and add Limitations section
    status: pending
  - id: update-examples
    content: Update key examples to use println and string interpolation
    status: pending
isProject: false
---

# Address Reddit Review Concerns (Verified v3)

## What I verified is NOT broken (corrections from earlier):

- `**queue_enqueue_batch` space formula** -- `(head - tail - 1) & QUEUE_MASK` is correct. Standard ring buffer math; works on all two's complement platforms (i.e., all real hardware).
- **SIMD is not dead code** -- It's a runtime feature flag (`AETHER_FLAG_ENABLE_SIMD` / auto-detected). Two layers: `aether_simd_batch.h` provides real AVX2+NEON primitives (`simd_batch_process_int`, `simd_batch_compare_ids`) used via the optimization layer in `scheduler_optimizations.h`; `aether_simd_vectorized.c` provides standalone utility functions used in examples. The flag gates `optimized_process_batch_simd` which enables batch processing.
- **REPL** -- The README already says "compile-and-run loop" right next to `ae repl`. The implementation IS interactive (prompt, line accumulation, brace tracking). It's honest.
- **Package manager** -- `apkg` exists as a standalone tool and `ae add` works for GitHub packages. README "Package registry" is in the Roadmap section (not claiming it exists). This is accurate.

---

## Phase 1: Real Bugs

### 1.1 Fix `aether_numa_free` mismatched allocator (Linux NUMA path)

[runtime/aether_numa.c](runtime/aether_numa.c) lines 161-179: When `HAVE_LIBNUMA` is defined and `g_topology.available` is true, `aether_numa_alloc` falls back to `malloc()` if `node < 0` or `node >= num_nodes`. But `aether_numa_free` calls `numa_free()` unconditionally when `g_topology.available` is true -- this feeds a `malloc`'d pointer to `numa_free`, which is undefined behavior.

Fix: make `aether_numa_alloc` use `numa_alloc_local()` instead of `malloc()` as the fallback when NUMA is available, so all pointers go through the NUMA allocator consistently. This way `aether_numa_free` calling `numa_free()` is always correct.

### 1.2 Fix stale "NO work stealing" comments

[runtime/scheduler/multicore_scheduler.c](runtime/scheduler/multicore_scheduler.c) lines 87, 269-271 have comments saying "NO work stealing" directly above code that implements work stealing (lines 277-313). Fix the comments to match reality: partitioned scheduler with work-stealing fallback for idle cores.

### 1.3 Fix JSON parser 4KB string buffer

[std/json/aether_json.c](std/json/aether_json.c) line 57: `char buffer[4096]` silently truncates JSON strings over 4095 chars. Replace with dynamic allocation that grows as needed.

### 1.4 Fix HTTP server static 64KB response buffer

[std/net/aether_http_server.c](std/net/aether_http_server.c) line 340: `static char buffer[65536]` is both size-limited AND not thread-safe (shared across calls via `static`). Replace with a dynamically-allocated buffer sized to the actual response, returned via caller-provided buffer or malloc.

### 1.5 Fix HTTP server 8KB request read buffer

[std/net/aether_http_server.c](std/net/aether_http_server.c) line 489: `char buffer[8192]` for request reading truncates large POST bodies. Replace with a read loop that reads until Content-Length is satisfied or connection closes.

## Phase 2: Language Ergonomics

### 2.1 Add `println()` builtin

Every example currently needs `print("...\n")`. Add `println()` that auto-appends a newline.

File: [compiler/codegen/codegen_expr.c](compiler/codegen/codegen_expr.c) -- add a `println` case mirroring the existing `print` logic but appending `putchar('\n')` after the print.

### 2.2 Add string interpolation `"Hello ${name}"`

Biggest ergonomic win. Transforms:

```
print("Count: "); print(count); print("\n");
```

into:

```
println("Count: ${count}")
```

Files to change:

- [compiler/parser/tokens.h](compiler/parser/tokens.h) -- decide token strategy (single token with segments, or desugar at lex time)
- [compiler/parser/lexer.c](compiler/parser/lexer.c) -- detect `${` inside strings, split into segments
- [compiler/ast.h](compiler/ast.h) -- add `AST_STRING_INTERP` node type
- [compiler/parser/parser.c](compiler/parser/parser.c) -- parse interpolated string into AST
- [compiler/codegen/codegen_expr.c](compiler/codegen/codegen_expr.c) -- generate `printf` with format specifiers inferred from expression types

## Phase 3: README Accuracy

Rewrite [README.md](README.md) to be precise:

- Line 25: "Work-stealing for dynamic load balancing" -> "Work-stealing fallback for idle core balancing" (primary strategy is partitioned)
- Line 156: "Multi-core work-stealing scheduler" -> "Multi-core partitioned scheduler with work-stealing fallback"
- Line 322: Same adjustment
- Line 166: `apkg/` -- clarify role vs `ae` CLI (both can add dependencies; `apkg` is standalone, `ae add` is the primary interface)
- Add brief "Limitations" or "Known Constraints" to the Status section: no string interpolation (until Phase 2), single-threaded HTTP server, basic JSON parser, no generics

## Phase 4: Update Examples

After Phase 2 lands, update key examples to use `println()` and string interpolation:

- [examples/basics/hello.ae](examples/basics/hello.ae)
- [examples/actors/ping-pong.ae](examples/actors/ping-pong.ae)
- [examples/actors/counter.ae](examples/actors/counter.ae)
- [examples/basics/pattern-matching.ae](examples/basics/pattern-matching.ae)
- The inline code example in README.md

