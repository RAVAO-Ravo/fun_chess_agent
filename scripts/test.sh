#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset="${1:-debug}"

cd "${project_root}"
python -m mypy
cmake --preset "${preset}"
cmake --build --preset "${preset}" --parallel
ctest --preset "${preset}"
