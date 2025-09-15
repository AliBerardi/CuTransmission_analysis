#!/usr/bin/env bash
set -Eeuo pipefail

# -------------------------------
# Check for output directory
# -------------------------------
OUTPUT_DIR="OUTPUT"
mkdir -p "$OUTPUT_DIR" || { echo -e "${RED}Error:${NC} could not create '$OUTPUT_DIR'."; exit 1; }

# -------------------------------
# Meta / version / Author
# -------------------------------
VERSION="1.1.0"
AUTHOR="Alice Berardi"

# -------------------------------
# Config (fixed names)
# -------------------------------
SCRIPT1="Efficiency_plot_runlists.py"
SCRIPT2="EFFICIENCY_AllRuns.py"
SCRIPT3="STABILITY_allrunsMaxBin.py"
SCRIPT4="Histograms_AllRuns.py"
SCRIPT5="gflash_calibration.py"
ROOTSCRIPT="Transmission_ratio_final.C"
ROOT_FIXED_ARG="input_files/Transmission_ratio_final.cmnd"

# -------------------------------
# Colors
# -------------------------------
if [[ -t 1 && -z "${NO_COLOR:-}" ]]; then
  BOLD='\033[1m'
  DIM='\033[2m'
  RED='\033[0;31m'
  GREEN='\033[0;32m'
  YELLOW='\033[0;33m'
  BLUE='\033[0;34m'
  NC='\033[0m'
else
  BOLD=''; DIM=''; RED=''; GREEN=''; YELLOW=''; BLUE=''; NC=''
fi

# -------------------------------
# Portable high-resolution time (Linux + macOS)
# -------------------------------
now_ns() {
  # Try GNU date first
  local t
  t=$(date +%s%N 2>/dev/null || true)
  if [[ "$t" =~ N$ || -z "$t" ]]; then
    # macOS/BSD fallback via Python
    python3 - <<'PY'
import time
print(int(time.time()*1e9))
PY
  else
    printf '%s\n' "$t"
  fi
}

# -------------------------------
# Usage
# -------------------------------
print_usage() {
  local me="${0##*/}"
cat <<USAGE
Usage:
  $me [FLAGS]

Flags (combine as you like):
  -p, --plot           Run Efficiency_plot_runlists.py
  -e, --eff-all        Run EFFICIENCY_AllRuns.py
  -s, --stability      Run STABILITY_allrunsMaxBin.py
  -c, --histograms     Run Histograms_AllRuns.py
  -g, --gflash         Run gflash_calibration.py
  -t, --transmission   Run Transmission_ratio_final.C
  -a, --all            Run all of the above (Python + ROOT)
  -l, --log <FILE>     Save combined output of selected scripts to FILE
  -v, --version        Show version and author
  -h, --help           Show this help and exit

Examples:
  $me --plot --log output.log
  $me -p -s
  $me --all

Author: ${AUTHOR}
Version: ${VERSION}
USAGE
}

# -------------------------------
# Parse flags
# -------------------------------
QUEUE=()
LOGFILE=""

add_py()   { local f="$1"; QUEUE+=("python3 \"$f\" :: $f"); }
add_root() { local f="$1"; QUEUE+=("root -l -b -q '$f(\"$ROOT_FIXED_ARG\")' :: $f"); }

if [[ $# -eq 0 ]]; then
  print_usage
  exit 1
fi

while [[ $# -gt 0 ]]; do
  case "$1" in
    -v|--version)
      echo "${0##*/} ${VERSION} — Author: ${AUTHOR}"
      exit 0
      ;;
    -p|--plot)        add_py "$SCRIPT1" ;;
    -e|--eff-all)     add_py "$SCRIPT2" ;;
    -s|--stability)   add_py "$SCRIPT3" ;;
    -c| --histograms) add_py "$SCRIPT4" ;;
    -g| --gflash)     add_py "$SCRIPT5" ;;
    -t|--transmission)      add_root "$ROOTSCRIPT" ;;
    -a|--all)
      add_py "$SCRIPT1"
      add_py "$SCRIPT2"
      add_py "$SCRIPT3"
      add_py "$SCRIPT4"
      add_py "$SCRIPT5"
      add_root "$ROOTSCRIPT"
      ;;
    -l|--log)
      if [[ -z "${2-}" || "$2" == -* ]]; then
        echo -e "${RED}Error:${NC} --log requires a file name." >&2
        exit 1
      fi
      LOGFILE="$2"
      shift
      ;;
    -h|--help)
      print_usage
      exit 0
      ;;
    *)
      echo -e "${RED}Unknown option:${NC} $1" >&2
      print_usage
      exit 1
      ;;
  esac
  shift
done

