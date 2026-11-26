#!/usr/bin/env python3

"""Execute and assert one generated case.

Cases check that a CDP response carries each required result field.
Requests normally have no parameters. Commands with a semantic parameter
requirement can use a fixture to supply a valid value.
"""

from . import launcher
from .client import call, connect, debugger_url


class ProtocolError(AssertionError):
    """A CDP command returned an error response."""

    def __init__(self, method, error):
        self.code = error.get("code") if isinstance(error, dict) else None
        super().__init__("%s returned %s" % (method, error))


def label(case):
    return case["method"]


def fixture_node_id(socket):
    document = call(socket, 1, "DOM.getDocument", {"depth": 0})
    check_response("DOM.getDocument", document, ("root",))
    root_id = document["result"]["root"]["nodeId"]
    selected = call(socket, 2, "DOM.querySelector",
                    {"nodeId": root_id, "selector": "#schema-geometry"})
    check_response("DOM.querySelector", selected, ("nodeId",))
    return selected["result"]["nodeId"]


def run(starfish, case, timeout):
    """Start Starfish, send the request, then always stop the process."""
    with launcher.running(starfish, timeout) as endpoint:
        socket = connect(debugger_url(endpoint), timeout)
        try:
            method = case["method"]
            if method in ("DOM.describeNode", "DOM.getOuterHTML",
                          "DOM.resolveNode", "DOM.getBoxModel",
                          "DOM.getContentQuads"):
                node_id = fixture_node_id(socket)
                params = {"nodeId": node_id}
                request_id = 3
                if method in ("DOM.getBoxModel", "DOM.getContentQuads"):
                    described = call(socket, request_id, "DOM.describeNode", params)
                    check_response("DOM.describeNode", described, ("node",))
                    params = {"backendNodeId": described["result"]["node"]["backendNodeId"]}
                    request_id += 1
                response = call(socket, request_id, method, params)
            else:
                response = call(socket, 1, method)
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
        raise ProtocolError(method, response["error"])
    result = response.get("result", {})
    for key in result_keys:
        if key not in result:
            raise AssertionError("%s did not return %s" % (method, key))
