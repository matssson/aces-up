# Copyright (c) 2025 Matsson <contact@matsson.org>
# SPDX-License-Identifier: MIT

from pathlib import Path
import sys
import matplotlib.pyplot as plt
import matplotlib.ticker as mtick
import pandas as pd
from scipy.stats import beta

BASE_DIR = Path(__file__).resolve().parent
CSV_PATH = BASE_DIR / "data" / "score_counts.csv"
OUT_PATH = BASE_DIR / "data" / "score_distribution.png"
CONF = 99.99

fmt = f"    {{:<{9}}}: {{:>{8}.5f}}"

def print_clopper_pearson(x, n, conf):
  alpha = 1 - conf/100
  high = beta.ppf(1 - alpha/2, x + 1, n - x)
  low  = beta.ppf(alpha/2, x, n - x + 1)
  p_hat = x / n
  max_diff = max(high - p_hat, p_hat - low)

  print(f"{conf}% Clopper-Pearson CI")
  for label, value in [("lower", low),
                       ("upper", high),
                       ("max-diff", max_diff)]:
    print(fmt.format(label, value * 100), "%")

try:
  df = pd.read_csv(CSV_PATH)
except Exception as err:
  sys.exit(err)
df = df.sort_values("score")

total_runs = df["count"].sum()
solved_games = df.loc[df["score"] == 0, "count"].sum()

print(f"Total number of games: {total_runs}")
print(f"Number of solved games: {solved_games}")
print(f"Proportion solved: {100 * solved_games/total_runs} %")

median_score = df["score"].iloc[df["count"].cumsum().searchsorted([(total_runs + 1)//2, total_runs//2 + 1])].mean()
mean_score = (df["score"] * df["count"]).sum() / total_runs
s_sq = ((df["score"] - mean_score)**2 * df["count"]).sum()
var_sample = s_sq / (total_runs - 1)
std_sample = var_sample**0.5

for label, value in [("median", median_score),
                     ("mean", mean_score),
                     ("variance", var_sample),
                     ("std dev", std_sample)]:
  print(fmt.format(label, value))

print_clopper_pearson(solved_games, total_runs, CONF)

df["pct"] = df["count"] / total_runs
plt.figure(figsize=(10, 6))
plt.bar(df["score"], df["pct"], width=0.8)

ax = plt.gca()
ax.yaxis.set_major_locator(mtick.MultipleLocator(0.05))
ax.yaxis.set_minor_locator(mtick.MultipleLocator(0.01))
ax.yaxis.set_major_formatter(mtick.PercentFormatter(xmax=1))
ax.set_ylim(0, 0.25)
ax.xaxis.set_major_locator(mtick.MultipleLocator(4))
ax.xaxis.set_minor_locator(mtick.MultipleLocator(1))
ax.set_xlim(-1, 1 + df["score"].max())

plt.xlabel("Score")
plt.title(f"Distribution of scores (n = {total_runs:,})")
plt.tight_layout()

plt.savefig(OUT_PATH)
plt.show()