# -------------------------------
# Deduplicate queue (portable, works with set -u)
# -------------------------------
declare -a deduped=()
for item in "${QUEUE[@]}"; do
  skip=0
  if ((${#deduped[@]})); then
    for d in "${deduped[@]}"; do
      if [[ "$d" == "$item" ]]; then
        skip=1
        break
      fi
    done
  fi
  (( skip == 0 )) && deduped+=("$item")
done
QUEUE=("${deduped[@]}")

if ((${#QUEUE[@]} == 0)); then
  echo -e "${YELLOW}No scripts selected.${NC}"
  print_usage
  exit 1
fi

# -------------------------------
# Logging header
# -------------------------------
if [[ -n "$LOGFILE" ]]; then
  echo "===== Run started: $(date) =====" >"$LOGFILE"
  echo "Version: $VERSION — Author: $AUTHOR" >>"$LOGFILE"
fi

# -------------------------------
# Execute, time, and collect results
# -------------------------------
LABELS=()
STATUSES=()
DURATIONS=()

for item in "${QUEUE[@]}"; do
  cmd=${item%% :: *}
  label=${item##* :: }

  echo -e "${BLUE}-------------------------------------${NC}"
  echo -e "${BOLD}Running ${label}${NC}"
  echo -e "${BLUE}-------------------------------------${NC}"

  # Existence checks
  if [[ "$label" == *.py || "$label" == *.PY ]]; then
    [[ -f "$label" ]] || { echo -e "${RED}Error:${NC} Python script '$label' not found."; exit 1; }
  elif [[ "$label" == *.C || "$label" == *.cxx || "$label" == *.C++ ]]; then
    [[ -f "$label" ]] || { echo -e "${RED}Error:${NC} ROOT macro '$label' not found."; exit 1; }
  fi

  start_ns=$(now_ns)

  if [[ -n "$LOGFILE" ]]; then
    # Run in a *subshell* so exit does not kill the main script
    (
      echo ">>> Starting $label at $(date)"
      if eval "$cmd"; then
        rc=0
      else
        rc=$?
      fi
      echo "<<< Finished $label at $(date)"
      exit $rc
    ) >>"$LOGFILE" 2>&1
    ec=$?
  else
    # Guard against `set -e` aborting the driver
    if eval "$cmd"; then
      ec=0
    else
      ec=$?
    fi
  fi

  end_ns=$(now_ns)
  dur_ms=$(( (end_ns - start_ns) / 1000000 ))
  # Format seconds.milliseconds without awk/bc
  dur_fmt=$(printf "%d.%03d" $((dur_ms/1000)) $((dur_ms%1000)))

  LABELS+=("$label")
  STATUSES+=("$ec")
  DURATIONS+=("$dur_fmt")

  if [[ $ec -eq 0 ]]; then
    echo -e "${GREEN}✔ Completed:${NC} $label in ${DIM}${dur_fmt}s${NC}"
  else
    echo -e "${RED}✖ Failed:${NC} $label in ${DIM}${dur_fmt}s${NC} (exit $ec)"
  fi

  if [[ -n "$LOGFILE" ]]; then
    echo "[$(date)] $label : exit $ec, ${dur_fmt}s" >>"$LOGFILE"
  fi
done

# -------------------------------
# Summary
# -------------------------------
echo -e "${BLUE}=====================================${NC}"
echo -e "${BOLD}Summary${NC}"
echo -e "${BLUE}=====================================${NC}"

maxlen=0
for lbl in "${LABELS[@]:-}"; do
  (( ${#lbl} > maxlen )) && maxlen=${#lbl}
done

total_ok=0
total_fail=0
total_time_ms=0

idxs=("${!LABELS[@]}")
for i in "${idxs[@]:-}"; do
  lbl="${LABELS[$i]}"
  ec="${STATUSES[$i]}"
  dur="${DURATIONS[$i]}"
  dur_ms=$(awk -v d="$dur" 'BEGIN { printf "%.0f", d*1000 }')
  total_time_ms=$(( total_time_ms + dur_ms ))

  pad=$(( maxlen - ${#lbl} ))
  printf "  %s%*s : " "$lbl" "$pad" ""
  if [[ $ec -eq 0 ]]; then
    echo -e "${GREEN}OK${NC} (${DIM}${dur}s${NC})"
    ((total_ok++))
  else
    echo -e "${RED}FAILED${NC} (${DIM}${dur}s${NC}, exit $ec)"
    ((total_fail++))
  fi
done

total_fmt=$(printf "%.3f" "$(awk "BEGIN {print $total_time_ms/1000}")")
echo -e "${BOLD}Totals:${NC} ${GREEN}${total_ok} ok${NC}, ${RED}${total_fail} failed${NC}, time ${DIM}${total_fmt}s${NC}"

if [[ -n "$LOGFILE" ]]; then
  echo "===== Run finished: $(date) =====" >>"$LOGFILE"
  echo "Summary: ${total_ok} ok, ${total_fail} failed, time ${total_fmt}s" >>"$LOGFILE"
  echo "All script output saved to $LOGFILE"
fi
