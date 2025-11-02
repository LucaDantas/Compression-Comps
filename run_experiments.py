#!/usr/bin/env python3
"""
Orchestrator script to run simple_pipeline with SP and HAAR transforms
on all datasets with chunk size 512, in parallel.
"""

import os
import subprocess
import csv
from pathlib import Path
from concurrent.futures import ProcessPoolExecutor, as_completed
from typing import List, Tuple, Dict
import re
import statistics

# Configuration
CHUNK_SIZE = 512
TRANSFORMS = ["SP", "HAAR"]
DATASETS_DIR = "Datasets"
EXECUTABLE = "./simple_pipeline"
RESULTS_DIR = "results"

def find_all_images() -> List[Tuple[str, str]]:
    """
    Find all PNG images in all dataset subdirectories.
    Returns list of (dataset_name, image_path) tuples.
    """
    images = []
    datasets_path = Path(DATASETS_DIR)
    
    if not datasets_path.exists():
        print(f"Error: {DATASETS_DIR} directory not found")
        return images
    
    for dataset_dir in datasets_path.iterdir():
        if dataset_dir.is_dir():
            dataset_name = dataset_dir.name
            for image_file in dataset_dir.glob("*.png"):
                image_path = str(image_file)
                images.append((dataset_name, image_path))
    
    return sorted(images)

def parse_output(output: str) -> Tuple[float, float, float, float, float, float, float, float, float, float, float]:
    """
    Parse the output from simple_pipeline.
    Expected content includes a metrics tuple and optionally a Times line.
    Returns: (mse, psnr, original_entropy, transformed_entropy, quantized_entropy,
              encode_ms, quant_ms, entropy_enc_ms, entropy_dec_ms, dequant_ms, inverse_ms)
    Missing timing values are returned as None.
    """
    # Normalize lines
    output = output.strip()
    lines = [ln.strip() for ln in output.splitlines() if ln.strip()]

    # First, find the tuple line (something like: (mse, psnr, orig_ent, trans_ent, quant_ent))
    tuple_line = None
    times_line = None
    for ln in lines:
        if ln.startswith("(") and ln.endswith(")"):
            tuple_line = ln
        if ln.lower().startswith("times (ms):") or ln.startswith("Times (ms):"):
            times_line = ln

    if tuple_line is None:
        # Maybe the output is just the tuple on a single line without newlines
        if output.startswith("(") and output.endswith(")"):
            tuple_line = output
        else:
            raise ValueError(f"Could not find metrics tuple in output: {output}")

    # parse tuple
    t = tuple_line
    if t.startswith("(") and t.endswith(")"):
        t = t[1:-1]
    parts = [x.strip() for x in t.split(",")]
    if len(parts) != 5:
        raise ValueError(f"Expected 5 metric values, got {len(parts)}: {tuple_line}")
    try:
        mse = float(parts[0])
        psnr = float(parts[1])
        original_entropy = float(parts[2])
        transformed_entropy = float(parts[3])
        quantized_entropy = float(parts[4])
    except ValueError as e:
        raise ValueError(f"Could not parse metric values: {tuple_line}") from e

    # default timings
    encode_ms = quant_ms = entropy_enc_ms = entropy_dec_ms = dequant_ms = inverse_ms = None

    # parse times line if present
    if times_line:
        # Example: Times (ms): encode=12.34 quant=3.21 entropy_enc=0 entropy_dec=0 dequant=2.11 inverse=10.50
        # extract key=value pairs
        kvs = re.findall(r"([a-z_]+)=([0-9.+-eE]+)", times_line.replace('(', ' ').replace(')', ' '))
        kv = {k: float(v) for k, v in kvs}
        encode_ms = kv.get('encode')
        quant_ms = kv.get('quant')
        entropy_enc_ms = kv.get('entropy_enc')
        entropy_dec_ms = kv.get('entropy_dec')
        dequant_ms = kv.get('dequant')
        inverse_ms = kv.get('inverse')

    return (mse, psnr, original_entropy, transformed_entropy, quantized_entropy,
            encode_ms, quant_ms, entropy_enc_ms, entropy_dec_ms, dequant_ms, inverse_ms)

