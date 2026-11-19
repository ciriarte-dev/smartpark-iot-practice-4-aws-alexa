#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/smartpark-tests.XXXXXX")
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic \
    -Itest/host/local_fakes -Itest/host/fakes -Iinclude test/host/test_main.cpp \
    src/UltrasonicSensor.cpp src/BarrierServo.cpp src/Parking.cpp src/ParkingDisplay.cpp src/main.cpp -o "$build_dir/tests"
"$build_dir/tests"
# JSON real; red, RTOS, TLS y NVS se sustituyen por dobles controlados.
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic \
    -DARDUINOJSON_ENABLE_ARDUINO_STRING=0 -DARDUINOJSON_ENABLE_ARDUINO_STREAM=0 \
    -DARDUINOJSON_ENABLE_PROGMEM=0 \
    -Itest/host/aws_fakes -Itest/host/fakes -Iinclude \
    -I.pio/libdeps/esp32doit-devkit-v1/ArduinoJson/src \
    test/host/test_aws.cpp src/AwsIoTManager.cpp src/HistoryPublisher.cpp src/Parking.cpp src/BarrierServo.cpp src/UltrasonicSensor.cpp -o "$build_dir/aws-tests"
if [ -n "${SMARTPARK_HISTORY_PYTHON:-}" ]; then
    SMARTPARK_HISTORY_FIXTURES="$build_dir/history.jsonl" "$build_dir/aws-tests"
    AWS_EC2_METADATA_DISABLED=true PYTHONDONTWRITEBYTECODE=1 "$SMARTPARK_HISTORY_PYTHON" \
        test/check_history_contract.py "$build_dir/history.jsonl"
else
    "$build_dir/aws-tests"
fi
