#!/bin/bash 

# Configuration - Pins
PIN_HORIZ_H_SERVO=26
PIN_HORIZ_V_SERVO=16

# Horizontal Constants (Range: 730)
MID_H=1520
MAX_H=2250
RANGE_H=$((MAX_H - MID_H))

# Vertical Constants (Range: 250)
MID_V=1450
MAX_V=1700
RANGE_V=$((MAX_V - MID_V))

# Default values
RAW_H=0
RAW_V=0

# --- Parse Named Parameters ---
while getopts "h:v:" opt; do
  case $opt in
    h) RAW_H=$OPTARG ;;
    v) RAW_V=$OPTARG ;;
    *) echo "Usage: $0 [-h horizontal_percent] [-v vertical_percent]"; exit 1 ;;
  esac
done

# --- Helper Functions ---

clamp() {
    local val=$1
    if (( val < -100 )); then echo -100
    elif (( val > 100 )); then echo 100
    else echo "$val"; fi
}

calculate_pwm() {
    local mid=$1
    local range=$2
    local percent=$3
    echo $(( mid + (percent * range / 100) ))
}

# --- Validation & Error Handling ---

if [[ ! -x /usr/bin/pigs ]]; then
    echo "-------------------------------------------------------"
    echo "ERROR: 'pigs' (pigpio) is not installed on this system."
    echo "To fix this, please run the following commands:"
    echo "  1. sudo apt update"
    echo "  2. sudo apt install pigpio"
    echo "  3. sudo pigpiod"
    echo "-------------------------------------------------------"
    exit 1
fi

if ! pgrep pigpiod > /dev/null; then
    echo "NOTICE: pigpiod daemon is not running. Starting it now..."
    sudo pigpiod
    sleep 1 # Give the daemon a moment to initialize
fi

# --- Main Logic ---

PERCENT_H=$(clamp "$RAW_H")
PERCENT_V=$(clamp "$RAW_V")

PWM_H=$(calculate_pwm $MID_H $RANGE_H $PERCENT_H)
PWM_V=$(calculate_pwm $MID_V $RANGE_V $PERCENT_V)

echo "Setting: H ${PERCENT_H}% (${PWM_H}us) | V ${PERCENT_V}% (${PWM_V}us)"
pigs servo "${PIN_HORIZ_H_SERVO}" "${PWM_H}"
pigs servo "${PIN_HORIZ_V_SERVO}" "${PWM_V}"
