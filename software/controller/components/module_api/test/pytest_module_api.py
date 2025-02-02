import pytest
from pytest_embedded_idf.dut import IdfDut
import json
from typing import List
import requests
import re

IP_AD_REGEX = r"ip:\s(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})"


class OpenFlapModule:
    _module_counter = 0

    def __init__(
        self,
        moduleIndex: int = None,
        columnEnd: bool = False,
        characterMapSize: int = 48,
        characterMap: List[str] = None,
        offset: int = 0,
        vtrim: int = 0,
        character: str = " ",
    ):
        if moduleIndex is None:
            self.moduleIndex = OpenFlapModule._module_counter
            OpenFlapModule._module_counter += 1
        else:
            self.moduleIndex = moduleIndex

        self.columnEnd = columnEnd
        self.characterMapSize = characterMapSize
        self.characterMap = (
            characterMap
            if characterMap is not None
            else list(
                " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789*$!?.,:/@#&"
            )  # Only use ASCII characters because pytest cannot handle non-ASCII characters
        )
        self.offset = offset
        self.vtrim = vtrim
        self.character = character

    def to_json(self) -> str:
        return json.dumps(
            self, default=lambda o: o.__dict__, indent=4, ensure_ascii=False
        )

    @classmethod
    def from_json(cls, json_str: str):
        data = json.loads(json_str)
        return cls(**data)


def launch_unity_test_by_name(dut, name):
    testcase = [c for c in dut.test_menu if c.name == name][0]
    dut._get_ready(timeout=5)
    dut.write(str(testcase.index))


# @pytest.mark.esp32
# @pytest.mark.qemu
# @pytest.mark.parametrize("config", ["qemu"], indirect=True)
# def test_module_unity_on_qemu(dut: IdfDut) -> None:
#     dut.run_all_single_board_cases(group="property", reset=True)


def test_module_api(dut: IdfDut, mark) -> None:
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
def test_module_api_on_qemu(dut: IdfDut) -> None:
    test_module_api(dut, "qemu")


@pytest.mark.esp32
@pytest.mark.target
@pytest.mark.parametrize("config", ["target"], indirect=True)
def test_module_api_on_target(dut: IdfDut) -> None:
    test_module_api(dut, "target")
