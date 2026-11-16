#!/bin/sh
# Everything that can be tested before the parts arrive. Run from the repo root:
#   sh tools/virtual_test.sh
# Needs python3 and gcc; the circuit simulations also need ngspice.
set -e
cd "$(dirname "$0")/.."
echo "== 1. Pin map rules =="
python3 tools/check_pins.py
echo
echo "== 2. Firmware logic on this computer =="
gcc -Wall -Wextra -Ifirmware/main firmware/main/logic.c firmware/test/test_logic.c -o "${TMPDIR:-/tmp}/pt_test_logic"
"${TMPDIR:-/tmp}/pt_test_logic"
echo
echo "== 3. Circuit simulations =="
if command -v ngspice >/dev/null; then
  python3 sim/run.py > sim/RESULTS.md && echo "wrote sim/RESULTS.md"
else
  echo "ngspice not installed: skipped (sim/RESULTS.md has the last results)"
fi
