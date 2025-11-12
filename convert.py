import os
import math
import argparse
from PIL import Image
from pdf2image import convert_from_path

def get_next_power_of_2(n):
    """
    Calculates the next power of 2 greater than or equal to n.
    If n is already a power of 2, it returns n.
    
    Examples:
    get_next_power_of_2(700) -> 1024
    get_next_power_of_2(1024) -> 1024
    get_next_power_of_2(1025) -> 2048
    """
    if n <= 0:
        return 1  # Smallest power of 2
    
    # Check if n is already a power of 2
    is_power_of_2 = (n & (n - 1) == 0)
    if is_power_of_2:
        return n
        
    # Calculate the next power of 2
    # math.log(n, 2) gives the exponent
    # math.ceil rounds up to the next integer exponent
    # 2**exponent gives the power of 2
    return 2**(math.ceil(math.log(n, 2)))

def process_pdf(pdf_path, output_dir, dpi=300, target_size=None):
    """
    Converts each page of a PDF to a PNG, then pads it with whitespace.
    
    If target_size is given, it downsamples to fit within target_size
    and pads to target_size.
    
    If target_size is None, it pads to the next square power of 2.
    """
    # Ensure the output directory exists
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"Starting conversion for: {pdf_path}...")
    
    try:
        # 1. Convert PDF to a list of PIL Image objects
        # dpi (dots per inch) controls the output resolution (image size)
        images = convert_from_path(pdf_path, dpi=dpi)
        
        print(f"Found {len(images)} page(s) to process.")
        
        for i, img in enumerate(images):
            # 2. Find the original dimensions
            width, height = img.size
            print(f"  Processing Page {i+1}: Original size {width}x{height}px")

            # 3. Determine the target size
            if target_size:
                # New logic: User specified a fixed target size
                final_target_size = target_size
                print(f"    Downsampling to fit within {final_target_size}x{final_target_size}...")
                
                # Downsample the image in-place to fit within the target box
                # We use Image.Resampling.LANCZOS for high-quality downsampling
                img.thumbnail((final_target_size, final_target_size), Image.Resampling.LANCZOS)
                
                # Get the new, downsampled dimensions
                width, height = img.size
                print(f"    Downsampled size: {width}x{height}px")
            else:
                # Old logic: Find the next power of 2
                max_dim = max(width, height)
                final_target_size = get_next_power_of_2(max_dim)
                print(f"    Target square size (next power of 2): {final_target_size}x{final_target_size}px")

            # 4. Create the new, padded image
            # We create a new image in 'RGB' mode with a white background
            # 'RGB' is safer than 'RGBA' to avoid transparency issues
            padded_img = Image.new("RGB", (final_target_size, final_target_size), (255, 255, 255))

            # 5. Calculate position to paste the original (to center it)
            paste_x = (final_target_size - width) // 2
            paste_y = (final_target_size - height) // 2

            # 6. Paste the original image onto the new canvas
            padded_img.paste(img, (paste_x, paste_y))

            # 7. Save the final image
            # Get the original PDF filename without the extension
            base_filename = os.path.splitext(os.path.basename(pdf_path))[0]
            output_filename = os.path.join(output_dir, f"{base_filename}_page_{i+1}_padded.png")
            
            padded_img.save(output_filename, "PNG")
            print(f"    Successfully saved: {output_filename}")

        print("Processing complete.")

    except Exception as e:
        print(f"An error occurred: {e}")
        print("Please ensure 'poppler' is installed and in your system's PATH.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert PDF pages to PNGs padded to the next power of 2."
    )
    
    parser.add_argument(
        "pdf_file", 
        type=str, 
        help="The path to the PDF file you want to convert."
    )
    
    parser.add_argument(
        "--output-dir", 
        "-o", 
        type=str, 
        default="output_images", 
        help="The directory to save the output PNG files. (default: 'output_images')"
    )
    
    parser.add_argument(
        "--dpi", 
        "-d", 
        type=int, 
        default=300, 
        help="The resolution (dots per inch) for the conversion. Higher DPI = larger images. (default: 300)"
    )
    
    parser.add_argument(
        "--target-size", 
        "-t", 
        type=int, 
        default=None, 
        help="A fixed target size (e.g., 2048). If set, downsamples to fit "
             "and pads to this square size. "
             "If not set, pads to the next power of 2."
    )
    
    args = parser.parse_args()
    
    process_pdf(args.pdf_file, args.output_dir, args.dpi, args.target_size)