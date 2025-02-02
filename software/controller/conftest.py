# SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0
# pylint: disable=W0621  # redefined-outer-name
#
# IDF is using [pytest](https://github.com/pytest-dev/pytest) and
# [pytest-embedded plugin](https://github.com/espressif/pytest-embedded) as its test framework.
#
# if you found any bug or have any question,
# please report to https://github.com/espressif/pytest-embedded/issues
# or discuss at https://github.com/espressif/pytest-embedded/discussions
import os
import sys

if os.path.join(os.environ['IDF_PATH'], 'tools', 'ci') not in sys.path:
    sys.path.append(os.path.join(os.environ['IDF_PATH'], 'tools', 'ci'))

if os.path.join(os.environ['IDF_PATH'], 'tools', 'ci', 'python_packages') not in sys.path:
    sys.path.append(os.path.join(os.environ['IDF_PATH'], 'tools', 'ci', 'python_packages'))

import glob
import io
import logging
import os
import re
import typing as t
import zipfile
from copy import deepcopy
from urllib.parse import quote

import common_test_methods  # noqa: F401
import pytest
import requests
import yaml
from _pytest.config import Config
from _pytest.fixtures import FixtureRequest
from artifacts_handler import ArtifactType
from idf_ci.app import import_apps_from_txt
from idf_ci.uploader import AppDownloader, AppUploader
from idf_ci_utils import IDF_PATH, idf_relpath
from idf_pytest.constants import DEFAULT_SDKCONFIG, ENV_MARKERS, SPECIAL_MARKERS, TARGET_MARKERS, PytestCase, \
    DEFAULT_LOGDIR
from idf_pytest.plugin import IDF_PYTEST_EMBEDDED_KEY, ITEM_PYTEST_CASE_KEY, IdfPytestEmbedded
from idf_pytest.utils import format_case_id
from pytest_embedded.plugin import multi_dut_argument, multi_dut_fixture
from pytest_embedded_idf.dut import IdfDut
from pytest_embedded_idf.unity_tester import CaseTester


############
# Fixtures #
############
@pytest.fixture(scope='session')
def idf_path() -> str:
    return os.environ['IDF_PATH']


@pytest.fixture(scope='session')
def session_root_logdir(idf_path: str) -> str:
    """Session scoped log dir for pytest-embedded"""
    return idf_path


# @pytest.fixture
# def case_tester(unity_tester: CaseTester) -> CaseTester:
#     return unity_tester


@pytest.fixture
@multi_dut_argument
def config(request: FixtureRequest) -> str:
    return getattr(request, 'param', None) or DEFAULT_SDKCONFIG  # type: ignore


@pytest.fixture
@multi_dut_fixture
def target(request: FixtureRequest, dut_total: int, dut_index: int) -> str:
    plugin = request.config.stash[IDF_PYTEST_EMBEDDED_KEY]

    if dut_total == 1:
        return plugin.target[0]  # type: ignore

    return plugin.target[dut_index]  # type: ignore


@pytest.fixture
def test_func_name(request: FixtureRequest) -> str:
    return request.node.function.__name__  # type: ignore


@pytest.fixture
def test_case_name(request: FixtureRequest, target: str, config: str) -> str:
    is_qemu = request._pyfuncitem.get_closest_marker('qemu') is not None
    if hasattr(request._pyfuncitem, 'callspec'):
        params = deepcopy(request._pyfuncitem.callspec.params)  # type: ignore
    else:
        params = {}

    filtered_params = {}
    for k, v in params.items():
        if k not in request.session._fixturemanager._arg2fixturedefs:  # type: ignore
            filtered_params[k] = v  # not fixture ones

    return format_case_id(target, config, request.node.originalname, is_qemu=is_qemu, params=filtered_params)  # type: ignore


