from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from pathlib import Path

_CACERT_CONF = "core.net.http:cacert_path"


def _conan_cacert_configured(conan: str) -> bool:
    result = subprocess.run(
        [conan, "config", "show", _CACERT_CONF],
        capture_output=True,
        text=True,
        check=False,
    )
    return result.returncode == 0 and bool(result.stdout.strip())


def _detect_ca_cert(conan: str) -> None:
    # The requests package used by conan 2 looks for certificates in
    # REQUESTS_CA_BUNDLE (takes priority) and CURL_CA_BUNDLE environment
    # variables. Additionally, a certificate can be set in conan's
    # configuration. If any of these is set, we keep their value assuming they
    # were set for a reason and return.
    if os.environ.get("REQUESTS_CA_BUNDLE") or os.environ.get("CURL_CA_BUNDLE"):
        return
    if _conan_cacert_configured(conan):
        return
    system_cert = Path("/etc/pki/tls/cert.pem")
    if system_cert.is_file():
        # We cannot inject the certificate when calling conan. Conan 2 rejects
        # core.* in -c, so pass the bundle via requests' env var
        os.environ["REQUESTS_CA_BUNDLE"] = str(system_cert)


def _cmake_args_from_preset(build_dir: Path) -> list[str]:
    presets_file = build_dir / "CMakePresets.json"
    if not presets_file.exists():
        raise RuntimeError(f"Conan did not generate {presets_file}")

    with presets_file.open(encoding="utf-8") as stream:
        presets = json.load(stream)

    try:
        preset = presets["configurePresets"][0]
    except (KeyError, IndexError) as error:
        raise RuntimeError(f"No configure preset found in {presets_file}") from error

    cmake_args: list[str] = []
    if toolchain := preset.get("toolchainFile"):
        toolchain_path = Path(toolchain)
        if not toolchain_path.is_absolute():
            toolchain_path = build_dir / toolchain_path
        cmake_args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_path}")

    for key, value in preset.get("cacheVariables", {}).items():
        if isinstance(value, dict):
            value = value.get("value", "")
        cmake_args.append(f"-D{key}={value}")

    for key, value in preset.get("environment", {}).items():
        os.environ[key] = re.sub(
            r"\$penv\{(\w+)\}",
            lambda match: os.environ.get(match.group(1), ""),
            str(value),
        )

    return cmake_args


def prepare_conan(build_dir: Path, *, generate_user_presets: bool = True) -> list[str]:
    conan = shutil.which("conan")
    if conan is None:
        raise RuntimeError("The Conan executable is required to build resdata")
    _detect_ca_cert(conan)

    build_dir = build_dir.resolve()
    build_dir.mkdir(parents=True, exist_ok=True)
    command = [
        conan,
        "install",
        ".",
        f"--output-folder={build_dir}",
        "--build=missing",
    ]
    if not generate_user_presets:
        command.extend(["-c", "tools.cmake.cmaketoolchain:user_presets="])

    subprocess.run([conan, "profile", "detect", "--force"], check=False)
    subprocess.run(command, check=True)
    return _cmake_args_from_preset(build_dir)
