"""Helpers for handing a Python file object's descriptor to the C layer."""

from __future__ import annotations

import os
from contextlib import contextmanager
from typing import IO, Iterator


@contextmanager
def synced_fd(file: IO) -> Iterator[int]:
    """Yields the file descriptor of @file for use by the C layer.

    The C code reads and writes directly through the file descriptor, thereby
    bypassing the buffers of the Python file object. In order for the two to
    agree on where in the file they are, this context manager flushes any
    pending writes and positions the descriptor at the logical position of the
    Python file object before yielding it, and repositions the Python file
    object to wherever the C code left the descriptor afterwards.

    Non-seekable files, such as pipes, have no position to keep in sync and are
    only flushed.
    """
    file.flush()
    seekable = file.seekable()
    if seekable:
        # Drops the read-ahead buffer of the Python file object and leaves the
        # descriptor at the position the Python file object is logically at.
        file.seek(file.tell())

    try:
        yield file.fileno()
    finally:
        if seekable:
            file.seek(os.lseek(file.fileno(), 0, os.SEEK_CUR))
