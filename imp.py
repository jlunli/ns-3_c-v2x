"""Minimal compatibility shim for legacy waf on Python 3.12+."""

from __future__ import annotations

import sys
import types


def new_module(name: str) -> types.ModuleType:
    return types.ModuleType(name)


def get_tag() -> str:
    return getattr(sys.implementation, "cache_tag", "cpython-unknown")
