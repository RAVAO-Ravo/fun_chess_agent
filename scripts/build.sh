#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
preset="${1:-release}"

cd "${project_root}"
cmake --preset "${preset}"
cmake --build --preset "${preset}" --parallel
