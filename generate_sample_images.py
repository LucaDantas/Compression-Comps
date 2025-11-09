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
QUANTIZATION_SCALES = [i for i in range(1, 31, 2)]
TRANSFORMS = ["SP", "HAAR", "DCT", "DFT"]
IMAGE = "Datasets/SquaredKodak/1.png"
EXECUTABLE = "./pipeline_data_collection"
RESULTS_DIR = "results/sample_images"


def run_pipeline(transform: str, image_path: str, quant_scale: float, save_flag: str) -> None:
    """
    Run simple_pipeline for a single image and transform.
    Returns a dict with the results.
    """
    subprocess.run(
        [EXECUTABLE, transform, image_path, str(quant_scale), save_flag],
        capture_output=False,
        text=True,
        timeout=300  # 5 minute timeout per image
    )
    
def main():
    # Check if executable exists
    if not os.path.exists(EXECUTABLE):
        print(f"Error: Executable {EXECUTABLE} not found. Please compile simple_pipeline.cpp first.")
        return 1
    
    # Create results directory
    os.makedirs(RESULTS_DIR, exist_ok=True)
    
    # Prepare all tasks
    tasks = []
    for transform in TRANSFORMS:
        for quant_scale in QUANTIZATION_SCALES:
            save_flag = os.path.join(RESULTS_DIR, f"{transform}_compressionRatio=")
            tasks.append((transform, IMAGE, quant_scale, save_flag))
    
    
    # Run tasks in parallel
    results = []
    completed = 0
    
    with ProcessPoolExecutor() as executor:
        # Submit all tasks
        future_to_task = {
            executor.submit(run_pipeline, transform, image_path, quant_scale, save_flag): (transform, image_path, quant_scale, save_flag)
            for transform, image_path, quant_scale, save_flag in tasks
        }
        
        # Collect results as they complete
        for future in as_completed(future_to_task):
            transform, image_path, quant_scale, save_flag = future_to_task[future]
            result = future.result()
            results.append(result)
            completed += 1
            
            if completed % 10 == 0:
                print(f"Progress: {completed}/{len(tasks)} tasks completed")
                    
    
    print(f"\nCompleted all {len(tasks)} tasks")
    

if __name__ == "__main__":
    exit(main())

