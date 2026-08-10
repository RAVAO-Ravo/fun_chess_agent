#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "${project_root}"
cmake --preset release
cmake --build --preset release --target build_opening_book --parallel
"${project_root}/build/release/build_opening_book" \
    --input-dir "${project_root}/data/openings/source" \
    --output "${project_root}/data/openings/generated/book.txt" \
    --training-output "${project_root}/data/openings/generated/training_positions.fen"
