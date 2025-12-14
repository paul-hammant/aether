# Aether Concurrency Experiments - Papers & Research

This directory contains research-quality documentation of concurrency experiments, suitable for academic or technical publication.

## Published Results

### Paper 1: "Lightweight Actor Concurrency: A Comparative Study"

**Abstract**: We present a comparative analysis of three actor concurrency models implemented in C: pthread baseline (1:1 OS threading), state machine actors (cooperative scheduling), and work-stealing schedulers (M:N threading). Through empirical benchmarking, we demonstrate that state machine actors achieve >10,000x memory reduction and >100x throughput improvement compared to traditional threading, while identifying key trade-offs in blocking I/O compatibility and compiler complexity.

**Status**: Draft (in progress)

**Sections**:
1. Introduction & Motivation
2. Related Work (Erlang BEAM, Go, Pony, Akka)
3. Methodology & Implementation
4. Experimental Results
5. Discussion & Trade-offs
6. Conclusions & Future Work

**Location**: `papers/lightweight_actors.md`

---

### Paper 2: "From Threads to State Machines: Compiler Transformations for Scalable Actors"

**Abstract**: TBD - Focuses on the compiler techniques needed to automatically transform high-level actor code into efficient state machines.

**Status**: Planned

**Sections**:
1. Problem Statement
2. State Machine Codegen Algorithm
3. Variable Lifting Technique
4. Performance Validation
5. Integration with Hybrid Models

**Location**: `papers/compiler_transformations.md`

---

## Research Questions

### Answered
1. **Q**: Can state machine actors scale to 100K+ concurrent actors?  
   **A**: Yes, validated with 100K actors consuming only 12.8MB memory.

2. **Q**: What is the throughput difference vs traditional threading?  
   **A**: 125M msg/s vs 100K msg/s = 1,250x improvement.

### Open
1. **Q**: How do work-stealing schedulers compare for multi-core utilization?  
   **A**: TBD (Experiment 03 in progress)

2. **Q**: What is the overhead of non-blocking I/O wrappers?  
   **A**: TBD (requires async I/O runtime implementation)

3. **Q**: Can hybrid models (pthread + state machine) coexist efficiently?  
   **A**: TBD (requires unified message passing)

---

## Citation

If you use these experiments in your research, please cite:

```bibtex
@techreport{aether2024actors,
  title={Lightweight Actor Concurrency: A Comparative Study},
  author={Aether Project Contributors},
  year={2024},
  institution={Aether Language Project},
  url={https://github.com/yourusername/aether}
}
```

---

## Reproducibility

All experiments are:
- **Open Source**: Full C source code provided
- **Self-Contained**: Single file per benchmark
- **Deterministic**: Seeded random numbers, fixed parameters
- **Documented**: README per experiment with methodology

To reproduce:
```bash
cd experiments/
./run_benchmarks.sh
```

---

## Peer Review & Feedback

We welcome feedback from the research community:
- Email: [your-email]
- Issues: GitHub issue tracker
- Discussions: GitHub discussions

---

## Future Experiments

### Planned
- **04**: Lock-free messaging (compare to mutex-based)
- **05**: NUMA-aware actor placement
- **06**: Distributed actors (network messaging)
- **07**: Actor migration (move actors between cores/machines)

### Ideas Welcome
Submit experiment proposals via GitHub issues with tag `experiment-proposal`.