@pytest.fixture
@multi_dut_fixture
def build_dir(
    request: FixtureRequest,
    app_path: str,
    target: t.Optional[str],
    config: t.Optional[str],
) -> str:
    """
    Check local build dir with the following priority:

    1. build_<target>_<config>
    2. build_<target>
    3. build_<config>
    4. build

    Returns:
        valid build directory
    """

    build_path = os.path.abspath(request.config.option.build_dir)
    src_path = os.path.abspath(request.config.option.file_or_dir[0])
    project_name = os.path.basename(src_path)
    component_name = relative_path = os.path.basename(os.path.dirname(os.path.relpath(app_path, src_path)))
    component_build_path = os.path.join(build_path, component_name)

    # logging.info(f'build_path: {build_path}')
    # logging.info(f'src_path: {src_path}')
    # logging.info(f'app_path: {app_path}')
    # logging.info(f'project_name: {project_name}')
    # logging.info(f'component_name: {component_name}')

    case: PytestCase = request._pyfuncitem.stash[ITEM_PYTEST_CASE_KEY]
    check_dirs = []
    if target is not None and config is not None:
        check_dirs.append(f'build_{target}_{config}')
    if target is not None:
        check_dirs.append(f'build_{target}')
    if config is not None:
        check_dirs.append(f'build_{config}')
    check_dirs.append('build')

    for check_dir in check_dirs:
        binary_path = os.path.join(component_build_path, check_dir)
        if os.path.isdir(binary_path):
            logging.info(f'found valid binary path: {binary_path}')
            return binary_path

        logging.warning('checking binary path: %s... missing... try another place', binary_path)

    raise ValueError(
        f'no build dir valid. Please build the binary via "idf.py -B {component_build_path}/{check_dirs[0]} build -C /opt/esp/idf/tools/unit-test-app/ -D EXTRA_COMPONENT_DIRS={os.path.join(src_path,"components")} -T {component_name}" and run pytest again'
    )


@pytest.fixture(autouse=True)
@multi_dut_fixture
def junit_properties(test_case_name: str, record_xml_attribute: t.Callable[[str, object], None]) -> None:
    """
    This fixture is autoused and will modify the junit report test case name to <target>.<config>.<case_name>
    """
    record_xml_attribute('name', test_case_name)


@pytest.fixture(autouse=True)
@multi_dut_fixture
def ci_job_url(record_xml_attribute: t.Callable[[str, object], None]) -> None:
    if ci_job_url := os.getenv('CI_JOB_URL'):
        record_xml_attribute('ci_job_url', ci_job_url)


@pytest.fixture(autouse=True)
def set_test_case_name(request: FixtureRequest, test_case_name: str) -> None:
    request.node.funcargs['test_case_name'] = test_case_name


######################
# Log Util Functions #
######################
@pytest.fixture
def log_performance(record_property: t.Callable[[str, object], None]) -> t.Callable[[str, str], None]:
    """
    log performance item with pre-defined format to the console
    and record it under the ``properties`` tag in the junit report if available.
    """

    def real_func(item: str, value: str) -> None:
        """
        :param item: performance item name
        :param value: performance value
        """
        logging.info('[Performance][%s]: %s', item, value)
        record_property(item, value)

    return real_func


@pytest.fixture
def check_performance(idf_path: str) -> t.Callable[[str, float, str], None]:
    """
    check if the given performance item meets the passing standard or not
    """

    def real_func(item: str, value: float, target: str) -> None:
        """
        :param item: performance item name
        :param value: performance item value
        :param target: target chip
        :raise: AssertionError: if check fails
        """

        def _find_perf_item(operator: str, path: str) -> float:
            with open(path) as f:
                data = f.read()
            match = re.search(fr'#define\s+IDF_PERFORMANCE_{operator}_{item.upper()}\s+([\d.]+)', data)
            return float(match.group(1))  # type: ignore

        def _check_perf(operator: str, standard_value: float) -> None:
            if operator == 'MAX':
                ret = value <= standard_value
            else:
                ret = value >= standard_value
            if not ret:
                raise AssertionError(
                    f"[Performance] {item} value is {value}, doesn't meet pass standard {standard_value}"
                )

        path_prefix = os.path.join(idf_path, 'components', 'idf_test', 'include')
        performance_files = (
            os.path.join(path_prefix, target, 'idf_performance_target.h'),
            os.path.join(path_prefix, 'idf_performance.h'),
        )

        found_item = False
        for op in ['MIN', 'MAX']:
            for performance_file in performance_files:
                try:
                    standard = _find_perf_item(op, performance_file)
                except (OSError, AttributeError):
                    # performance file doesn't exist or match is not found in it
                    continue

                _check_perf(op, standard)
                found_item = True
                break

        if not found_item:
            raise AssertionError(f'Failed to get performance standard for {item}')

    return real_func


@pytest.fixture
def log_minimum_free_heap_size(dut: IdfDut, config: str) -> t.Callable[..., None]:
    def real_func() -> None:
        res = dut.expect(r'Minimum free heap size: (\d+) bytes')
        logging.info(
            '\n------ heap size info ------\n'
            '[app_name] {}\n'
            '[config_name] {}\n'
            '[target] {}\n'
            '[minimum_free_heap_size] {} Bytes\n'
            '------ heap size end ------'.format(
                os.path.basename(dut.app.app_path),
                config,
                dut.target,
                res.group(1).decode('utf8'),
            )
        )

    return real_func


@pytest.fixture(scope='session')
def dev_password(request: FixtureRequest) -> str:
    return request.config.getoption('dev_passwd') or ''