def run_pipeline(transform: str, chunk_size: int, image_path: str) -> Dict:
    """
    Run simple_pipeline for a single image and transform.
    Returns a dict with the results.
    """
    try:
        result = subprocess.run(
            [EXECUTABLE, transform, str(chunk_size), image_path],
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout per image
        )
        
        if result.returncode != 0:
            error_msg = result.stderr.strip()
            print(f"Error running {transform} on {image_path}: {error_msg}")
            return {
                "dataset": Path(image_path).parent.name,
                "image_path": image_path,
                "transform": transform,
                "mse": None,
                "psnr": None,
                "original_entropy": None,
                "transformed_entropy": None,
                "quantized_entropy": None,
                "error": error_msg
            }
        
        output = result.stdout.strip()
        (mse, psnr, orig_ent, trans_ent, quant_ent,
         encode_ms, quant_ms, entropy_enc_ms, entropy_dec_ms, dequant_ms, inverse_ms) = parse_output(output)

        return {
            "dataset": Path(image_path).parent.name,
            "image_path": image_path,
            "transform": transform,
            "mse": mse,
            "psnr": psnr,
            "original_entropy": orig_ent,
            "transformed_entropy": trans_ent,
            "quantized_entropy": quant_ent,
            "encode_ms": encode_ms,
            "quant_ms": quant_ms,
            "entropy_enc_ms": entropy_enc_ms,
            "entropy_dec_ms": entropy_dec_ms,
            "dequant_ms": dequant_ms,
            "inverse_ms": inverse_ms,
            "error": None
        }
    except subprocess.TimeoutExpired:
        print(f"Timeout running {transform} on {image_path}")
        return {
            "dataset": Path(image_path).parent.name,
            "image_path": image_path,
            "transform": transform,
            "mse": None,
            "psnr": None,
            "original_entropy": None,
            "transformed_entropy": None,
            "quantized_entropy": None,
            "error": "Timeout"
        }
    except Exception as e:
        print(f"Exception running {transform} on {image_path}: {e}")
        return {
            "dataset": Path(image_path).parent.name,
            "image_path": image_path,
            "transform": transform,
            "mse": None,
            "psnr": None,
            "original_entropy": None,
            "transformed_entropy": None,
            "quantized_entropy": None,
            "error": str(e)
        }

