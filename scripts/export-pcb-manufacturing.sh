#!/usr/bin/env bash
set -euo pipefail

if [[ $# != 1 ]]; then
    echo "Usage: $0 NEW_OUTPUT_DIRECTORY" >&2
    exit 2
fi
cli=${KICAD_CLI:-kicad-cli}
version=$("$cli" version)
[[ $version == 10.* ]] || { echo "KiCad 10 required, found $version" >&2; exit 2; }
project="$(cd -- "$(dirname -- "$0")/../hardware" && pwd)/fan-controller"
mkdir -- "$1"
out=$(cd -- "$1" && pwd)
"$cli" pcb export gerbers -l F.Cu,B.Cu,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,Edge.Cuts \
    --subtract-soldermask --check-zones -o "$out/" "$project.kicad_pcb"
"$cli" pcb export drill --format excellon --drill-origin absolute --excellon-units mm \
    --excellon-zeros-format decimal --excellon-oval-format route --excellon-separate-th \
    --generate-map --map-format svg -o "$out/" "$project.kicad_pcb"
