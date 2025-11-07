import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import os

# --- Configuration ---
OUTPUT_DIR = 'extended_results/plots'
INPUT_DIR = 'results'
FILE_PATH = os.path.join(INPUT_DIR, 'extended_experiment_results.csv')
GLOBAL_FIGSIZE = (12, 10) # Bigger figure size for better resolution and clarity
DPI = 300 # Set a higher DPI for saving plots

# --- Consistent Color/Order Definitions ---
# Use these to ensure colors and order are the same across all plots
# (Updated with user's specific transform and dataset names)
TRANSFORM_ORDER = ['DCT', 'DFT', 'HAAR', 'SP']
DATASET_ORDER = ['SquaredKodak', 'SquaredIconsSample', 'SquaredAerialImages', 'SquaredTextureImages']

# Colors will be dynamically generated in main() based on these lists
TRANSFORM_COLORS = {}
DATASET_COLORS = {}


def generate_parametric_plot(df, transform_name="All"):
    """Generates the parametric plot (Plot 1)."""
    
    # Filter data if a specific transform is requested
    if transform_name != "All":
        df = df[df['transform'] == transform_name].copy()
        output_filename = f'plot_1_parametric_curve_{transform_name}.png'
    else:
        output_filename = 'plot_1_parametric_curve_All.png'

    # Group by all categorical variables AND the quantization_scale to trace the performance curve
    avg_df = df.groupby(['dataset', 'transform', 'quantization_scale'], as_index=False).agg({
        'compression_ratio': 'mean',
        'psnr': 'mean'
    }).sort_values(by='quantization_scale') # Sort by quant scale to ensure lines connect logically

    plt.figure(figsize=GLOBAL_FIGSIZE)
    
    # Use lineplot to connect the points for each (dataset, transform) group
    sns.lineplot(
        data=avg_df,
        x='compression_ratio',
        y='psnr',
        hue='dataset',
        hue_order=DATASET_ORDER,
        palette=DATASET_COLORS,
        style='transform' if transform_name == "All" else None,
        style_order=TRANSFORM_ORDER if transform_name == "All" else None,
        markers=True,
        dashes=False,
        linewidth=2,
        legend='full'
    )

    title = f'1. Parametric Performance Curves by Quantization Scale (Transform: {transform_name})'
    plt.title(title, fontsize=18, fontweight='bold')
    plt.xlabel('Average Compression Ratio (CR)', fontsize=16)
    plt.ylabel('Average PSNR (dB)', fontsize=16)
    plt.legend(title='Experiment Group' if transform_name == "All" else 'Dataset', bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.tight_layout()
    
    save_path = os.path.join(OUTPUT_DIR, output_filename)
    plt.savefig(save_path, dpi=DPI)
    print(f"Saved {output_filename}")
    plt.close() # Close figure to free memory

def generate_scatter_plot(df, transform_name="All"):
    """Generates the scatter plot (Plot 2)."""
    
    # Filter data if a specific transform is requested
    if transform_name != "All":
        df = df[df['transform'] == transform_name].copy()
        output_filename = f'plot_2_full_scatter_{transform_name}.png'
    else:
        output_filename = 'plot_2_full_scatter_All.png'

    plt.figure(figsize=GLOBAL_FIGSIZE)
    sns.scatterplot(
        data=df,
        x='compression_ratio',
        y='psnr',
        hue='dataset',
        hue_order=DATASET_ORDER,
        palette=DATASET_COLORS,
        style='transform' if transform_name == "All" else None,
        style_order=TRANSFORM_ORDER if transform_name == "All" else None,
        alpha=0.5,
        s=50,
    )

    title = f'2. Individual Experiment Results (CR vs. PSNR) (Transform: {transform_name})'
    plt.title(title, fontsize=18, fontweight='bold')
    plt.xlabel('Compression Ratio (CR)', fontsize=16)
    plt.ylabel('PSNR (dB)', fontsize=16)
    plt.legend(title='Experiment Parameters' if transform_name == "All" else 'Dataset', bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.tight_layout()

    save_path = os.path.join(OUTPUT_DIR, output_filename)
    plt.savefig(save_path, dpi=DPI)
    print(f"Saved {output_filename}")
    plt.close() # Close figure to free memory

def _filter_and_bin_data(df):
    """
    Helper function to filter data to CR intersection and apply quantile bins.
    Returns the filtered and binned DataFrame.
    """
    # 1. Filter Data to Intersection of Compression Ratio Ranges
    cr_ranges = df.groupby('transform')['compression_ratio'].agg(['min', 'max'])
    global_cr_min = cr_ranges['min'].max()
    global_cr_max = cr_ranges['max'].min()
    
    print(f"\nFiltering CR range for binned plots to intersection: [{global_cr_min:.2f}, {global_cr_max:.2f}]")

    filtered_df = df[
        (df['compression_ratio'] >= global_cr_min) & 
        (df['compression_ratio'] <= global_cr_max)
    ].copy()

    if filtered_df.empty:
        print("ERROR: Intersection filter resulted in an empty dataset. Cannot generate binned plots.")
        return None

    # 2. Define CR Bins using Quantiles on the FILTERED data
    try:
        # Create 4 quantile bins (quartiles)
        filtered_df['cr_bin'] = pd.qcut(filtered_df['compression_ratio'], q=4, duplicates='drop', precision=2)
    except ValueError as e:
        # Fallback to linear spacing if quantile binning fails
        print(f"Could not create 4 quantile bins on filtered data. Falling back to linear spacing: {e}")
        cr_min = filtered_df['compression_ratio'].min()
        cr_max = filtered_df['compression_ratio'].max()
        bins = np.linspace(cr_min * 0.99, cr_max * 1.01, 5) 
        filtered_df['cr_bin'] = pd.cut(filtered_df['compression_ratio'], bins=bins, include_lowest=True, right=True)
    
    return filtered_df

def generate_binned_boxplots(df):
    """
    Generates Box Plots for PSNR distribution, binned by CR quantiles
    after filtering to the CR intersection range.
    """
    print("\nGenerating Binned Box Plots (PSNR distribution per CR quantile bin)...")
    
    filtered_df = _filter_and_bin_data(df)
    if filtered_df is None:
        return # Stop if filtering failed

    # Iterate through Bins and Plot
    unique_bins = sorted(filtered_df['cr_bin'].unique(), key=lambda x: x.left)
    
    for cr_bin in unique_bins:
        bin_df = filtered_df[filtered_df['cr_bin'] == cr_bin].copy()
        
        if bin_df.empty:
            print(f"Skipping empty bin: {cr_bin}")
            continue

        bin_str = f"({cr_bin.left:.2f} - {cr_bin.right:.2f}]"
        safe_bin_str = f"{cr_bin.left:.2f}_to_{cr_bin.right:.2f}"
        output_filename = f'plot_3_boxplot_CR_Bin_Quantile_Intersection_{safe_bin_str}.png'
        
        plt.figure(figsize=GLOBAL_FIGSIZE)
        
        sns.boxplot(
            data=bin_df,
            x='dataset',
            order=DATASET_ORDER,
            y='psnr',
            hue='transform',
            hue_order=TRANSFORM_ORDER,
            palette=TRANSFORM_COLORS,
            dodge=True,
            showfliers=False
        )

        title = f'3. PSNR Distribution by Transform and Dataset (Quantile Bin)\n(Filtered Intersection CR Range: {bin_str})'
        plt.title(title, fontsize=18, fontweight='bold')
        plt.xlabel('Dataset', fontsize=16)
        plt.ylabel('PSNR (dB) Distribution', fontsize=16)

        plt.legend(title='Transform', bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(axis='y', linestyle='--', alpha=0.7)
        plt.tight_layout()
        
        save_path = os.path.join(OUTPUT_DIR, output_filename)
        plt.savefig(save_path, dpi=DPI)
        print(f"Saved {output_filename}")
        plt.close()

def generate_binned_bar_plots(df):
    """
    Generates Bar Plots for mean PSNR, binned by CR quantiles
    after filtering to the CR intersection range.
    """
    print("\nGenerating Binned Bar Plots (Mean PSNR per CR quantile bin)...")
    
    filtered_df = _filter_and_bin_data(df)
    if filtered_df is None:
        return # Stop if filtering failed

    # Iterate through Bins and Plot
    unique_bins = sorted(filtered_df['cr_bin'].unique(), key=lambda x: x.left)
    
    for cr_bin in unique_bins:
        bin_df = filtered_df[filtered_df['cr_bin'] == cr_bin].copy()
        
        if bin_df.empty:
            print(f"Skipping empty bin: {cr_bin}")
            continue
            
        bin_str = f"({cr_bin.left:.2f} - {cr_bin.right:.2f}]"
        safe_bin_str = f"{cr_bin.left:.2f}_to_{cr_bin.right:.2f}"
        output_filename = f'plot_4_barplot_CR_Bin_Quantile_Intersection_{safe_bin_str}.png'
        
        plt.figure(figsize=GLOBAL_FIGSIZE)
        
        # Create the bar plot structure for Mean PSNR within this CR bin
        sns.barplot(
            data=bin_df,
            x='dataset',
            order=DATASET_ORDER,
            y='psnr',
            hue='transform',
            hue_order=TRANSFORM_ORDER,
            palette=TRANSFORM_COLORS,
            dodge=True,
            # ci=None, # Deprecated in newer seaborn
            errorbar=None # Hide error bars for a cleaner look, or use 'sd'
        )

        title = f'4. Mean PSNR by Transform and Dataset (Quantile Bin)\n(Filtered Intersection CR Range: {bin_str})'
        plt.title(title, fontsize=18, fontweight='bold')
        plt.xlabel('Dataset', fontsize=16)
        plt.ylabel('Mean PSNR (dB)', fontsize=16)
        
        # Optional: Add mean value labels on top of bars
        # This can get cluttered, uncomment if needed
        # ax = plt.gca()
        # for p in ax.patches:
        #     ax.annotate(f"{p.get_height():.2f}", 
        #                   (p.get_x() + p.get_width() / 2., p.get_height()), 
        #                   ha='center', va='center', 
        #                   xytext=(0, 9), 
        #                   textcoords='offset points',
        #                   fontsize=9)

        plt.legend(title='Transform', bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.grid(axis='y', linestyle='--', alpha=0.7)
        plt.tight_layout()
        
        save_path = os.path.join(OUTPUT_DIR, output_filename)
        plt.savefig(save_path, dpi=DPI)
        print(f"Saved {output_filename}")
        plt.close()


def generate_plots(df):
    """Generates all requested plots."""
    sns.set_theme(style="whitegrid")
    # Note: We are now using custom palettes defined in Configuration
    plt.style.use('ggplot')

    # --- 1. Generate ALL-DATA plots first ---
    print("Generating 'All Transforms' plots...")
    generate_parametric_plot(df, transform_name="All")
    generate_scatter_plot(df, transform_name="All")
    
    # --- 2. Generate Binned Plots (Box and Bar) ---
    # These functions have their own filtering and binning logic
    generate_binned_boxplots(df) 
    generate_binned_bar_plots(df)

    # --- 3. Generate PER-TRANSFORM plots ---
    unique_transforms = df['transform'].unique()
    print(f"\nGenerating individual plots for transforms: {', '.join(unique_transforms)}...")

    for transform in unique_transforms:
        generate_parametric_plot(df, transform_name=transform)
        generate_scatter_plot(df, transform_name=transform)
    
    print("\nAll plots generated successfully.")


def main():
    """Main function to load data and generate plots."""
    # Ensure output directory exists before loading/creating files
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    # Attempt to load the user's data
    try:
        df = pd.read_csv(FILE_PATH)
        
        # Ensure categorical columns are present for grouping
        required_cols = ['dataset', 'transform', 'compression_ratio', 'psnr', 'quantization_scale']
        missing_cols = [col for col in required_cols if col not in df.columns]
        
        if missing_cols:
            raise ValueError(f"CSV is missing required columns (all lowercase): {', '.join(missing_cols)}")
            
        print(f"Successfully loaded data from '{FILE_PATH}'.")

        # Update global color/order lists if data has different/more labels
        global TRANSFORM_ORDER, TRANSFORM_COLORS, DATASET_ORDER, DATASET_COLORS
        
        db_transforms = df['transform'].unique()
        db_datasets = df['dataset'].unique()
        
        # Check if DB values are different from config. Use config order if all items are present,
        # otherwise, fall back to data order.
        
        # --- Validate Transforms ---
        config_transform_set = set(TRANSFORM_ORDER)
        data_transform_set = set(db_transforms)

        if not data_transform_set.issubset(config_transform_set):
            missing_in_config = data_transform_set - config_transform_set
            print(f"Warning: Transforms in data {missing_in_config} are not in config. Adding them.")
            TRANSFORM_ORDER.extend(list(missing_in_config))
        elif not config_transform_set.issubset(data_transform_set):
            missing_in_data = config_transform_set - data_transform_set
            print(f"Warning: Transforms in config {missing_in_data} are not in data. Removing them.")
            TRANSFORM_ORDER = [t for t in TRANSFORM_ORDER if t in data_transform_set]

        # --- Validate Datasets ---
        config_dataset_set = set(DATASET_ORDER)
        data_dataset_set = set(db_datasets)
        
        if not data_dataset_set.issubset(config_dataset_set):
            missing_in_config = data_dataset_set - config_dataset_set
            print(f"Warning: Datasets in data {missing_in_config} are not in config. Adding them.")
            DATASET_ORDER.extend(list(missing_in_config))
        elif not config_dataset_set.issubset(data_dataset_set):
            missing_in_data = config_dataset_set - data_dataset_set
            print(f"Warning: Datasets in config {missing_in_data} are not in data. Removing them.")
            DATASET_ORDER = [d for d in DATASET_ORDER if d in data_dataset_set]

        # --- Dynamically Generate Color Palettes ---
        transform_palette = sns.color_palette("tab10", len(TRANSFORM_ORDER)).as_hex()
        TRANSFORM_COLORS = {name: color for name, color in zip(TRANSFORM_ORDER, transform_palette)}

        dataset_palette = sns.color_palette("Dark2", len(DATASET_ORDER)).as_hex()
        DATASET_COLORS = {name: color for name, color in zip(DATASET_ORDER, dataset_palette)}
        
        print(f"--- Using Transforms: {TRANSFORM_ORDER}")
        print(f"--- Using Datasets: {DATASET_ORDER}")

        generate_plots(df)

    except (FileNotFoundError, ValueError) as e:
        print(f"ERROR: Could not run plotting script.")
        print(f"Details: {e}")
        print(f"Please ensure '{FILE_PATH}' exists and contains all required columns in lowercase.")
        # Exit gracefully if data isn't loaded
        return

if __name__ == "__main__":
    main()