#!/usr/bin/env python3
"""Plot the gNB COMMANDED downlink MCS over time (from dl-phy-trace.tsv) for the
target UE, comparing Baseline / Jammer / ReactiveJammer. This is the true
link-adaptation signal: it shows what MCS the scheduler chose per allocation,
independent of whether the UE decoded the TB (unlike rx-packet-trace.tsv, which
only logs surviving TBs and is biased high under jamming).

Usage: plot-cmd-mcs.py <run_dir>/5g  [target_rnti]
"""
import sys, os
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

run = sys.argv[1]
target_rnti = int(sys.argv[2]) if len(sys.argv) > 2 else None
attacks = ["Baseline", "Jammer", "ReactiveJammer"]
WIN = (2.0, 8.0)  # attack window

fig, ax = plt.subplots(figsize=(11, 5))
summary = []
for a in attacks:
    f = os.path.join(run, a, "dl-phy-trace.tsv")
    if not os.path.isfile(f):
        print(f"[skip] {f} missing")
        continue
    df = pd.read_csv(f, sep="\t")
    # type column: 0 == DL data (CTRL/DATA enum); keep DL data TBs, new transmissions only
    df = df[df["retxNum"] == 0]
    if target_rnti is not None:
        df = df[df["rnti"] == target_rnti]
    else:
        # pick the busiest RNTI (the data-bearing UE)
        if not df.empty:
            target_rnti = df["rnti"].value_counts().idxmax()
            df = df[df["rnti"] == target_rnti]
    df = df.sort_values("time")
    win = df[(df["time"] >= WIN[0]) & (df["time"] <= WIN[1])]
    mean_mcs = win["mcs"].mean() if not win.empty else float("nan")
    summary.append((a, target_rnti, len(df), mean_mcs))
    # rolling median trend over time
    ax.plot(df["time"], df["mcs"], alpha=0.12, lw=0.6,
            label=f"_{a} raw")
    trend = df.set_index("time")["mcs"].rolling(200, min_periods=1).median()
    ax.plot(trend.index, trend.values, lw=2.0, label=a)

ax.axvspan(WIN[0], WIN[1], color="red", alpha=0.06, label="attack window")
ax.set_xlabel("time (s)")
ax.set_ylabel("commanded DL MCS (gNB AMC)")
ax.set_title(f"Commanded DL MCS over time (target RNTI={target_rnti})")
ax.legend(loc="upper right", fontsize=8)
ax.grid(True, alpha=0.3)
out = os.path.join(run, "commanded-mcs-over-time.png")
plt.tight_layout()
plt.savefig(out, dpi=150)
print(f"wrote {out}")
print("\nattack          rnti  rows   meanCmdMCS(2-8s)")
for a, r, n, m in summary:
    print(f"{a:<15} {r}    {n:<6} {m:.2f}")