def main():
    # Check if executable exists
    if not os.path.exists(EXECUTABLE):
        print(f"Error: Executable {EXECUTABLE} not found. Please compile simple_pipeline.cpp first.")
        return 1
    
    # Create results directory
    os.makedirs(RESULTS_DIR, exist_ok=True)
    
    # Find all images
    images = find_all_images()
    print(f"Found {len(images)} images across all datasets")
    
    if len(images) == 0:
        print("No images found. Exiting.")
        return 1
    
    # Prepare all tasks
    tasks = []
    for transform in TRANSFORMS:
        for dataset_name, image_path in images:
            tasks.append((transform, dataset_name, image_path))
    
    print(f"Total tasks: {len(tasks)} (2 transforms × {len(images)} images)")
    
    # Run tasks in parallel
    results_by_transform = {transform: [] for transform in TRANSFORMS}
    completed = 0
    
    with ProcessPoolExecutor() as executor:
        # Submit all tasks
        future_to_task = {
            executor.submit(run_pipeline, transform, CHUNK_SIZE, image_path): (transform, dataset_name, image_path)
            for transform, dataset_name, image_path in tasks
        }
        
        # Collect results as they complete
        for future in as_completed(future_to_task):
            transform, dataset_name, image_path = future_to_task[future]
            try:
                result = future.result()
                results_by_transform[transform].append(result)
                completed += 1
                
                if completed % 10 == 0:
                    print(f"Progress: {completed}/{len(tasks)} tasks completed")
                    
            except Exception as e:
                print(f"Error processing {transform} on {image_path}: {e}")
                results_by_transform[transform].append({
                    "dataset": dataset_name,
                    "image_path": image_path,
                    "transform": transform,
                    "mse": None,
                    "psnr": None,
                    "original_entropy": None,
                    "transformed_entropy": None,
                    "quantized_entropy": None,
                    "error": str(e)
                })
    
    print(f"\nCompleted all {len(tasks)} tasks")
    
    # Write results to CSV files
    for transform in TRANSFORMS:
        csv_filename = os.path.join(RESULTS_DIR, f"{transform}_results.csv")
        results = sorted(results_by_transform[transform], 
                        key=lambda x: (x["dataset"], x["image_path"]))
        
        # Remove 'transform' field from results as it's redundant (file name indicates transform)
        fieldnames = ["dataset", "image_path", "mse", "psnr",
                     "original_entropy", "transformed_entropy",
                     "quantized_entropy",
                     "encode_ms", "quant_ms", "entropy_enc_ms", "entropy_dec_ms", "dequant_ms", "inverse_ms",
                     "error"]
        # Ensure keys exist for all rows (use None when missing)
        csv_results = []
        for row in results:
            out = {k: row.get(k, None) for k in fieldnames}
            csv_results.append(out)

        def _fmt_stats(vals: List[float]) -> str:
            if not vals:
                return ""
            n = len(vals)
            mean = statistics.mean(vals)
            median = statistics.median(vals)
            mn = min(vals)
            mx = max(vals)
            stdev = statistics.stdev(vals) if n > 1 else 0.0
            return f"count={n}; mean={mean:.6f}; std={stdev:.6f}; min={mn:.6f}; median={median:.6f}; max={mx:.6f}"

        # Collect numeric values (ignore None / errored rows)
        mse_vals = [r["mse"] for r in results if r["mse"] is not None]
        psnr_vals = [r["psnr"] for r in results if r["psnr"] is not None]
        orig_ent_vals = [r["original_entropy"] for r in results if r["original_entropy"] is not None]
        trans_ent_vals = [r["transformed_entropy"] for r in results if r["transformed_entropy"] is not None]
        quant_ent_vals = [r["quantized_entropy"] for r in results if r["quantized_entropy"] is not None]

        encode_vals = [r["encode_ms"] for r in results if r.get("encode_ms") is not None]
        quant_vals = [r["quant_ms"] for r in results if r.get("quant_ms") is not None]
        entropy_enc_vals = [r["entropy_enc_ms"] for r in results if r.get("entropy_enc_ms") is not None]
        entropy_dec_vals = [r["entropy_dec_ms"] for r in results if r.get("entropy_dec_ms") is not None]
        dequant_vals = [r["dequant_ms"] for r in results if r.get("dequant_ms") is not None]
        inverse_vals = [r["inverse_ms"] for r in results if r.get("inverse_ms") is not None]

        error_count = sum(1 for r in results if r["error"])
        total_count = len(results)
        success_count = total_count - error_count

        # Build summary rows appended to CSV (dataset='SUMMARY', image_path identifies the stat row)
        summary_rows = []

        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "counts",
            "mse": "",
            "psnr": "",
            "original_entropy": "",
            "transformed_entropy": "",
            "quantized_entropy": "",
            "error": f"total={total_count}; success={success_count}; errors={error_count}"
        })

        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "mse",
            "mse": _fmt_stats(mse_vals),
            "psnr": "",
            "original_entropy": "",
            "transformed_entropy": "",
            "quantized_entropy": "",
            "error": ""
        })

        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "psnr",
            "mse": "",
            "psnr": _fmt_stats(psnr_vals),
            "original_entropy": "",
            "transformed_entropy": "",
            "quantized_entropy": "",
            "error": ""
        })

        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "original_entropy",
            "mse": "",
            "psnr": "",
            "original_entropy": _fmt_stats(orig_ent_vals),
            "transformed_entropy": "",
            "quantized_entropy": "",
            "error": ""
        })

        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "transformed_entropy",
            "mse": "",
            "psnr": "",
            "original_entropy": "",
            "transformed_entropy": _fmt_stats(trans_ent_vals),
            "quantized_entropy": "",
            "error": ""
        })

        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "quantized_entropy",
            "mse": "",
            "psnr": "",
            "original_entropy": "",
            "transformed_entropy": "",
            "quantized_entropy": _fmt_stats(quant_ent_vals),
            "error": ""
        })

        # Timing summaries
        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "encode_ms",
            "mse": "",
            "psnr": "",
            "original_entropy": "",
            "transformed_entropy": "",
            "quantized_entropy": _fmt_stats(encode_vals),
            "encode_ms": "",
            "quant_ms": "",
            "entropy_enc_ms": "",
            "entropy_dec_ms": "",
            "dequant_ms": "",
            "inverse_ms": "",
            "error": _fmt_stats(encode_vals)
        })

        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "quant_ms",
            "mse": "",
            "psnr": "",
            "original_entropy": "",
            "transformed_entropy": "",
            "quantized_entropy": _fmt_stats(quant_vals),
            "encode_ms": "",
            "quant_ms": "",
            "entropy_enc_ms": "",
            "entropy_dec_ms": "",
            "dequant_ms": "",
            "inverse_ms": "",
            "error": _fmt_stats(quant_vals)
        })

        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "entropy_enc_ms",
            "mse": "",
            "psnr": "",
            "original_entropy": "",
            "transformed_entropy": "",
            "quantized_entropy": _fmt_stats(entropy_enc_vals),
            "encode_ms": "",
            "quant_ms": "",
            "entropy_enc_ms": "",
            "entropy_dec_ms": "",
            "dequant_ms": "",
            "inverse_ms": "",
            "error": _fmt_stats(entropy_enc_vals)
        })

        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "entropy_dec_ms",
            "mse": "",
            "psnr": "",
            "original_entropy": "",
            "transformed_entropy": "",
            "quantized_entropy": _fmt_stats(entropy_dec_vals),
            "encode_ms": "",
            "quant_ms": "",
            "entropy_enc_ms": "",
            "entropy_dec_ms": "",
            "dequant_ms": "",
            "inverse_ms": "",
            "error": _fmt_stats(entropy_dec_vals)
        })

        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "dequant_ms",
            "mse": "",
            "psnr": "",
            "original_entropy": "",
            "transformed_entropy": "",
            "quantized_entropy": _fmt_stats(dequant_vals),
            "encode_ms": "",
            "quant_ms": "",
            "entropy_enc_ms": "",
            "entropy_dec_ms": "",
            "dequant_ms": "",
            "inverse_ms": "",
            "error": _fmt_stats(dequant_vals)
        })

        summary_rows.append({
            "dataset": "SUMMARY",
            "image_path": "inverse_ms",
            "mse": "",
            "psnr": "",
            "original_entropy": "",
            "transformed_entropy": "",
            "quantized_entropy": _fmt_stats(inverse_vals),
            "encode_ms": "",
            "quant_ms": "",
            "entropy_enc_ms": "",
            "entropy_dec_ms": "",
            "dequant_ms": "",
            "inverse_ms": "",
            "error": _fmt_stats(inverse_vals)
        })

        # Append summary rows to the CSV data
        csv_results.extend(summary_rows)
        
        with open(csv_filename, 'w', newline='') as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(csv_results)
        
        print(f"Written {len(results)} results to {csv_filename}")
    
    print("\nAll results written to CSV files in the results/ directory")
    return 0

if __name__ == "__main__":
    exit(main())

