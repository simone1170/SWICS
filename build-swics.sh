#!/usr/bin/env bash
# Portable SWICS build wrapper.
# Reproduces the exact configuration used on the WSL2/Ubuntu-24.04 + GCC-13 setup.
# Works on older toolchains too (the -include cstdint flag is harmless there).
#
# Usage:  ./build-swics.sh
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
SIM="$HERE/simulation"

# Only the modules the scratch/thesis scenario actually needs. This deliberately
# EXCLUDES the legacy wifi/mesh/wave/wimax/uan/lr-wpan modules, which fail to
# compile under GCC 13 (missing transitive <cstdint>/<iterator> includes) and are
# not used by SWICS.
MODS="mmwave,jamming,aodv,applications,bridge,buildings,csma,internet,mobility,point-to-point,spectrum"

cd "$SIM"

# -include cstdint: global safety net for GCC-13 "X is not a member of std" errors
export CXXFLAGS="${CXXFLAGS:-} -include cstdint"

echo ">> configuring (modules: $MODS)"
./ns3 configure --enable-modules="$MODS"

echo ">> building"
./ns3 build

echo ">> binary:"
ls -la "$SIM/build/test_testbed"

# Python venv for the schedule runner (pandas/matplotlib)
cd "$HERE/utils"
if [ ! -d venv ]; then
  python3 -m venv venv
  ./venv/bin/pip install --quiet --upgrade pip
  ./venv/bin/pip install --quiet pandas matplotlib
fi
echo ">> done. Run e.g.:  cd utils && ./venv/bin/python schedule-simulation.py 1s-all-attacks"
