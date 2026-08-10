#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
run_id="$(date '+%Y%m%d-%H%M%S')"
output_dir="${project_root}/runs/benchmarks/${run_id}"
latest_output="${project_root}/benchmarks/results/latest.csv"
positions_path="${2:-${project_root}/benchmarks/positions/search.tsv}"

if [[ "${positions_path}" != /* ]]; then
    positions_path="${PWD}/${positions_path}"
fi

mkdir -p "${output_dir}"
cd "${project_root}"
cmake --preset benchmark
cmake --build --preset benchmark --target search_benchmark --parallel
"${project_root}/build/benchmark/search_benchmark" \
    --positions "${positions_path}" \
    --params "${project_root}/data/models/current.json" \
    --mode all \
    --depth "${1:-5}" \
    --output "${output_dir}/search_results.csv"

cp "${output_dir}/search_results.csv" "${latest_output}"
echo "Benchmark écrit dans ${output_dir}/search_results.csv"
echo "Dernière mesure copiée dans ${latest_output}"
