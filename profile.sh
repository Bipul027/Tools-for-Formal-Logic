#!/usr/bin/env bash
# Build with profiling flags, print a per-stage timing table, record with perf,
# and produce flame graphs that show ONLY YOUR FUNCTIONS (std::/libc frames removed,
# their time charged to the user function that called them), plus reports.
#
# Usage:
#   ./profile.sh                          # generates a big formula automatically (see GEN_ARGS) and profiles it
#   ./profile.sh big.txt                  # profile your own input
#   GEN_ARGS="random --n 20000 --vars 500" ./profile.sh
#   CALLGRAPH=fp ./profile.sh deep.txt    # very deep recursion (--shape skewed): frame pointers
#   SKIP=cdcl ./profile.sh                # parse + Tseitin only (no solver)
#   NOBUILD=1 ./profile.sh big.txt        # reuse existing ./program (must be a PROFILE_MODE build)
#   KEEP_EXTRA='^std::sort' ./profile.sh  # also keep frames matching this regex
#   REPEATS=20 WIDTH=2000 FREQ=999 ./profile.sh big.txt   (REPEATS defaults to "auto": enough runs for ~3 s)
#
# Needs filter_user_frames.py next to this script.

set -euo pipefail

# ---- Path to your cloned https://github.com/brendangregg/FlameGraph ----
FLAMEGRAPH_DIR="../../../../../Projects/FlameGraph"
# --------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="profile_out"
INPUT="${1:-}"
GEN_ARGS="${GEN_ARGS:-random --n 50000 --vars 26 --atom-fmt letters}"
CALLGRAPH="${CALLGRAPH:-dwarf}"         # dwarf: correct stacks through libc/libstdc++ (needs shallow recursion)
                                        # fp:    survives deep recursion, but stacks through libc/libstdc++ can be garbage
DWARF_STACK="${DWARF_STACK:-65528}"     # bytes of stack copied per sample for dwarf (max 65528)
if [ "$CALLGRAPH" = "dwarf" ]; then FREQ="${FREQ:-499}"; else FREQ="${FREQ:-1999}"; fi
REPEATS="${REPEATS:-auto}"
WIDTH="${WIDTH:-1500}"                  # keep this near your screen width; zoom by clicking boxes
MINWIDTH="${MINWIDTH:-0.2}"
FONTSIZE="${FONTSIZE:-11}"
NOBUILD="${NOBUILD:-0}"
KEEP_EXTRA="${KEEP_EXTRA:-}"
export SKIP="${SKIP:-}"
BIN="./program"
COMM="$(basename "$BIN")"               # only samples from this process name are kept (drops bash/make noise)

# Functions to build a dedicated flame graph for (stacks that contain the name).
STAGES=(buildParseTree evaluateFormula TseitinEncodedCNF CDCLCNF)

# ---- Sanity checks ----
command -v perf >/dev/null || { echo "perf not found (sudo apt install linux-tools-common linux-tools-generic)"; exit 1; }
command -v nm >/dev/null && command -v c++filt >/dev/null || { echo "nm / c++filt not found (sudo apt install binutils)"; exit 1; }
command -v python3 >/dev/null || { echo "python3 not found"; exit 1; }
[ -f "$SCRIPT_DIR/filter_user_frames.py" ] || { echo "filter_user_frames.py must be next to profile.sh"; exit 1; }
for f in flamegraph.pl stackcollapse-perf.pl stackcollapse-recursive.pl; do
  [ -f "$FLAMEGRAPH_DIR/$f" ] || { echo "FLAMEGRAPH_DIR='$FLAMEGRAPH_DIR' is wrong ($f not found). Run this script from the project folder or use an absolute path."; exit 1; }
done

PARANOID=$(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo 2)
if [ "$PARANOID" -gt 1 ]; then
  echo "perf_event_paranoid=$PARANOID is too restrictive. Run:"
  echo "  sudo sysctl kernel.perf_event_paranoid=1"
  exit 1
fi

