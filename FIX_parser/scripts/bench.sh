#!/usr/bin/env bash
# Benchmarks the FIX_parser in pure measurement mode against a replayed PCAP.
#
# Usage:
#   scripts/bench.sh [pcap] [iterations]
#     pcap        default: tests/fix.pcap
#     iterations  default: 10
#
# Reports min / median / max parse time across runs plus CPU governor and
# current frequency, since results scale directly with clock speed. For
# reproducible numbers run with the CPU pinned to "performance"
# (e.g. sudo cpupower frequency-set -g performance) and on a fixed core
# (taskset -c 2 scripts/bench.sh).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/build/FIX_parser"
REPLAY="$ROOT/src/replay.py"
PCAP="${1:-$ROOT/tests/fix.pcap}"
ITERATIONS="${2:-10}"
HOST=127.0.0.1
PORT=8080

if [ ! -x "$BIN" ]; then
    echo "No build found; building..."
    "$ROOT/scripts/build.sh"
fi
if [ ! -f "$PCAP" ]; then
    echo "error: pcap not found: $PCAP" >&2
    exit 1
fi

declare -a TIMES_NS=()
MSG_COUNT=0

for i in $(seq 1 "$ITERATIONS"); do
    setsid nohup python3 "$REPLAY" "$PCAP" --host "$HOST" --port "$PORT" \
        >/tmp/fix_bench.replay 2>&1 </dev/null &
    REPLAY_BG=$!
    for _ in $(seq 1 50); do
        ss -tln 2>/dev/null | grep -q ":$PORT " && break
        sleep 0.2
    done

    "$BIN" "$HOST" "$PORT" 2>/tmp/fix_bench.stats
    kill "$REPLAY_BG" 2>/dev/null || true
    wait "$REPLAY_BG" 2>/dev/null || true

    ns=$(grep "Total parse time" /tmp/fix_bench.stats | grep -oE '[0-9]+' | head -1)
    msgs=$(grep "Messages parsed" /tmp/fix_bench.stats | grep -oE '[0-9]+' | head -1)
    [ -z "${ns:-}" ] && { echo "warning: no timing captured in run $i" >&2; continue; }
    MSG_COUNT="$msgs"
    TIMES_NS+=("$ns")
done

if [ "${#TIMES_NS[@]}" -eq 0 ]; then
    echo "error: no valid measurements" >&2
    exit 1
fi

# sort numerically
mapfile -t SORTED < <(printf '%s\n' "${TIMES_NS[@]}" | sort -n)
N=${#SORTED[@]}
MIN=${SORTED[0]}
MAX=${SORTED[$((N - 1))]}
MID=$((N / 2))
MED=${SORTED[$MID]}

us() { python3 -c "print(f'{$1/1000.0:.1f}')"; }

GOV=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo "n/a")
FREQ=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq 2>/dev/null || echo "n/a")

echo "pcap:       $PCAP"
echo "messages:   $MSG_COUNT per run ($N runs)"
echo "parse time:  min $(us "$MIN") us | median $(us "$MED") us | max $(us "$MAX") us"
echo "CPU:        governor=$GOV  current=$((FREQ / 1000)) MHz"
echo
echo "note: results scale with CPU clock; pin governor=performance for stable numbers."