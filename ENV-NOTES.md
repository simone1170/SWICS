# SWICS build environment notes

Captured 2026-06-09T08:52:39Z on the WSL2 bootstrap machine. Use this to
reproduce the build on the dedicated Linux box.

## Toolchain (WSL2)
- OS: Ubuntu 24.04.3 LTS (under WSL2)
- ns-3: 3.38.rc1
- Compiler: gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- cmake version 3.28.3

## How to build (any machine)
```sh
./build-swics.sh
```
This restricts the build to the modules the scratch/thesis scenario needs and adds
`-include cstdint` for GCC-13 compatibility. See the script header for rationale.

## Why not the stock utils/install.sh?
`utils/install.sh` builds **all** ns-3 modules. On Ubuntu 24.04 / GCC 13 the legacy
`wifi` module fails to compile (`std::uint8_t`/`std::begin` without `<cstdint>`/`<iterator>`).
SWICS does not use wifi/mesh/wave/wimax/uan/lr-wpan, so `build-swics.sh` excludes them.
On an older toolchain (e.g. Ubuntu 22.04 / GCC 11) `build-swics.sh` still works.

## Python venv (schedule runner)
- Location: utils/venv (gitignored)
- Packages: contourpy==1.3.3 cycler==0.12.1 fonttools==4.63.0 kiwisolver==1.5.0 matplotlib==3.10.9 numpy==2.4.6 packaging==26.2 pandas==3.0.3 pillow==12.2.0 pyparsing==3.3.2 python-dateutil==2.9.0.post0 six==1.17.0 

## Run
```sh
cd utils
./venv/bin/python schedule-simulation.py 1s-all-attacks     # smoke test (~seconds)
./venv/bin/python schedule-simulation.py 1200s-jamm-powers  # full jamming-power baseline (long)
```
Output lands in `dataset/new-sim-run-<timestamp>/` (gitignored): per-attack
`physical-state.csv` plus `total_comparison_*.png` synopsis plots.
