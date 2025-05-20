# JAIR Article Results Data and Plot Generation program

This branch contains the data used to generate the plots in the JAIR article **Invariant Graph Propagation in Constraint-Based Local Search**.

## Repository Contents

The two `.json` data files used for generating the plots are located in the `benchmark-json` directory.These data files were the results from running the benchmarks of the Atlantis solver using the develop branch of this repository. The timestamp in the filename of each `.json` data file correspond to when the benchmarks were run. 

The `plot-formatter.py` python code file contains all source code and is the plot generation program.

The `plot-formatter.json` file contains all (default) settings to the `plot-formatter.py` program. 
When generating the plots and for each run, the program uses the mean number of probes per second for the run.

# Generating Plots
In order to run the plot generation program, the `matplotlib` python 3 package must be installed. Instructions for installing this package are available at: https://matplotlib.org/stable/install/index.html

To generate the plots, run the plot generation program as follows:
```bash
python3 plot-formatter.py --input benchmark-json/DATA_FILE
``` 
where `DATA_FILE` is the `.json` data file to be plotted.