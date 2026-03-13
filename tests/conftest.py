# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 André Favoto

import os
import subprocess
import time
import uuid
from pathlib import Path

import pytest
from p4p.client.thread import Context


ROOT = Path(__file__).resolve().parents[1]
IOC_BOOT_DIR = ROOT / "iocBoot" / "ioctest"
TESTS_DIR = Path(__file__).resolve().parent
PV_PREFIX = "mqtt:test:"
# Default mosquitto installation binds to 1883. Use a non-default port to avoid conflict.
BROKER_PORT = 18830


@pytest.fixture(scope="session")
def mqtt_broker():
    """Start a local Mosquitto broker process for the test session."""
    conf_path = TESTS_DIR / "mosquitto.conf"

    proc = subprocess.Popen(
        ["mosquitto", "-c", str(conf_path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    # Wait for the broker to be ready
    time.sleep(1.0)
    if proc.poll() is not None:
        raise RuntimeError(
            "Local Mosquitto broker failed to start.\n"
            "Ensure 'mosquitto' is installed (e.g. sudo apt-get install mosquitto).\n"
        )

    broker_url = f"mqtt://localhost:{BROKER_PORT}"
    yield broker_url

    proc.terminate()
    try:
        proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        proc.kill()


@pytest.fixture(scope="session")
def pva_context(mqtt_broker):
    env = os.environ.copy()
    env["MQTT_TEST_CLIENT_ID"] = f"epicsMQTT-ci-{uuid.uuid4().hex[:10]}"
    env["MQTT_TEST_TOPIC_ROOT"] = f"epicsMQTT/ci/{uuid.uuid4().hex[:10]}"
    env["MQTT_TEST_BROKER_URL"] = mqtt_broker

    ioc_proc = subprocess.Popen(
        ["./st.cmd"],
        cwd=IOC_BOOT_DIR,
        env=env,
        stdin=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    time.sleep(1.0)  # IOC startup + MQTT connection
    context = Context("pva")
    try:
        context.get(f"{PV_PREFIX}Int32Input", timeout=1.0)
        yield context
    except Exception as error:
        raise RuntimeError(f"Failed to start IOC process or connect to PVs: {error}")
    finally:
        context.close()
        ioc_proc.terminate()
