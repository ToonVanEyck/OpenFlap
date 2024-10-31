import pytest
from pytest_embedded_idf.dut import IdfDut
from pytest_embedded_qemu.dut import QemuDut
from pytest_embedded_qemu.app import QemuApp
import requests
import os
import re

IP_AD_REGEX = r"ip:\s(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})"


def launch_unity_test_by_name(dut, name):
    testcase = [c for c in dut.test_menu if c.name == name][0]
    dut._get_ready(timeout=5)
    dut.write(str(testcase.index))


def assert_valid_webdata(ip_address, localhost_path, file_path):
    response = requests.get(f"http://{ip_address}:80{localhost_path}")
    assert response.status_code == 200
    with open(file_path, "r", encoding="utf-8") as file:
        content = file.read()
    assert response.text == content, "The content does not match"


def test_webserver(dut: IdfDut, mark) -> None:
    launch_unity_test_by_name(dut, "Test webserver")

    ip_match = dut.expect(IP_AD_REGEX)
    ip_address = ip_match.group(1).decode("utf-8")

    if mark == "qemu":
        ip_address = "localhost"

    dut.expect_exact("Webserver started")

    assets_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), "assets")
    assert_valid_webdata(ip_address, "", os.path.join(assets_dir, "index.html"))
    assert_valid_webdata(
        ip_address, "/style.css", os.path.join(assets_dir, "style.css")
    )
    assert_valid_webdata(
        ip_address, "/script.js", os.path.join(assets_dir, "script.js")
    )

    dut.write(b"\n")  # Signal the rest to stop
    dut.expect_exact("Webserver stopped")
    dut.expect_unity_test_output()


@pytest.mark.esp32
@pytest.mark.qemu
@pytest.mark.parametrize("config", ["qemu"], indirect=True)
def test_webserver_on_qemu(dut: IdfDut) -> None:
    test_webserver(dut, "qemu")


@pytest.mark.esp32
@pytest.mark.target
@pytest.mark.parametrize("config", ["target"], indirect=True)
def test_webserver_on_target(dut: IdfDut) -> None:
    test_webserver(dut, "target")
