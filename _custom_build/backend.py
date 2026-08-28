from __future__ import annotations

import os
from pathlib import Path
from typing import Any

from conan import prepare_conan
from scikit_build_core.build import *  # noqa: F403
from scikit_build_core.build import build_editable as _build_editable
from scikit_build_core.build import build_wheel as _build_wheel


def _prepare_conan(config_settings: dict[str, Any] | None) -> None:
    settings = config_settings or {}
    configured_dir = settings.get(
        "build-dir",
        settings.get("skbuild.build-dir", os.environ.get("SKBUILD_BUILD_DIR", "build")),
    )
    if isinstance(configured_dir, list):
        configured_dir = configured_dir[-1]
    cmake_args = prepare_conan(Path(configured_dir), generate_user_presets=False)
    existing_args = os.environ.get("SKBUILD_CMAKE_ARGS", "")
    os.environ["SKBUILD_CMAKE_ARGS"] = ";".join(
        value for value in (existing_args, *cmake_args) if value
    )


def build_wheel(
    wheel_directory: str,
    config_settings: dict[str, Any] | None = None,
    metadata_directory: str | None = None,
) -> str:
    _prepare_conan(config_settings)
    return _build_wheel(wheel_directory, config_settings, metadata_directory)


def build_editable(
    wheel_directory: str,
    config_settings: dict[str, Any] | None = None,
    metadata_directory: str | None = None,
) -> str:
    _prepare_conan(config_settings)
    return _build_editable(wheel_directory, config_settings, metadata_directory)
