#!/bin/bash

PID_FILE="/tmp/openocd_cc1350.pid"

run_build() {
    # Stop OpenOCD if it's already running
    if [[ -f "$PID_FILE" ]]; then
        PID=$(cat "$PID_FILE")
        if ps -p "$PID" > /dev/null; then
            echo "[INFO] Killing existing OpenOCD process (PID $PID)..."
            kill "$PID"
            sleep 1
        fi
        rm -f "$PID_FILE"
    fi

    echo "[INFO] Building and flashing..."
    mkmk CC1350_LAUNCHXL || { echo "[ERROR] mkmk failed"; exit 1; }
    make || { echo "[ERROR] make failed"; exit 1; }

    echo "[INFO] Starting OpenOCD in background..."
    openocd -f board/ti_cc13x0_launchpad.cfg &
    OPENOCD_PID=$!
    echo "$OPENOCD_PID" > "$PID_FILE"
    sleep 1  # allow OpenOCD to initialize

    echo "[INFO] Running GDB script..."
    gdb-multiarch -batch \
        -ex "file Image" \
        -ex "target extended-remote localhost:3333" \
        -ex "monitor reset halt" \
        -ex "load" \
        -ex "continue"

    echo "[INFO] GDB finished. Firmware should now be running."
}

run_serial() {
    echo "[INFO] Starting serial terminal (minicom)..."
    minicom -D /dev/ttyACM0 -b 9600
}

stop_openocd() {
    if [[ -f "$PID_FILE" ]]; then
        PID=$(cat "$PID_FILE")
        if ps -p "$PID" > /dev/null; then
            echo "[INFO] Stopping OpenOCD (PID $PID)..."
            kill "$PID"
            rm "$PID_FILE"
        else
            echo "[WARN] No OpenOCD process found with PID $PID"
            rm "$PID_FILE"
        fi
    else
        echo "[INFO] No OpenOCD PID file found. Nothing to stop."
    fi
}

# Argument parsing
if [[ $# -eq 0 ]]; then
    echo "Usage: $0 [--build] [--serial] [--stop]"
    exit 1
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build)
            run_build
            shift
            ;;
        --serial)
            run_serial
            shift
            ;;
        --stop)
            stop_openocd
            shift
            ;;
        *)
            echo "[ERROR] Unknown option: $1"
            echo "Usage: $0 [--build] [--serial] [--stop]"
            exit 1
            ;;
    esac
done
