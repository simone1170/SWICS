#!/usr/bin/env python3
"""Plot per-UE MCS over time from a SWICS rx-packet-trace.tsv.

The trace is produced by the mmWave PHY Rx instrumentation enabled in
scratch/thesis/testbed.cc (one row per received transport block):

    DL/UL  time  frame  subF  slot  1stSym  symbol#  cellId  rnti  ccId
    tbSize  mcs  rv  SINR(dB)  corrupt  TBler

Usage:
    ./venv/bin/python plot-mcs.py <path/to/rx-packet-trace.tsv> [--dir DL|UL|both]

Writes <trace_dir>/mcs-over-time.png next to the input and prints summary stats.
"""
import sys
import argparse
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def load(path):
    df = pd.read_csv(path, sep="\t")
    df.columns = [c.strip() for c in df.columns]
    # normalise the direction column name (header is "DL/UL")
    df = df.rename(columns={"DL/UL": "dir"})
    for c in ("time", "rnti", "mcs", "SINR(dB)", "corrupt", "TBler"):
        if c in df:
            df[c] = pd.to_numeric(df[c], errors="coerce")
    return df.dropna(subset=["time", "rnti", "mcs"])


def summarise(df):
    print(f"rows: {len(df)}  |  RNTIs: {sorted(df['rnti'].unique().tolist())}")
    print(f"time span: {df['time'].min():.4f}..{df['time'].max():.4f} s")
    corrupt = int(df.get("corrupt", pd.Series(dtype=float)).fillna(0).sum())
    print(f"overall MCS mean={df['mcs'].mean():.2f}  min={int(df['mcs'].min())}  "
          f"max={int(df['mcs'].max())}  corrupt(NACK) TBs={corrupt}")
    print("per-RNTI mean MCS:")
    for rnti, g in df.groupby("rnti"):
        c = int(g.get("corrupt", pd.Series(dtype=float)).fillna(0).sum())
        print(f"  rnti {int(rnti):>3}: mean={g['mcs'].mean():5.2f}  "
              f"n={len(g):>5}  corrupt={c}")


def plot(df, out, which, window=9, min_tb=0):
    if which != "both":
        df = df[df["dir"] == which]
    if min_tb and "tbSize" in df:
        df = df[df["tbSize"] >= min_tb]
    rntis = sorted(df["rnti"].unique())
    cmap = plt.get_cmap("tab10")
    fig, ax = plt.subplots(figsize=(11, 5))
    for i, rnti in enumerate(rntis):
        color = cmap(i % 10)
        g = df[df["rnti"] == rnti].sort_values("time")
        # faint raw transport-block samples ...
        ax.scatter(g["time"], g["mcs"], s=5, color=color, alpha=0.15)
        # ... with a bold rolling-median trend line on top
        trend = g["mcs"].rolling(window, center=True, min_periods=1).median()
        ax.plot(g["time"], trend, lw=1.6, color=color, label=f"RNTI {int(rnti)}")
        # mark corrupt (NACK) transport blocks
        if "corrupt" in g:
            bad = g[g["corrupt"] == 1]
            if len(bad):
                ax.scatter(bad["time"], bad["mcs"], marker="x", s=45, color="red", zorder=5)
    ax.set_xlabel("time (s)")
    ax.set_ylabel("MCS index")
    ax.set_title(f"Per-UE MCS over time ({which})  —  bold = rolling median, "
                 f"faint = raw TBs, red x = corrupt (NACK)")
    ax.set_ylim(-1, 29)
    ax.grid(True, alpha=0.3)
    ax.legend(ncol=2, fontsize=8, loc="lower right")
    fig.tight_layout()
    fig.savefig(out, dpi=150)
    print(f"wrote {out}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("trace")
    ap.add_argument("--dir", choices=["DL", "UL", "both"], default="DL")
    args = ap.parse_args()
    df = load(args.trace)
    summarise(df)
    out = args.trace.rsplit("/", 1)[0] + "/mcs-over-time.png"
    plot(df, out, args.dir)


if __name__ == "__main__":
    main()
