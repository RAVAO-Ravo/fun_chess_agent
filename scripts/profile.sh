#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
run_id="$(date '+%Y%m%d-%H%M%S')"
output_dir="${project_root}/runs/profiles/${run_id}"

mkdir -p "${output_dir}"
cd "${project_root}"
cmake --preset profile
cmake --build --preset profile --target search_benchmark --parallel
(
    cd "${output_dir}"
    "${project_root}/build/profile/search_benchmark" \
        --positions "${project_root}/benchmarks/positions/search.tsv" \
        --mode classic \
        --depth "${1:-4}" \
        --output search_results.csv
    gprof "${project_root}/build/profile/search_benchmark" gmon.out > gprof.txt
)

echo "Profil enregistré dans ${output_dir}/gprof.txt"
