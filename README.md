# JAIR article plot data and generation

This branch contains the data used to generate the plots in the JAIR article **Invariant Graph Propagation in Constraint-Based Local Search**.

The two .JSON files used for generating the plots are located in the `benchmark-json` directory.

The data files were collected by running the benchmarks of the Atlantis solver using the develop branch of this repository. 

The timestamp in the filename of each .JSON file correspond to when the benchmarks were run. 

To generate the plots, run the python3 program 
```bash
python3 plot-formatter.py --input benchmark.json/PLOT
``` 
where `PLOT` is the .JSON file to be plotted.