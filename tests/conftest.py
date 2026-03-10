import os
import subprocess
import time
import uuid
from pathlib import Path

import pytest
from p4p.client.thread import Context


ROOT = Path(__file__).resolve().parents[1]
IOC_BOOT_DIR = ROOT / "iocBoot" / "ioctest"
PV_PREFIX = "mqtt:test:"


@pytest.fixture(scope="session")
def pva_context():
    env = os.environ.copy()
    env["MQTT_TEST_CLIENT_ID"] = f"epicsMQTT-ci-{uuid.uuid4().hex[:10]}"
    env["MQTT_TEST_TOPIC_ROOT"] = f"epicsMQTT/ci/{uuid.uuid4().hex[:10]}"

    ioc_proc = subprocess.Popen(
        ["./st.cmd"],
        cwd=IOC_BOOT_DIR,
        env=env,
        stdin=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    time.sleep(5.0)  # IOC startup + connection to public broker
    context = Context("pva")
    try:
        context.get(f"{PV_PREFIX}Int32Input", timeout=1.0)
        yield context
    except Exception as error:
        raise RuntimeError(f"Failed to start IOC process or connect to PVs: {error}")
    finally:
        context.close()
        ioc_proc.terminate()
