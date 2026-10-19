#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

python3 "$DIR/build_dwg_sources.py"
python3 "$DIR/build_esq1_sources.py"
python3 "$DIR/build_drum_sources.py"

echo "Wavetable sources (DWG, ESQ-1, and Drums) successfully built."
