import pytest
from pytest_embedded_idf.dut import IdfDut
import json
from typing import List
import requests
import re

IP_AD_REGEX = r"ip:\s(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})"


def launch_unity_test_by_name(dut, name):
    testcase = [c for c in dut.test_menu if c.name == name][0]
    dut._get_ready(timeout=5)
    dut.write(str(testcase.index))


# @pytest.mark.esp32
# @pytest.mark.qemu
# @pytest.mark.parametrize("config", ["qemu"], indirect=True)
# def test_module_unity_on_qemu(dut: IdfDut) -> None:
#     dut.run_all_single_board_cases(group="property", reset=True)


def test_controller_ota(dut: IdfDut, mark) -> None:
    launch_unity_test_by_name(dut, "Test module http API post handler")

    ip_match = dut.expect(IP_AD_REGEX)
    ip_address = ip_match.group(1).decode("utf-8")

    modules = []
    for i in range(1):
        module = OpenFlapModule(moduleIndex=i)
        modules.append(module)

    json_data = json.dumps(
        [json.loads(module.to_json()) for module in modules],
        indent=4,
        ensure_ascii=False,
    ).encode("utf-8")

    if mark == "qemu":
        ip_address = "localhost"

    dut.expect_exact("Webserver started")

    response = requests.post(
        f"http://{ip_address}:80/api/module",
        data=json_data,
        headers={"Content-Type": "application/json"},
    )
    assert response.status_code == 200

    dut.write(b"\n")  # Signal the rest to stop
    dut.expect_exact("Webserver stopped")
    dut.expect_unity_test_output()


@pytest.mark.esp32
@pytest.mark.qemu
@pytest.mark.parametrize("config", ["qemu"], indirect=True)
def test_controller_ota_on_qemu(dut: IdfDut) -> None:
    test_controller_ota(dut, "qemu")


@pytest.mark.esp32
@pytest.mark.target
@pytest.mark.parametrize("config", ["target"], indirect=True)
def test_controller_ota_on_target(dut: IdfDut) -> None:
    test_controller_ota(dut, "target")