SYS_MAXSTACK=$(cat /proc/sys/kernel/perf_event_max_stack 2>/dev/null || echo 127)
if [ "$SYS_MAXSTACK" -lt 1024 ]; then
  echo "WARNING: kernel.perf_event_max_stack=$SYS_MAXSTACK. Stacks deeper than that are cut off,"
  echo "         which breaks flame graphs of recursive code. Run:"
  echo "           sudo sysctl kernel.perf_event_max_stack=4096"
  echo
fi

mkdir -p "$OUT_DIR"

# ---- Input ----
if [ -z "$INPUT" ]; then
  [ -f gen_formula.py ] || { echo "No input given and gen_formula.py not found in the current folder."; exit 1; }
  INPUT="$OUT_DIR/generated.txt"
  echo ">> No input file given; generating one: gen_formula.py $GEN_ARGS"
  python3 gen_formula.py $GEN_ARGS > "$INPUT"
fi
[ -f "$INPUT" ] || { echo "Input file '$INPUT' not found"; exit 1; }

# ---- Build ----
# Overriding CXXFLAGS on the command line replaces the Makefile's value,
# so the include paths must be repeated here.
if [ "$NOBUILD" != "1" ]; then
  PROFILE_FLAGS="-Wall -Wextra -std=c++17 -I./wff -I./evaluator \
-O2 -g -fno-omit-frame-pointer -fno-inline-functions -DPROFILE_MODE"
  echo ">> Building with: $PROFILE_FLAGS"
  make clean
  make -j"$(nproc)" CXXFLAGS="$PROFILE_FLAGS"
fi

# ---- Timing pass (no profiler attached) ----
echo ">> Timing pass: which stage is slow? (input: $(wc -c < "$INPUT") bytes)"
T0=$(date +%s.%N)
( ulimit -s unlimited 2>/dev/null || true; "$BIN" < "$INPUT" > /dev/null )
T1=$(date +%s.%N)
RUNTIME=$(awk -v a="$T0" -v b="$T1" 'BEGIN { printf "%.3f", b - a }')
echo "   one run takes ${RUNTIME} s"

# Enough repeated runs to collect samples for roughly 3 seconds of CPU time.
if [ "$REPEATS" = "auto" ]; then
  REPEATS=$(awk -v t="$RUNTIME" 'BEGIN { if (t < 0.001) t = 0.001; r = int(3 / t) + 1; if (r > 300) r = 300; print r }')
fi

# ---- Record ----
if [ "$CALLGRAPH" = "dwarf" ]; then
  CG_OPT="--call-graph dwarf,$DWARF_STACK"
else
  CG_OPT="--call-graph fp"
fi
PERF_CMD=(perf record -F "$FREQ" $CG_OPT -o "$OUT_DIR/perf.data")

echo ">> Recording ($CALLGRAPH, ${FREQ} Hz, ${REPEATS} run(s))"
if [ "$REPEATS" -gt 1 ]; then
  RUN_CMD="for i in \$(seq $REPEATS); do $BIN < '$INPUT' > /dev/null 2>&1; done"
  ( ulimit -s unlimited 2>/dev/null || true; "${PERF_CMD[@]}" -- bash -c "$RUN_CMD" )
else
  # perf runs the program directly (no bash wrapper), with the input redirected to its stdin
  ( ulimit -s unlimited 2>/dev/null || true; "${PERF_CMD[@]}" -- "$BIN" < "$INPUT" > /dev/null )
fi

# ---- Fold stacks ----
echo ">> Folding stacks"
RAW="$OUT_DIR/out.raw.folded"          # every frame, recursion not merged
FOLDED="$OUT_DIR/out.folded"           # every frame, recursion merged
USERFOLDED="$OUT_DIR/out.user.folded"  # only your functions, recursion merged

perf script -i "$OUT_DIR/perf.data" --comms "$COMM" 2>/dev/null \
  | "$FLAMEGRAPH_DIR/stackcollapse-perf.pl" 2>/dev/null > "$RAW"

