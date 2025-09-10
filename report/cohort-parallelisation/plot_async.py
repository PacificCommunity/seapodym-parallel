import pandas as pd
import matplotlib.pyplot as plt
import glob

# Load all CSV files
for filename in glob.glob("results_asyncN*.csv"):
    
    print(filename)
    df = pd.read_csv(filename)

    # Extract N from filename
    N = filename.split("N")[-1].split(".")[0]

    # # Plot execution times
    # plt.figure(figsize=(8,6))
    # for nm in sorted(df['nm'].unique()):
    #     subset = df[df['nm']==nm]
    #     plt.plot(subset['nd'], subset['time_blocking'], 'o-', label=f"Blocking nm={nm}")
    #     plt.plot(subset['nd'], subset['time_nonblocking'], 's--', label=f"Non-blocking nm={nm}")
    # plt.xlabel("Chunk size (nd)")
    # plt.ylabel("Execution time (s)")
    # plt.title(f"Blocking vs Non-blocking MPI_Get (N={N})")
    # plt.legend()
    # plt.grid(True)
    # plt.savefig(f"times_N{N}.png", dpi=150)
    # plt.close()

    # Plot speedup
    df['speedup'] = df['time_blocking'] / df['time_nonblocking']
    plt.figure(figsize=(8,6))
    for nm in sorted(df['nm'].unique()):
        subset = df[df['nm']==nm]
        plt.plot(subset['nd'], subset['speedup'], 'o-', label=f"nm={nm}")
    plt.axhline(1, color='k', linestyle='--')
    plt.xlabel("Chunk size (nd)")
    plt.ylabel("Speedup nonblocking/blocking")
    plt.title(f"Speedup of Non-blocking vs Blocking MPI_Get ({N} ranks)")
    plt.legend()
    #plt.grid(True)
    plt.savefig(f"speedup_async_N{N}.png", dpi=150)
    plt.show()
    plt.close()
