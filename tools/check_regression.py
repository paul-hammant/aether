#!/usr/bin/env python3
"""
Performance regression detector for Aether benchmarks.
Compares current results against baseline and fails if regression > threshold.
"""

import json
import sys
from pathlib import Path
from typing import Dict, Any, Tuple

# Configuration
REGRESSION_THRESHOLD = 0.10  # 10% regression triggers failure
WARNING_THRESHOLD = 0.05     # 5% regression triggers warning

# ANSI colors
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
CYAN = '\033[0;36m'
NC = '\033[0m'  # No Color


def load_json(filepath: Path) -> Dict[str, Any]:
    """Load JSON file."""
    try:
        with open(filepath, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        print(f"Error: {filepath} not found")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"Error parsing {filepath}: {e}")
        sys.exit(1)


def compare_metric(baseline: float, current: float) -> Tuple[float, str]:
    """
    Compare metric and return (percentage_change, status).
    Positive change = improvement, negative = regression.
    """
    if baseline == 0:
        return 0.0, "N/A"
    
    change = ((current - baseline) / baseline) * 100
    
    if change >= 0:
        # Improvement
        status = "improved"
    elif abs(change / 100) >= REGRESSION_THRESHOLD:
        # Significant regression
        status = "regressed"
    elif abs(change / 100) >= WARNING_THRESHOLD:
        # Warning level regression
        status = "warning"
    else:
        # Minor change
        status = "stable"
    
    return change, status


def format_change(change: float, status: str) -> str:
    """Format percentage change with color."""
    sign = "+" if change >= 0 else ""
    change_str = f"{sign}{change:.2f}%"
    
    if status == "improved":
        return f"{GREEN}{change_str}{NC}"
    elif status == "regressed":
        return f"{RED}{change_str}{NC}"
    elif status == "warning":
        return f"{YELLOW}{change_str}{NC}"
    else:
        return f"{change_str}"


def main():
    # Get paths
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    baseline_file = project_root / "benchmarks" / "baseline.json"
    results_file = project_root / "benchmarks" / "results.json"
    
    # Load data
    baseline = load_json(baseline_file)
    results = load_json(results_file)
    
    baseline_benchmarks = baseline.get("benchmarks", {})
    current_benchmarks = results.get("benchmarks", {})
    
    print(f"\n{CYAN}=== Performance Regression Check ==={NC}\n")
    print(f"Baseline: {baseline.get('timestamp', 'unknown')}")
    print(f"Current:  {results.get('timestamp', 'unknown')}")
    print(f"Threshold: {REGRESSION_THRESHOLD*100}% regression\n")
    
    # Track overall status
    has_regression = False
    has_warning = False
    
    # Compare each benchmark
    for bench_name in sorted(current_benchmarks.keys()):
        if bench_name not in baseline_benchmarks:
            print(f"{bench_name}: {YELLOW}NEW (no baseline){NC}")
            continue
        
        baseline_metrics = baseline_benchmarks[bench_name]
        current_metrics = current_benchmarks[bench_name]
        
        print(f"{bench_name}:")
        
        # Compare all numeric metrics
        for metric_name in sorted(current_metrics.keys()):
            if metric_name == "description":
                continue
            
            if metric_name not in baseline_metrics:
                print(f"  {metric_name}: {YELLOW}NEW{NC}")
                continue
            
            baseline_val = baseline_metrics[metric_name]
            current_val = current_metrics[metric_name]
            
            if not isinstance(baseline_val, (int, float)) or not isinstance(current_val, (int, float)):
                continue
            
            change, status = compare_metric(baseline_val, current_val)
            change_str = format_change(change, status)
            
            # Format values with appropriate suffixes
            if baseline_val >= 1_000_000:
                baseline_fmt = f"{baseline_val/1_000_000:.1f}M"
                current_fmt = f"{current_val/1_000_000:.1f}M"
            elif baseline_val >= 1_000:
                baseline_fmt = f"{baseline_val/1_000:.1f}K"
                current_fmt = f"{current_val/1_000:.1f}K"
            else:
                baseline_fmt = f"{baseline_val:.0f}"
                current_fmt = f"{current_val:.0f}"
            
            print(f"  {metric_name}: {baseline_fmt} -> {current_fmt} ({change_str})")
            
            if status == "regressed":
                has_regression = True
            elif status == "warning":
                has_warning = True
        
        print()
    
    # Summary
    print(f"{CYAN}=== Summary ==={NC}\n")
    
    if has_regression:
        print(f"{RED}FAIL: Performance regression detected (>{REGRESSION_THRESHOLD*100}%){NC}")
        print(f"\nRegression threshold exceeded. Please investigate:")
        print(f"  1. Profile the code to identify bottlenecks")
        print(f"  2. Check recent commits for performance impact")
        print(f"  3. Verify test environment (CPU load, thermal throttling)")
        print(f"  4. If intentional, update baseline: ./tools/benchmark_runner.sh --save-baseline")
        return 1
    elif has_warning:
        print(f"{YELLOW}WARNING: Minor performance degradation detected (>{WARNING_THRESHOLD*100}%){NC}")
        print(f"Not failing build, but monitor for trends.")
        return 0
    else:
        print(f"{GREEN}PASS: No performance regressions detected{NC}")
        return 0


if __name__ == "__main__":
    sys.exit(main())

