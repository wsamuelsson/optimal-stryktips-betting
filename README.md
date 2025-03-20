# Optimal betting in Stryktipset

## Paper
This implementation builds on the Clair, Letscher paper [1]. What is new in this implementation is the use of: 
            -Efficient computation of conditional payoff using the binomial-poission distribution
            -Implementated realistic payoff
            -Inferring betting pool size via the Pareto distribution
            -Parallell implementation of a Simmulated annealing solver
[1] Bryan Clair and David Letscher. "Optimal Strategies for Sports Betting Pools." *Operations Research*, vol. 55, no. 6, 2007, pp. 1163–1177. INFORMS. DOI: [10.1287/opre.1070.0424](https://doi.org/10.1287/opre.1070.0424).

## Dependencies

This project requires:

- [FFTW3](http://www.fftw.org/)
  - On Debian/Ubuntu: `sudo apt install libfftw3-dev`
- MPI compiler (e.g., mpicc, mpirun)
  - On Debian/Ubuntu: `sudo apt install mpich`

You can check dependencies by running:

```bash
./check_dependencies.sh
