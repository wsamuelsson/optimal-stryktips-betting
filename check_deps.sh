#!/bin/bash
set -e

echo "========== Checking dependencies =========="

# Check for FFTW3
if ldconfig -p | grep libfftw3 > /dev/null 2>&1; then
    echo "FFTW3 library found."
else
    echo "FFTW3 not found! Please install libfftw3 (e.g., 'sudo apt install libfftw3-dev')."
    exit 1
fi

# Check for MPI compiler
if command -v mpicc > /dev/null 2>&1; then
    echo "MPI compiler (mpicc) found."
else
    echo "MPI compiler not found! Please install MPI (e.g., 'sudo apt install mpich')."
    exit 1
fi

#Check python dependencies
python3 check_deps.py

echo "All dependencies satisfied!"
