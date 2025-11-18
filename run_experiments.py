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
import random
# from wakepy import keep

# Configuration
QUANTIZATION_SCALES = {
    "SP": [0.5 * (1.3**i) for i in range(20)],
    "HAAR": [1.3**(i-1) for i in range(20)],
    "DCT": [0.1 * 1.2**(i) for i in range(20)],
    "DFT": [1.3**(i-1) for i in range(20)]
}
TRANSFORMS = ["HAAR", "SP", "DCT", "DFT"]
DATASETS_DIR = "Datasets"
EXECUTABLE = "./pipeline_data_collection"
RESULTS_DIR = "results"

def find_all_images() -> List[Tuple[str, str]]:
    """
    Find PNG images in all dataset subdirectories, sampling 25 images per dataset.
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
            dataset_images = []
            
            for image_file in dataset_dir.glob("*.png"):
                image_path = str(image_file)
                match = re.search(r"[\d]+.png", image_path)
                if match:
                    num = int(match.group()[:-4])
                dataset_images.append((dataset_name, image_path))

            images.extend(sorted(dataset_images))
            print(f"Using all {len(dataset_images)} images from {dataset_name}")
    
    return sorted(images)

def parse_output(output: str) -> Tuple[float, float, float, float, float, float, float, float, float]:
    """
    Parse the output from pipeline_data_collection.
    Expected format: (compression_ratio, direct_compression_ratio, original_entropy, transformed_entropy, quantized_entropy, mse, psnr, encoding_time, decoding_time)
    Returns: (compression_ratio, direct_compression_ratio, original_entropy, transformed_entropy, quantized_entropy, mse, psnr, encoding_time, decoding_time)
    """

    compression_ratio, direct_compression_ratio, original_entropy, transformed_entropy, quantized_entropy, mse, psnr, encoding_time, decoding_time = map(float, output.strip()[1:-1].split(','))
    return (compression_ratio, direct_compression_ratio, original_entropy, transformed_entropy, quantized_entropy, mse, psnr, encoding_time, decoding_time)

def run_pipeline(transform: str, image_path: str, quant_scale: float, save_flag: str) -> Dict:
    """
    Run simple_pipeline for a single image and transform.
    Returns a dict with the results.
    """
    result = subprocess.run(
        [EXECUTABLE, transform, image_path, str(quant_scale), save_flag],
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
            "quantization_scale": quant_scale,
            "decode_ms": None,
            "encode_ms": None,
            "compression_ratio": None,
            "direct_compression_ratio": None,
            "error": error_msg
        }
    
    output = result.stdout.strip()
    (compression_ratio, direct_compression_ratio, original_entropy, transformed_entropy, quantized_entropy, mse, psnr, encoding_time, decoding_time) = parse_output(output)

    return {
        "dataset": Path(image_path).parent.name,
        "image_path": image_path,
        "transform": transform,
        "mse": mse,
        "psnr": psnr,
        "compression_ratio": compression_ratio,
        "direct_compression_ratio": direct_compression_ratio,
        "original_entropy": original_entropy,
        "transformed_entropy": transformed_entropy,
        "quantized_entropy": quantized_entropy,
        "quantization_scale": quant_scale,
        "encode_ms": encoding_time,
        "decode_ms": decoding_time,
        "error": None
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
            for quant_scale in QUANTIZATION_SCALES[transform]:
                tasks.append((transform, dataset_name, image_path, quant_scale, "no_save"))
    
    print(f"Total tasks: {len(tasks)} ({len(TRANSFORMS)} transforms × {len(images)} images)")
    
    # Run tasks in parallel
    results = []
    completed = 0
    
    with ProcessPoolExecutor() as executor:
        # Submit all tasks
        future_to_task = {
            executor.submit(run_pipeline, transform, image_path, quant_scale, save_flag): (transform, dataset_name, image_path, quant_scale, save_flag)
            for transform, dataset_name, image_path, quant_scale, save_flag in tasks
        }
        
        # Collect results as they complete
        for future in as_completed(future_to_task):
            transform, dataset_name, image_path, quant_scale, save_flag = future_to_task[future]
            result = future.result()
            results.append(result)
            completed += 1
            
            if completed % 10 == 0:
                print(f"Progress: {completed}/{len(tasks)} tasks completed")
                    
    
    print(f"\nCompleted all {len(tasks)} tasks")
    
    # Write results to CSV files
    csv_filename = os.path.join(RESULTS_DIR, "extended_experiment_results.csv")
    results = sorted(results, 
                    key=lambda x: (x["dataset"], x["image_path"], x["transform"]))
    
    # Remove 'transform' field from results as it's redundant (file name indicates transform)
    fieldnames = [
                    "transform", 
                    "dataset", 
                    "image_path", 
                    "mse", 
                    "psnr",
                    "compression_ratio",
                    "direct_compression_ratio",
                    "original_entropy",
                    "transformed_entropy",
                    "quantized_entropy",
                    "quantization_scale",
                    "encode_ms", 
                    "decode_ms",
                    "error"
                  ]
    # Ensure keys exist for all rows (use None when missing)
    csv_results = []
    for row in results:
        out = {k: row.get(k, None) for k in fieldnames}
        csv_results.append(out)
    
    with open(csv_filename, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(csv_results)
    
    print(f"Written {len(results)} results to {csv_filename}")
    
    print("\nAll results written to CSV files in the results/ directory")
    return 0

if __name__ == "__main__":
    main()
    # with keep.running(on_fail="warn"):
    #     exit(main())

