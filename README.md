[![INFORMS Journal on Computing Logo](https://INFORMSJoC.github.io/logos/INFORMS_Journal_on_Computing_Header.jpg)](https://pubsonline.informs.org/journal/ijoc)

# Branch-and-Price for the Set-Union Bin Packing Problem

This archive is distributed in association with the [INFORMS Journal on
Computing](https://pubsonline.informs.org/journal/ijoc) under the [MIT License](LICENSE.txt).

The software and data in this repository are a snapshot of the software and data
that were used in the research reported on in the paper 
[Branch-and-Price for the Set-Union Bin Packing Problem](https://doi.org/10.1287/ijoc.2024.0791) by Julia Wahlen and Timo Gschwind. 


## Cite

To cite the contents of this repository, please cite both the paper and this repo, using their respective DOIs.

https://doi.org/10.1287/ijoc.2024.0791

https://doi.org/10.1287/ijoc.2024.0791.cd

Below is the BibTex for citing this snapshot of the repository.

```
@misc{WahlenGschwind2025,
  author =        {Julia Wahlen and Timo Gschwind},
  publisher =     {INFORMS Journal on Computing},
  title =         {Branch-and-Price for the Set-Union Bin Packing Problem},
  year =          {2025},
  doi =           {10.1287/ijoc.2024.0791.cd},
  url =           {https://github.com/INFORMSJoC/2024.0791},
  note =          {Available for download at https://github.com/INFORMSJoC/2024.0791},
}  
```

## Description

This repository provides part of the implementation of our branch-and-price algorithm for solving the **Set-Union Bin Packing Problem (SUBP)**.
More precisely, it provides all problem-specific parts of our algorithm - most notably various algorithms for solving the column-generation pricing problem.
It does **not** provide the general branch-and-price infrastructure, because we do not have the full rights to share this part of the software.

For this project, we integrated our code with a branch-and-price framework jointly developed and maintained by several research groups. 
Because this code is not open source, we are unable to make the full standalone codebase public.

### Code Usage

The provided code is not intended to be used standalone, but to be integrated into a general branch-and-price framework (or a comparable method).
To this end, the pricing solver class provides three main functions to (i) solve a specific pricing problem (`Pricing`), (ii) update the pricer according to new dual prices (`UpdateDuals`), and (iii) update the pricer according to the current branching constraints (`UpdateBranchingConstraints`).


## Code Overview

All algorithms are coded in C++. Several components rely on the commercial solver CPLEX (version 20.10 was used in our experiments). 
The source files and their functionalities are listed below (alphabetically):

### Main Files

| File | Description |
|------|-------------|
| `mip_arf.h/.cpp` | Implementation of the ARF model, as proposed by Jans and Desrosiers (2013). |
| `mip_lex_i.h/.cpp` | Implementation of the SF-LEX-I model, introduced by Jans and Desrosiers (2013). |   
| `subp_instance.h/.cpp` | Parser and data structures for reading instance data. |
| `subp_pricing_solver.h/.cpp` | Pricing problem solver, consisting of:<br> - pricing variant selector (`Pricing`), <br> - branching constraint handler (`UpdateBranchingConstraints`), and <br> - dual price updater (`UpdateDuals`). |
| `subp_pricingMIP.h/.cpp` | Mixed-Integer Programming (MIP) formulation for the pricing problem. |
| `subp_relaxSUKP.h/.cpp` | Implementation of a completion bound based on a relaxed SUKP formulation (denoted as \$B_4\$ in the paper). |
| `subp_settings.h` | Configuration file for specifying problem-related settings. |
| `subp_spprc_labeling.h/.cpp` | Labeling algorithm for solving the pricing problem. |

### Auxiliary Files

| File | Description |
|------|-------------|
| `bitfield.h` | Definition of the custom data type 'bitfield'. |
| `json.hpp` | Header for JSON input/output operations. |
| `quick_union.h` | Quick union data structure implementation. |
| `random.h/.cpp` | Random number generation utilities. |
| `unordered_dense.h` | Support for hash maps. |



## Benchmark Instances

The computational study focuses on two benchmark sets:

1. **Unit-weight instances**: Based on the extensive benchmark by Grange et al. (2018), referred to as *pagination*.
2. **General-weight instances**: Large-scale instances derived from the SUKP benchmark by He et al. (2018), referred to as *general*.

All benchmark instances are included in the `data` folder.


## Computational Results

The results of our computational study are provided in the `results` folder.

#### General Benchmark

For each instance, the following information is reported in the file `detailedResults_general.csv`:

- `ourBKS`: The best known solution value obtained by our algorithm.
- `treeLB`: The best lower bound found during the search tree exploration.
- `time_seconds`: The total computation time in seconds.
- `numNodesSolved`: The number of nodes explored in the branch-and-bound tree.
- `non-IRUP`: Indicator for integer round-up property (1 if the instance does *not* satisfy the IRUP, 0 if it does, left blank if not yet proven).

#### Pagination Benchmark

The file `detailedResults_pagination.csv` contains the same columns as listed above for the general benchmark. In addition, it includes the following:

- `totalBKS`: The best known solution value aggregated across all methods.
- `GrangeBKS`: The best known solution value reported by Grange et al. (2018).


## References

- Grange, A., Kacem, I., and Martin, S. (2018). Algorithms for the bin packing problem with overlapping items. *Computers & Industrial Engineering*, 115, 331–341.
- He, Y., Xie, H., Wong, T.-L., and Wang, X. (2018). A novel binary artificial bee colony algorithm for the set-union knapsack problem. *Future Generation Computer Systems*, 78, 77–86.
- Jans, R. and Desrosiers, J. (2013). Efficient symmetry breaking formulations for the job grouping problem. *Computers & Operations Research*, 40(4), 1132–1142.
