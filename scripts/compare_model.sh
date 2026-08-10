#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
candidate="${1:-}"
reference="${2:-${project_root}/data/models/current.json}"

if [[ -z "${candidate}" || ! -f "${candidate}" || ! -f "${reference}" ]]; then
    echo "Usage: scripts/compare_model.sh <candidate.json> [reference.json]" >&2
    exit 2
fi

cd "${project_root}"
cmake --preset release
cmake --build --preset release --target compare_models --parallel
"${project_root}/build/release/compare_models" \
    --candidate "${candidate}" \
    --reference "${reference}" \
    --corpus "${project_root}/data/positions/holdout_corpus.tsv" \
    --max-halfmoves 300
