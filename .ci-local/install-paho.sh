#!/usr/bin/env bash
set -euo pipefail

PAHO_CPP_VERSION="v1.5.3"

if ldconfig -p 2>/dev/null | grep -q "libpaho-mqttpp3\.so" && ldconfig -p 2>/dev/null | grep -q "libpaho-mqtt3as\.so"; then
  echo "PAHO libraries already available; skipping build"
  exit 0
fi

workdir="${HOME}/.cache/paho-build"
rm -rf "${workdir}"
mkdir -p "${workdir}"
cd "${workdir}"

git clone --recursive --branch "${PAHO_CPP_VERSION}" https://github.com/eclipse-paho/paho.mqtt.cpp.git
cd paho.mqtt.cpp

git submodule update --init

cmake -S . -B build \
  -DPAHO_WITH_MQTT_C=ON \
  -DPAHO_BUILD_SHARED=ON \
  -DPAHO_BUILD_EXAMPLES=OFF

cmake --build build --parallel
sudo cmake --install build
sudo ldconfig
