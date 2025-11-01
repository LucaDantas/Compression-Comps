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

def parse_output(output: str) -> Tuple[float, float, float, float, float]:
    """
    Parse the output from simple_pipeline.
    Expected format: (mse, psnr, original_entropy, transformed_entropy, quantized_entropy)
    """
    # Remove parentheses and split by comma
    output = output.strip()
    if output.startswith("(") and output.endswith(")"):
        output = output[1:-1]
    
    parts = [x.strip() for x in output.split(",")]
    if len(parts) != 5:
        raise ValueError(f"Expected 5 values, got {len(parts)}: {output}")
    
    try:
        mse = float(parts[0])
        psnr = float(parts[1])
        original_entropy = float(parts[2])
        transformed_entropy = float(parts[3])
        quantized_entropy = float(parts[4])
        return mse, psnr, original_entropy, transformed_entropy, quantized_entropy
    except ValueError as e:
        raise ValueError(f"Could not parse values: {output}") from e

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
        mse, psnr, orig_ent, trans_ent, quant_ent = parse_output(output)
        
        return {
            "dataset": Path(image_path).parent.name,
            "image_path": image_path,
            "transform": transform,
            "mse": mse,
            "psnr": psnr,
            "original_entropy": orig_ent,
            "transformed_entropy": trans_ent,
            "quantized_entropy": quant_ent,
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
                     "quantized_entropy", "error"]
        csv_results = [{k: row[k] for k in fieldnames} for row in results]
        
        with open(csv_filename, 'w', newline='') as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(csv_results)
        
        print(f"Written {len(results)} results to {csv_filename}")
    
    print("\nAll results written to CSV files in the results/ directory")
    return 0

if __name__ == "__main__":
    exit(main())

