import argparse
import csv
import multiprocessing as mp
import os
import subprocess
from pathlib import Path
import time
from typing import List, Tuple, Any

# --- Configuration ---
# Define the name of your executable
EXECUTABLE_NAME = 'simple_pipeline' 
# Define the output CSV file name
OUTPUT_CSV_FILENAME = 'experiment_results.csv'
# Define the header for the output CSV file
CSV_HEADERS = [
    'folder_path',
    'file_name',
    'quantization_scale',
    'transform_type',
    'compression_ratio',
    'entropy_quantized',
    'mse',
    'psnr',
    'encoding_time',
    'decoding_time'
]

# --- Core Functionality ---

def process_image(task: Tuple[Path, str, int]) -> Tuple[Path, str, int, Any]:
    """
    Worker function to call the external executable for a single image, transform, and scale.

    The executable is expected to print a single line of output in the required format.

    :param task: A tuple (image_path, transform_type, quantization_scale).
    :return: A tuple containing (image_path, transform_type, quantization_scale, results_tuple).
             The results_tuple must be: (cr, eq, mse, psnr, enc_time, dec_time).
    """
    image_path, transform_type, quantization_scale = task
    
    # 1. Construct the Command-Line Arguments
    # The required inputs are: Transform type, Image Path, Quantization Scale
    command = [
        EXECUTABLE_NAME,
        transform_type,
        str(image_path), # Ensure path is passed as a string
        str(quantization_scale) # Ensure scale is passed as a string argument
    ]
    
    print(f"Executing: {command}")

    # 2. Execute the External Program and Capture Output
    try:
        # subprocess.run is used to execute the command.
        # - capture_output=True: captures stdout and stderr.
        # - text=True: decodes stdout/stderr as text (str).
        # - check=True: raises CalledProcessError if the command returns a non-zero exit code.
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            check=True, 
            timeout=300 # Optional: Set a timeout (e.g., 5 minutes) in case the process hangs
        )
        
        # The output is expected to be a single line:
        # (compression ratio, entropy quantized, mse, psnr, encoding time, decoding time)
        raw_output = result.stdout.strip()
        
        # 3. Parse the Output String
        # Strip parentheses and split by comma.
        if not raw_output.startswith('(') or not raw_output.endswith(')'):
             raise ValueError(f"Output format error: Expected '(...,...,...)' but got: {raw_output}")

        clean_output = raw_output.strip('()')
        
        # Convert the comma-separated strings to floats
        results_tuple = tuple(float(val.strip()) for val in clean_output.split(','))
        
        if len(results_tuple) != 6:
            raise ValueError(f"Expected 6 output values, but found {len(results_tuple)}.")
        
        print(f"Success for {image_path.name}: {results_tuple}")
        return (image_path, transform_type, quantization_scale, results_tuple)

    except subprocess.CalledProcessError as e:
        print(f"Error processing {image_path.name} (Non-zero exit code):")
        print(f"  STDERR: {e.stderr.strip()}")
        # Return error state
        error_tuple = (float('nan'), float('nan'), float('nan'), float('nan'), float('nan'), float('nan'))
        return (image_path, transform_type, quantization_scale, error_tuple)
    except subprocess.TimeoutExpired:
        print(f"Error processing {image_path.name} (Timeout Expired).")
        error_tuple = (float('nan'), float('nan'), float('nan'), float('nan'), float('nan'), float('nan'))
        return (image_path, transform_type, quantization_scale, error_tuple)
    except Exception as e:
        # Handles ValueError (parsing error) or other unexpected exceptions
        print(f"Error processing {image_path.name} ({type(e).__name__}): {e}")
        error_tuple = (float('nan'), float('nan'), float('nan'), float('nan'), float('nan'), float('nan'))
        return (image_path, transform_type, quantization_scale, error_tuple)
    
# --- Task Management (No Change) ---

def create_tasks(image_dir: Path, transforms: List[str], scales: List[int]) -> List[Tuple[Path, str, int]]:
    """
    Recursively finds all .png files and generates a list of (image_path, transform, scale) tasks.
    """
    # Using Path.rglob() to recursively find .png files
    image_paths = list(image_dir.rglob('*.png'))
    if not image_paths:
        raise FileNotFoundError(f"No .png files found recursively in directory: {image_dir}")

    tasks = []
    for img_path in image_paths:
        for transform in transforms:
            for scale in scales:
                tasks.append((img_path, transform, scale))
    
    print(f"Found {len(image_paths)} images. Creating {len(tasks)} total tasks.")
    return tasks

# --- CSV Writer (No Change) ---

def write_results_to_csv(results: List[Tuple[Path, str, int, Any]]):
    """
    Writes the collected results to a CSV file.
    """
    print(f"\nWriting results to {OUTPUT_CSV_FILENAME}...")
    
    with open(OUTPUT_CSV_FILENAME, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(CSV_HEADERS) # Write header row

        for image_path, transform_type, quantization_scale, result_tuple in results:
            folder_path = str(image_path.parent)
            file_name = image_path.name

            # Construct the final row
            row = [
                folder_path,
                file_name,
                quantization_scale,
                transform_type,
                *result_tuple # Unpack the 6-element result tuple
            ]
            writer.writerow(row)
    
    print("CSV generation complete.")

# --- Argument Parsing (No Change) ---

def parse_args():
    """Parses command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Run image processing script in parallel for various transforms and quantization scales.",
        formatter_class=argparse.RawTextHelpFormatter
    )
    
    parser.add_argument(
        '--transforms', 
        type=str, 
        required=True,
        nargs='+',
        help="List of transform types to use (e.g., --transforms 'wavelet' 'dct')"
    )
    parser.add_argument(
        '--image-dir', 
        type=str, 
        required=True,
        help="The root directory containing .png files (will search recursively)."
    )
    parser.add_argument(
        '--scales', 
        type=int, 
        required=True,
        nargs='+',
        help="List of quantization scales (integers) to use (e.g., --scales 10 20 30)"
    )

    args = parser.parse_args()
    return args

# --- Main Execution (Minor Change for Process Count) ---

def main():
    """Main function to orchestrate the parallel processing."""
    args = parse_args()

    # 1. Prepare image directory and scales
    image_dir = Path(args.image_dir).resolve()
    if not image_dir.is_dir():
        print(f"Error: Directory not found or not a directory: {image_dir}")
        return

    # 2. Create the task list
    try:
        quant_scales = args.scales
        transforms = args.transforms
        tasks = create_tasks(image_dir, transforms, quant_scales)
    except FileNotFoundError as e:
        print(f"Error: {e}")
        return
    except Exception as e:
        print(f"An unexpected error occurred during task creation: {e}")
        return

    # 3. Parallel Execution using a Pool
    # Since the work is now delegated to an *external* executable, we can potentially 
    # use more processes than CPU cores, especially if the executable is I/O-bound.
    # However, a safe default is to use the number of cores.
    num_processes = mp.cpu_count() 
    print(f"Starting parallel processing with {num_processes} worker processes for executable '{EXECUTABLE_NAME}'...")

    start_time = time.time()
    
    # Use Pool to execute the process_image function across multiple processes
    with mp.Pool(processes=num_processes) as pool:
        all_results = pool.map(process_image, tasks)

    end_time = time.time()
    print(f"\nAll tasks finished in {end_time - start_time:.2f} seconds.")

    # 4. Write results to CSV
    write_results_to_csv(all_results)
    print(f"Total entries written to CSV: {len(all_results)}")


if __name__ == '__main__':
    main()