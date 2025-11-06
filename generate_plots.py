import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import os

# --- Configuration ---
FILE_PATH = 'results/experiment_results.csv'

def generate_plots(df):
    """Generates the three requested plots using the provided DataFrame."""
    sns.set_theme(style="whitegrid")
    sns.set_palette("tab10")
    plt.style.use('ggplot')

    # --- 1. Parametric Plot (Performance Curves by Quantization Scale) ---
    print("\nGenerating Plot 1: Parametric Plot (Curves per Group, points are Quantization Scales)...")

    # Group by all categorical variables AND the quantization_scale to trace the performance curve
    avg_df = df.groupby(['dataset', 'transform', 'quantization_scale'], as_index=False).agg({
        'compression_ratio': 'mean',
        'psnr': 'mean'
    }).sort_values(by='quantization_scale') # Sort by quant scale to ensure lines connect logically

    plt.figure(figsize=(10, 8))
    
    # Use lineplot to connect the points for each (dataset, transform) group
    sns.lineplot(
        data=avg_df,
        x='compression_ratio',
        y='psnr',
        hue='dataset',         # Color by dataset
        style='transform',       # Line style/dash by transform
        markers=True,            # Show markers at each quantization_scale
        dashes=False,            # Use solid lines
        palette='colorblind',
        linewidth=2,
        legend='full'
    )

    # Add text labels for the quantization_scale at each point
    for _, row in avg_df.iterrows():
        plt.text(
            row['compression_ratio'] + 0.01,
            row['psnr'] + 0.1,
            f"Q={int(row['quantization_scale'])}", # Label with Q scale
            fontsize=8,
            ha='left',
            weight='bold',
            alpha=0.8
        )

    plt.title('1. Parametric Performance Curves by Quantization Scale (Q)', fontsize=16, fontweight='bold')
    plt.xlabel('Average compression_ratio (CR)', fontsize=14)
    plt.ylabel('Average psnr (dB)', fontsize=14)
    plt.legend(title='Experiment Group', bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.tight_layout()
    plt.savefig('plot_1_parametric_curve.png')
    print("Saved plot_1_parametric_curve.png")


    # --- 2. Scatter Plot (Individual Points, color by dataset, shape by transform) ---
    print("Generating Plot 2: Full Scatter Plot (Individual Points)...")
    plt.figure(figsize=(12, 8))
    sns.scatterplot(
        data=df,
        x='compression_ratio',
        y='psnr',
        hue='dataset',         # Color by dataset
        style='transform',       # Shape by transform
        alpha=0.5,               # Use transparency to show density
        s=50,
        palette='viridis'
    )

    plt.title('2. Individual Experiment Results (CR vs. psnr)', fontsize=16, fontweight='bold')
    plt.xlabel('compression_ratio (CR)', fontsize=14)
    plt.ylabel('psnr (dB)', fontsize=14)
    plt.legend(title='Experiment Parameters', bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.tight_layout()
    plt.savefig('plot_2_full_scatter.png')
    print("Saved plot_2_full_scatter.png")


    # --- 3. Box Plot (psnr by dataset/transform, labeled with Average CR) ---
    print("Generating Plot 3: psnr Box Plot with CR Labels...")

    plt.figure(figsize=(14, 8))
    # Create the box plot structure
    sns.boxplot(
        data=df,
        x='dataset',
        y='psnr',
        hue='transform',
        palette='Pastel1',
        dodge=True,
        showfliers=False # Hide outliers for cleaner visualization
    )
    plt.title('3. psnr Distribution by transform and dataset', fontsize=16, fontweight='bold')
    plt.xlabel('dataset', fontsize=14)
    plt.ylabel('psnr (dB) Distribution', fontsize=14)

    # Calculate average CR for each group to use as a label
    avg_cr_df = df.groupby(['dataset', 'transform'], as_index=False)['compression_ratio'].mean()

    # Iterate through the groups to add the average CR labels
    # This requires knowing the positions of the boxes on the x-axis
    n_datasets = len(df['dataset'].unique())
    n_transforms = len(df['transform'].unique())
    box_width = 0.8 / n_transforms # Standard dodge width is 0.8, divided by number of hues

    for i, dataset in enumerate(sorted(df['dataset'].unique())):
        for j, transform in enumerate(sorted(df['transform'].unique())):
            # Find the average CR for this specific group
            cr_value = avg_cr_df[
                (avg_cr_df['dataset'] == dataset) &
                (avg_cr_df['transform'] == transform)
            ]['compression_ratio'].iloc[0]

            # Calculate x position: i (dataset index) + (j - center_offset) * box_width
            center_offset = (n_transforms - 1) / 2
            x_pos = i + (j - center_offset) * box_width

            # Find the max psnr value of the data in this box to place the label just above
            y_max = df[
                (df['dataset'] == dataset) &
                (df['transform'] == transform)
            ]['psnr'].max()

            # Add the text label
            plt.text(
                x_pos,
                y_max + 1,  # Place slightly above the max value
                f"Avg CR: {cr_value:.2f}",
                ha='center',
                color='black',
                fontsize=9,
                weight='bold',
                bbox=dict(facecolor='white', alpha=0.7, edgecolor='none', pad=2)
            )

    plt.legend(title='transform', bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig('plot_3_psnr_box_with_cr_label.png')
    print("Saved plot_3_psnr_box_with_cr_label.png")

def main():
    """Main function to load data and generate plots."""
    # Attempt to load the user's data
    df = pd.read_csv(FILE_PATH)
    os.chdir("results")
    generate_plots(df)
    os.chdir("..")

if __name__ == "__main__":
    main()