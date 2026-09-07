#!/usr/bin/env python3

"""Execute and assert one generated case.

Every case is the same shape, because every case is generated from the
protocol: send one CDP request with no parameters and check that the response
carries each required result field. There is no other case type, since
nothing in the protocol says how to satisfy a required parameter.
"""

from . import launcher
from .client import call, connect, debugger_url


def label(case):
    return case["method"]


def run(starfish, case, timeout):
    """Start Starfish, send the request, then always stop the process."""
    with launcher.running(starfish, timeout) as endpoint:
        socket = connect(debugger_url(endpoint), timeout)
        try:
            response = call(socket, 1, case["method"])
        finally:
            socket.close()
    check_response(case["method"], response, case["result_keys"])


def check_response(method, response, result_keys):
    """A response counts only if it carries every required field.

    An empty success is not support: Starfish acks some unimplemented methods
    so that client handshakes can continue. A command that declares no
    required field can therefore never be judged, which is why the list holds
    only commands that declare one.
    """
    if "error" in response:
        raise AssertionError("%s returned %s" % (method, response["error"]))
    result = response.get("result", {})
    for key in result_keys:
        if key not in result:
            raise AssertionError("%s did not return %s" % (method, key))
