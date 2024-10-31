import pytest
from pytest_embedded_idf.dut import IdfDut


def run_unity_tests_qemu(dut: IdfDut) -> None:
    """
    This function iterates through the test menu of the DUT and runs tests that belong to the 'qemu' group.
    After each test, it performs a hard reset on the DUT and waits for a short period.
    This function is used to replace `dut.run_all_single_board_cases(group='qemu',reset=True)`, which does not work because it does not have the delay.
    Args:
        dut (IdfDut): The Device Under Test, which contains the test menu and methods to run tests and reset the device.
    Returns:
        None
    """
    for test in dut.test_menu:
        if "qemu" in test.groups:
            # Run the test
            dut.run_single_board_case(test.name, timeout=10)
            # This kind of flushes the output of the test so that the next test only starts when the reset is done.
            dut._get_ready(timeout=3)
            # Reset the board
            dut.hard_reset()


@pytest.mark.esp32
@pytest.mark.qemu
@pytest.mark.parametrize("config", ["qemu"], indirect=True)
def test_networking_unity_on_qemu(dut: IdfDut) -> None:
    run_unity_tests_qemu(dut)


@pytest.mark.esp32
@pytest.mark.target
@pytest.mark.parametrize("config", ["target"], indirect=True)
def test_networking_unity_on_esp(dut: IdfDut) -> None:
    dut.run_all_single_board_cases(group="target", reset=True)