@pytest.fixture(scope='session')
def dev_user(request: FixtureRequest) -> str:
    return request.config.getoption('dev_user') or ''


##################
# Hook functions #
##################
def pytest_addoption(parser: pytest.Parser) -> None:
    idf_group = parser.getgroup('idf')
    idf_group.addoption(
        '--sdkconfig',
        help='sdkconfig postfix, like sdkconfig.ci.<config>. (Default: None, which would build all found apps)',
    )
    idf_group.addoption(
        '--dev-user',
        help='user name associated with some specific device/service used during the test execution',
    )
    idf_group.addoption(
        '--dev-passwd',
        help='password associated with some specific device/service used during the test execution',
    )
    idf_group.addoption(
        '--app-info-filepattern',
        help='glob pattern to specify the files that include built app info generated by '
        '`idf-build-apps --collect-app-info ...`. will not raise ValueError when binary '
        'paths not exist in local file system if not listed recorded in the app info.',
    )


def pytest_configure(config: Config) -> None:
    # cli option "--target"
    target = [_t.strip().lower() for _t in (config.getoption('target', '') or '').split(',') if _t.strip()]

    # add markers based on idf_pytest/constants.py
    for name, description in {
        **TARGET_MARKERS,
        **ENV_MARKERS,
        **SPECIAL_MARKERS,
    }.items():
        config.addinivalue_line('markers', f'{name}: {description}')

    help_commands = ['--help', '--fixtures', '--markers', '--version']
    for cmd in help_commands:
        if cmd in config.invocation_params.args:
            target = ['unneeded']
            break

    markexpr = config.getoption('markexpr') or ''
    # check marker expr set via "pytest -m"
    if not target and markexpr:
        # we use `-m "esp32 and generic"` in our CI to filter the test cases
        # this doesn't cover all use cases, but fit what we do in CI.
        for marker in markexpr.split('and'):
            marker = marker.strip()
            if marker in TARGET_MARKERS:
                target.append(marker)

    # "--target" must be set
    if not target:
        raise SystemExit(
            """Pass `--target TARGET[,TARGET...]` to specify all targets the test cases are using.
    - for single DUT, we run with `pytest --target esp32`
    - for multi DUT, we run with `pytest --target esp32,esp32,esp32s2` to indicate all DUTs
"""
        )

    apps = None
    app_info_filepattern = config.getoption('app_info_filepattern')
    if app_info_filepattern:
        apps = []
        for f in glob.glob(os.path.join(IDF_PATH, app_info_filepattern)):
            apps.extend(import_apps_from_txt(f))

    if '--collect-only' not in config.invocation_params.args:
        config.stash[IDF_PYTEST_EMBEDDED_KEY] = IdfPytestEmbedded(
            config_name=config.getoption('sdkconfig'),
            target=target,
            apps=apps,
        )
        config.pluginmanager.register(config.stash[IDF_PYTEST_EMBEDDED_KEY])


def pytest_unconfigure(config: Config) -> None:
    _pytest_embedded = config.stash.get(IDF_PYTEST_EMBEDDED_KEY, None)
    if _pytest_embedded:
        del config.stash[IDF_PYTEST_EMBEDDED_KEY]
        config.pluginmanager.unregister(_pytest_embedded)


dut_artifacts_url = []


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item, call):  # type: ignore
    outcome = yield
    report = outcome.get_result()
    report.sections = []
    if report.failed:
        _dut = item.funcargs.get('dut')
        if not _dut:
            return

        job_id = os.getenv('CI_JOB_ID', 0)
        url = os.getenv('CI_PAGES_URL', '').replace('esp-idf', '-/esp-idf')
        template = f'{url}/-/jobs/{job_id}/artifacts/{DEFAULT_LOGDIR}/{{}}'
        logs_files = []

        def get_path(x: str) -> str:
            return x.split(f'{DEFAULT_LOGDIR}/', 1)[1]

        if isinstance(_dut, list):
            logs_files.extend([template.format(get_path(d.logfile)) for d in _dut])
            dut_artifacts_url.append('{}:'.format(_dut[0].test_case_name))
        else:
            logs_files.append(template.format(get_path(_dut.logfile)))
            dut_artifacts_url.append('{}:'.format(_dut.test_case_name))

        for file in logs_files:
            dut_artifacts_url.append('    - {}'.format(quote(file, safe=':/')))


def pytest_terminal_summary(terminalreporter, exitstatus, config):  # type: ignore
    if dut_artifacts_url:
        terminalreporter.ensure_newline()
        terminalreporter.section('Failed Test Artifacts URL', sep='-', red=True, bold=True)
        terminalreporter.line('\n'.join(dut_artifacts_url))
