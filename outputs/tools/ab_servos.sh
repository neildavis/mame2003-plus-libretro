#!/bin/bash 

# Configuration - Pins
PIN_HORIZ_H_SERVO=26
PIN_HORIZ_V_SERVO=16

# Percentage-based Midpoints (from your C code)
MID_H_PCT=1500
MID_V_PCT=1500

# Range limits for Percentage calculations
MAX_H_LIMIT=2000
RANGE_H=$((MAX_H_LIMIT - MID_H_PCT))
MAX_V_LIMIT=2000
RANGE_V=$((MAX_V_LIMIT - MID_V_PCT))

# Absolute Hardware Limits for Direct PWM
ABS_MIN=750
ABS_MAX=2250
DEFAULT_PWM=1500

# Initialize variables
RAW_H_PCT=""
RAW_V_PCT=""
DIRECT_H=""
DIRECT_V=""

# --- Parse Named Parameters ---
while getopts "h:v:H:V:" opt; do
  case $opt in
    h) RAW_H_PCT=$OPTARG ;;
    v) RAW_V_PCT=$OPTARG ;;
    H) DIRECT_H=$OPTARG ;;
    V) DIRECT_V=$OPTARG ;;
    *) echo "Usage: $0 [-h %] [-v %] [-H pwm] [-V pwm]"; exit 1 ;;
  esac
done

# --- Helper Functions ---

clamp() {
    local val=$1
    local min=$2
    local max=$3
    if (( val < min )); then echo "$min"
    elif (( val > max )); then echo "$max"
    else echo "$val"; fi
}

# --- Validation & Daemon Check ---

if [[ ! -x /usr/bin/pigs ]]; then
    echo "-------------------------------------------------------"
    echo "ERROR: 'pigs' (pigpio) is not installed."
    echo "To fix: sudo apt update && sudo apt install pigpio"
    echo "Then start with: sudo pigpiod"
    echo "-------------------------------------------------------"
    exit 1
fi

if ! pgrep pigpiod > /dev/null; then
    sudo pigpiod && sleep 1
fi

# --- Logic: Determine Final PWM Values ---

# Horizontal Axis Logic
if [[ -n "$DIRECT_H" ]]; then
    FINAL_H=$(clamp "$DIRECT_H" "$ABS_MIN" "$ABS_MAX")
elif [[ -n "$RAW_H_PCT" ]]; then
    SAFE_H_PCT=$(clamp "$RAW_H_PCT" -100 100)
    FINAL_H=$(( MID_H_PCT + (SAFE_H_PCT * RANGE_H / 100) ))
else
    # Default if no horizontal params provided
    FINAL_H=$DEFAULT_PWM
fi

# Vertical Axis Logic
if [[ -n "$DIRECT_V" ]]; then
    FINAL_V=$(clamp "$DIRECT_V" "$ABS_MIN" "$ABS_MAX")
elif [[ -n "$RAW_V_PCT" ]]; then
    SAFE_V_PCT=$(clamp "$RAW_V_PCT" -100 100)
    FINAL_V=$(( MID_V_PCT + (SAFE_V_PCT * RANGE_V / 100) ))
else
    # Default if no vertical params provided
    FINAL_V=$DEFAULT_PWM
fi

# --- Move Servos ---
echo "Moving: H -> ${FINAL_H}us | V -> ${FINAL_V}us"
pigs servo "${PIN_HORIZ_H_SERVO}" "${FINAL_H}"
pigs servo "${PIN_HORIZ_V_SERVO}" "${FINAL_V}"