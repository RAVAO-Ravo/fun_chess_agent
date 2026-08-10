#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_model="${1:-}"
active_model="${project_root}/data/models/current.json"

if [[ -z "${source_model}" || ! -f "${source_model}" ]]; then
    echo "Usage: scripts/promote_model.sh runs/<experience>/best_model.json" >&2
    exit 2
fi

cp "${source_model}" "${active_model}"
echo "Modèle promu explicitement vers ${active_model}"
