#!/bin/bash
set -e  # Exit if any command fails

echo "========== Building binaries =========="
make
echo "Build completed."

echo "========== Scraping Svenska Spel data =========="
python3 scrapeSvenskaSpel.py
echo "Scraping completed."

echo "========== Generating sample space =========="
python3 generateSampleSpace.py
echo "Sample space generation completed."

echo "========== Creating PoBin LUT =========="
# Check PoBinLUT.bin file size
POBIN_FILE="data/PoBinLUT.bin"
EXPECTED_SIZE=178564176

if [[ -f "$POBIN_FILE" ]]; then
    actual_size=$(stat -c%s "$POBIN_FILE")
    if [[ "$actual_size" -eq "$EXPECTED_SIZE" ]]; then
        echo "PoBin LUT already exists and is correct size ($EXPECTED_SIZE bytes). Skipping..."
    else
        echo "PoBin LUT size mismatch (found $actual_size bytes). Re-generating..."
        ./createPoBinLUT
        echo "PoBin LUT creation completed."
    fi
else
    echo "PoBin LUT file not found. Generating..."
    ./createPoBinLUT
    echo "PoBin LUT creation completed."
fi

echo "PoBin LUT creation completed."

echo "========== Running simulation =========="
mpirun -np 4 --bind-to none ./run_simulation
echo "Simulation completed."

echo "========== Pipeline finished successfully =========="
