import math
import time
from pathlib import Path

import pytest


IOC_LOG = Path(__file__).resolve().parents[1] / "iocBoot" / "ioctest" / "pytest-ioc.log"


def _get_value(actual):
    if hasattr(actual, "value"):
        try:
            actual = actual.value
        except Exception:
            pass

    if hasattr(actual, "tolist"):
        try:
            actual = actual.tolist()
        except Exception:
            pass

    return actual


def _readback_matches(actual, expected):
    actual = _get_value(actual)

    if isinstance(expected, list):
        try:
            actual_list = list(actual)
        except TypeError:
            return False

        if len(actual_list) != len(expected):
            return False

        if all(isinstance(x, float) for x in expected):
            return all(
                math.isclose(float(a), float(e), rel_tol=1e-6, abs_tol=1e-6)
                for a, e in zip(actual_list, expected)
            )
        return actual_list == expected

    if isinstance(expected, float):
        return math.isclose(float(actual), expected, rel_tol=1e-6, abs_tol=1e-6)
    return actual == expected


def _put_and_wait(context, output_pv, input_pv, value, timeout=10.0):
    context.put(output_pv, value, timeout=10.0)

    deadline = time.monotonic() + timeout
    last_value = None
    while time.monotonic() < deadline:
        last_value = context.get(input_pv, timeout=2.0)
        if _readback_matches(last_value, value):
            return
        time.sleep(0.5)

    raise AssertionError(f"Round-trip timed out for {output_pv} / {input_pv}. ")


@pytest.mark.parametrize(
    ("output_pv", "input_pv", "value"),
    [
        ("mqtt:test:Int32Output", "mqtt:test:Int32Input", 42),
        ("mqtt:test:Float64Output", "mqtt:test:Float64Input", 3.14159),
        ("mqtt:test:StringOutput", "mqtt:test:StringInput", "epicsMQTT-ci"),
        ("mqtt:test:IntArrayOutput", "mqtt:test:IntArrayInput", [1, 2, 3, 4, 5]),
        ("mqtt:test:FloatArrayOutput", "mqtt:test:FloatArrayInput", [1.1, 2.2, 3.3, 4.4, 5.5]),
    ],
)
def test_round_trip_via_broker(pva_context, output_pv, input_pv, value):
    _put_and_wait(pva_context, output_pv, input_pv, value)
