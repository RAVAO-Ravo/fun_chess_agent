#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

rm -rf "${project_root}/build"
find "${project_root}/gui" -type d -name __pycache__ -prune -exec rm -rf {} +
find "${project_root}/gui" -type f -name '*.pyc' -delete

echo "Artefacts de compilation et caches Python supprimés."
