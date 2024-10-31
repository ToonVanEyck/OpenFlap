import pytest
from pytest_embedded_idf.dut import IdfDut


@pytest.mark.esp32
@pytest.mark.qemu
@pytest.mark.parametrize("config", ["qemu"], indirect=True)
def test_modules_unity_on_qemu(dut: IdfDut) -> None:
    dut.run_all_single_board_cases(group="qemu", reset=True)