if [ ! -s "$RAW" ]; then
  echo "No samples for '$COMM' were collected. Use a bigger input (GEN_ARGS=\"random --n 200000 ...\")."
  exit 1
fi
SAMPLES=$(awk '{ s += $NF } END { print s + 0 }' "$RAW")
echo "   samples: $SAMPLES"
if [ "$SAMPLES" -lt 1000 ]; then
  echo "WARNING: only $SAMPLES samples. The graph will be mostly noise. Use a bigger input,"
  echo "         or raise FREQ / REPEATS."
fi

"$FLAMEGRAPH_DIR/stackcollapse-recursive.pl" "$RAW" > "$FOLDED"

# Which symbols are yours? (anything in the binary whose source file is not under /usr, /lib, /opt)
{ nm -l --defined-only "$BIN" | c++filt -p; } > "$OUT_DIR/user_symbols.txt" 2>/dev/null || true

FILTER_ARGS=(--symbols "$OUT_DIR/user_symbols.txt" --summary "$OUT_DIR/report_user_functions.txt")
[ -n "$KEEP_EXTRA" ] && FILTER_ARGS+=(--extra "$KEEP_EXTRA")
python3 "$SCRIPT_DIR/filter_user_frames.py" "${FILTER_ARGS[@]}" < "$RAW" \
  | "$FLAMEGRAPH_DIR/stackcollapse-recursive.pl" /dev/stdin > "$USERFOLDED"

if [ ! -s "$USERFOLDED" ]; then
  echo "No frames from your own functions were found. Is ./program built with -g (profile.sh does this)?"
  exit 1
fi

# ---- Flame graphs ----
echo ">> Generating flame graphs"
FG_OPTS=(--width "$WIDTH" --minwidth "$MINWIDTH" --fontsize "$FONTSIZE")

"$FLAMEGRAPH_DIR/flamegraph.pl" "${FG_OPTS[@]}" --title "Your functions only: $INPUT" \
  "$USERFOLDED" > "$OUT_DIR/flamegraph_user.svg"

"$FLAMEGRAPH_DIR/flamegraph.pl" "${FG_OPTS[@]}" --reverse --inverted \
  --title "Your functions, leaf-first (where time lands): $INPUT" \
  "$USERFOLDED" > "$OUT_DIR/flamegraph_user_reversed.svg"

for st in "${STAGES[@]}"; do
  if grep -qF "$st" "$USERFOLDED"; then
    grep -F "$st" "$USERFOLDED" \
      | "$FLAMEGRAPH_DIR/flamegraph.pl" "${FG_OPTS[@]}" --title "Your functions, only stacks containing: $st" \
      > "$OUT_DIR/stage_${st}.svg"
  fi
done

"$FLAMEGRAPH_DIR/flamegraph.pl" "${FG_OPTS[@]}" --title "Everything incl. std:: and libc: $INPUT" \
  "$FOLDED" > "$OUT_DIR/flamegraph_all.svg"

# ---- Text reports ----
perf report -i "$OUT_DIR/perf.data" --comms "$COMM" --no-children -g none --percent-limit 0.5 --stdio 2>/dev/null \
  > "$OUT_DIR/report_self.txt" || true

echo
echo ">> Your functions ranked (inclusive = including everything they call):"
head -22 "$OUT_DIR/report_user_functions.txt"

echo
echo ">> Done. Output in $OUT_DIR/:"
echo "   flamegraph_user.svg           START HERE: only your functions, std::/libc time charged to the caller"
echo "   flamegraph_user_reversed.svg  which of your functions the time lands in directly"
echo "   stage_<name>.svg              your functions, one stage at a time"
echo "   report_user_functions.txt     ranking table of your functions"
echo "   flamegraph_all.svg            everything, unfiltered (open in a browser, not an editor preview)"
echo "   report_self.txt               raw perf self-time table (incl. std::)"
echo "Tip: 'program' is now the profiling build; run 'make clean && make' to restore -O3."