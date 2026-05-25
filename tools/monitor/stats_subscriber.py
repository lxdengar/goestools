#!/usr/bin/env python3
"""Prototype subscriber for goesrecv JSON stats streams.

This script intentionally stays outside the C++/CMake build. It uses ctypes to
call libnanomsg directly so it can read the same PUB/SUB stats endpoints used by
goesrecv and the built-in monitor.
"""

import argparse
import ctypes
import ctypes.util
import json
import sys
from datetime import datetime, timezone


AF_SP = 1
NN_SUB = 33
NN_SUB_SUBSCRIBE = 1
NN_MSG = ctypes.c_size_t(-1).value


class NanomsgError(RuntimeError):
    pass


class NanomsgSubscriber:
    def __init__(self, endpoint):
        self._lib = self._load_library()
        self._configure_signatures()
        self._fd = self._lib.nn_socket(AF_SP, NN_SUB)
        if self._fd < 0:
            raise NanomsgError(self._last_error("nn_socket"))

        try:
            rv = self._lib.nn_connect(self._fd, endpoint.encode("utf-8"))
            if rv < 0:
                raise NanomsgError(self._last_error("nn_connect", endpoint))

            empty_topic = ctypes.c_char_p(b"")
            rv = self._lib.nn_setsockopt(
                self._fd,
                NN_SUB,
                NN_SUB_SUBSCRIBE,
                empty_topic,
                0,
            )
            if rv < 0:
                raise NanomsgError(self._last_error("nn_setsockopt", endpoint))
        except Exception:
            self.close()
            raise

    @staticmethod
    def _load_library():
        path = ctypes.util.find_library("nanomsg")
        if not path:
            raise NanomsgError("Unable to find libnanomsg. Install nanomsg first.")
        return ctypes.CDLL(path)

    def _configure_signatures(self):
        self._lib.nn_socket.argtypes = [ctypes.c_int, ctypes.c_int]
        self._lib.nn_socket.restype = ctypes.c_int
        self._lib.nn_connect.argtypes = [ctypes.c_int, ctypes.c_char_p]
        self._lib.nn_connect.restype = ctypes.c_int
        self._lib.nn_setsockopt.argtypes = [
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_void_p,
            ctypes.c_size_t,
        ]
        self._lib.nn_setsockopt.restype = ctypes.c_int
        self._lib.nn_recv.argtypes = [
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_void_p),
            ctypes.c_size_t,
            ctypes.c_int,
        ]
        self._lib.nn_recv.restype = ctypes.c_int
        self._lib.nn_freemsg.argtypes = [ctypes.c_void_p]
        self._lib.nn_freemsg.restype = ctypes.c_int
        self._lib.nn_close.argtypes = [ctypes.c_int]
        self._lib.nn_close.restype = ctypes.c_int
        self._lib.nn_errno.argtypes = []
        self._lib.nn_errno.restype = ctypes.c_int
        self._lib.nn_strerror.argtypes = [ctypes.c_int]
        self._lib.nn_strerror.restype = ctypes.c_char_p

    def _last_error(self, operation, endpoint=None):
        errno = self._lib.nn_errno()
        message = self._lib.nn_strerror(errno).decode("utf-8", errors="replace")
        if endpoint:
            return f"{operation}: {message} ({endpoint})"
        return f"{operation}: {message}"

    def recv(self):
        msg = ctypes.c_void_p()
        nbytes = self._lib.nn_recv(self._fd, ctypes.byref(msg), NN_MSG, 0)
        if nbytes < 0:
            raise NanomsgError(self._last_error("nn_recv"))

        try:
            return ctypes.string_at(msg, nbytes).decode("utf-8")
        finally:
            self._lib.nn_freemsg(msg)

    def close(self):
        if getattr(self, "_fd", -1) >= 0:
            self._lib.nn_close(self._fd)
            self._fd = -1

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, tb):
        self.close()


def parse_args(argv):
    parser = argparse.ArgumentParser(
        description="Subscribe to a goesrecv JSON stats endpoint."
    )
    parser.add_argument(
        "endpoint",
        nargs="?",
        default="tcp://127.0.0.1:6001",
        help="nanomsg endpoint to connect to (default: tcp://127.0.0.1:6001)",
    )
    parser.add_argument(
        "--source",
        default="stats",
        help="label to include with printed messages (for example demodulator or decoder)",
    )
    parser.add_argument(
        "--pretty",
        action="store_true",
        help="pretty-print parsed JSON instead of one compact line per message",
    )
    return parser.parse_args(argv)


def print_message(source, payload, pretty):
    now = datetime.now(timezone.utc).isoformat(timespec="seconds")
    if pretty:
        print(f"[{now}] {source}")
        print(json.dumps(payload, indent=2, sort_keys=True))
        sys.stdout.flush()
        return

    print(f"{now} {source} {json.dumps(payload, sort_keys=True)}", flush=True)


def main(argv=None):
    args = parse_args(argv or sys.argv[1:])

    try:
        with NanomsgSubscriber(args.endpoint) as subscriber:
            print(
                f"connected to {args.endpoint}; waiting for {args.source} stats",
                file=sys.stderr,
                flush=True,
            )
            while True:
                raw = subscriber.recv()
                try:
                    payload = json.loads(raw)
                except json.JSONDecodeError as exc:
                    print(f"invalid JSON: {exc}: {raw!r}", file=sys.stderr)
                    continue
                print_message(args.source, payload, args.pretty)
    except KeyboardInterrupt:
        return 0
    except NanomsgError as exc:
        print(f"stats_subscriber.py: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
