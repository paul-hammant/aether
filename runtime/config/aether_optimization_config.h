// Aether Runtime Optimization Configuration
// Defines which optimizations are always-on, auto-detected, or opt-in
//
// OPTIMIZATION TIERS:
// ====================
// Tier 1 - ALWAYS ON (built-in, proven wins):
//   - Actor Pooling (1.81x speedup)
//   - Direct Send (same-core bypass)
//   - Adaptive Batching (4-64 dynamic)
//   - Message Coalescing (15x throughput)
//   - TLS Message Pools (eliminates mutex contention)
//
// Tier 2 - AUTO-DETECT (hardware-dependent):
//   - SIMD Batch Processing (requires AVX2/NEON)
//   - MWAIT Idle (requires x86 MONITOR/MWAIT)
//   - CPU Core Pinning (OS-dependent)
//
// Tier 3 - OPT-IN (user must enable via flag, have trade-offs):
//   - Lock-free Mailbox (3.8x SLOWER single-thread, 1.8x faster under heavy contention)
//   - Message Deduplication (adds overhead, changes semantics - filters duplicates)

#ifndef AETHER_OPTIMIZATION_CONFIG_H
#define AETHER_OPTIMIZATION_CONFIG_H

#include <stdatomic.h>
#include <stdbool.h>

// ============================================================================
// TIER 1: ALWAYS-ON OPTIMIZATIONS (no user control needed)
// These provide consistent wins with no downsides
// ============================================================================

#define AETHER_ACTOR_POOLING_ENABLED    1   // Always use pooled allocation
#define AETHER_DIRECT_SEND_ENABLED      1   // Always bypass queue for same-core
#define AETHER_ADAPTIVE_BATCH_ENABLED   1   // Always adjust batch size dynamically
#define AETHER_COALESCING_ENABLED       1   // Always coalesce messages

// ============================================================================
// TIER 2: AUTO-DETECTED OPTIMIZATIONS (hardware/OS dependent)
// Automatically enabled if hardware supports them
// ============================================================================

// SIMD Processing - auto-detected at init
typedef struct {
    bool simd_available;        // CPU supports AVX2 or NEON
    bool mwait_available;       // CPU supports MONITOR/MWAIT
    bool cpu_pinning_available; // OS supports thread affinity
    int  simd_width;            // 128 (SSE), 256 (AVX2), or 512 (AVX-512)
    int  cache_line_size;       // For alignment (usually 64)
} AetherHardwareCapabilities;

// Global hardware caps (initialized once at runtime start)
extern AetherHardwareCapabilities g_hw_caps;

// Initialize hardware detection (called by scheduler_init)
void aether_detect_hardware(void);

// Quick checks for auto-detected features
static inline bool aether_has_simd(void) { return g_hw_caps.simd_available; }
static inline bool aether_has_mwait(void) { return g_hw_caps.mwait_available; }
static inline bool aether_has_cpu_pinning(void) { return g_hw_caps.cpu_pinning_available; }
static inline int  aether_simd_width(void) { return g_hw_caps.simd_width; }

// ============================================================================
// TIER 3: OPT-IN OPTIMIZATIONS (user-controlled via flags)
// May have trade-offs, user decides based on workload
// ============================================================================

// User-controllable optimization flags
typedef enum {
    AETHER_OPT_NONE             = 0,
    
    // Tier 3 opt-in flags (have trade-offs)
    AETHER_OPT_LOCKFREE_MAILBOX = (1 << 0),  // 3.8x SLOWER single-thread, 1.8x faster multi-thread contention
    AETHER_OPT_MESSAGE_DEDUP    = (1 << 1),  // Filters duplicate messages (semantic change + overhead)
    AETHER_OPT_VERBOSE          = (1 << 3),  // Print optimization info at startup
    
    // Convenience combos
    AETHER_OPT_HIGH_CONTENTION  = AETHER_OPT_LOCKFREE_MAILBOX,  // For heavy multi-threaded workloads
    AETHER_OPT_ALL              = 0xFFFFFFFF
} AetherOptFlags;

// Current runtime configuration
typedef struct {
    // Tier 3 opt-in flags (user-controlled)
    atomic_bool use_lockfree_mailbox;
    atomic_bool use_message_dedup;
    atomic_bool verbose;
    
    // Statistics
    atomic_uint_fast64_t actors_pooled;
    atomic_uint_fast64_t actors_malloced;
    atomic_uint_fast64_t direct_sends;
    atomic_uint_fast64_t queue_sends;
    atomic_uint_fast64_t batches_adjusted;
    atomic_uint_fast64_t simd_batches;
    atomic_uint_fast64_t messages_deduped;
} AetherRuntimeConfig;

extern AetherRuntimeConfig g_aether_config;

// Initialize with user flags
void aether_runtime_configure(AetherOptFlags flags);

// Print current configuration
void aether_print_config(void);

// ============================================================================
// IMPLEMENTATION (inline for performance)
// ============================================================================

static inline void aether_enable_opt(AetherOptFlags flag) {
    if (flag & AETHER_OPT_LOCKFREE_MAILBOX) atomic_store(&g_aether_config.use_lockfree_mailbox, true);
    if (flag & AETHER_OPT_MESSAGE_DEDUP) atomic_store(&g_aether_config.use_message_dedup, true);
    if (flag & AETHER_OPT_VERBOSE) atomic_store(&g_aether_config.verbose, true);
}

static inline void aether_disable_opt(AetherOptFlags flag) {
    if (flag & AETHER_OPT_LOCKFREE_MAILBOX) atomic_store(&g_aether_config.use_lockfree_mailbox, false);
    if (flag & AETHER_OPT_MESSAGE_DEDUP) atomic_store(&g_aether_config.use_message_dedup, false);
    if (flag & AETHER_OPT_VERBOSE) atomic_store(&g_aether_config.verbose, false);
}

static inline bool aether_has_opt(AetherOptFlags flag) {
    if (flag & AETHER_OPT_LOCKFREE_MAILBOX && !atomic_load(&g_aether_config.use_lockfree_mailbox)) return false;
    if (flag & AETHER_OPT_MESSAGE_DEDUP && !atomic_load(&g_aether_config.use_message_dedup)) return false;
    return true;
}

// Record statistics (enabled in debug builds)
#ifdef AETHER_STATS
#define AETHER_STAT_INC(field) atomic_fetch_add(&g_aether_config.field, 1)
#else
#define AETHER_STAT_INC(field) ((void)0)
#endif

#endif // AETHER_OPTIMIZATION_CONFIG_H
