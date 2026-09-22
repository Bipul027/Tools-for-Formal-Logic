import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("results.csv")
df = df.sort_values(by="Length")

plt.figure(figsize=(10, 6))
plt.plot(
    df["Length"],
    df["Custom_Time_us"] / 1000,
    marker="o",
    color="red",
    label="Custom CNF (ms)",
)
plt.plot(
    df["Length"], df["Z3_Time_us"] / 1000, marker="s", color="blue", label="Z3 SMT (ms)"
)

plt.yscale("log")
plt.title("Validity Checker Execution Time vs Formula Length")
plt.xlabel("Formula String Length (chars)")
plt.ylabel("Execution Time (milliseconds) - Log Scale")
plt.grid(True, which="both", ls="--", alpha=0.5)
plt.legend()
plt.tight_layout()
plt.savefig("benchmark_plot.png", dpi=300)
