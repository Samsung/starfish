#!/usr/bin/env python3

"""Build the command table from the pinned protocol definition.

The protocol is a specification, not a test. It states which commands exist,
which parameters they take, and which field names a response carries. It says
nothing about what any value should be. So the only thing derivable from it is
a name check: are the fields the protocol declares non-optional actually
present.

Reading it at startup rather than checking in a converted copy means a
checked-in expectation cannot drift from the protocol, be weaker than it, or
be edited by hand. Parsing the 1.6 MB costs a small fraction of one browser
start, and a run starts one browser per command, so caching it would save
nothing worth the risk of a stale copy.
"""

import json

PROTOCOL_FILES = ("browser_protocol.json", "js_protocol.json")


def load(protocol_dir):
    """Return {method: command} for every command in the pinned protocol."""
    commands = {}
    for name in PROTOCOL_FILES:
        document = json.loads((protocol_dir / name).read_text())
        if not isinstance(document.get("domains"), list):
            raise ValueError("%s has no domains list" % name)
        for domain in document["domains"]:
            for command in domain.get("commands", []):
                method = "%s.%s" % (domain["domain"], command["name"])
                commands[method] = {
                    "method": method,
                    # Field names the protocol declares non-optional. Empty
                    # for two thirds of CDP, which is imperative rather than
                    # query-shaped: enable, disable, setX, clearY return
                    # nothing, so nothing about them can be checked here.
                    "result_keys": [result["name"] for result
                                    in command.get("returns", [])
                                    if not result.get("optional", False)],
                    # Used to report a command as unreachable rather than
                    # merely failed.
                    "required_params": [parameter["name"] for parameter
                                        in command.get("parameters", [])
                                        if not parameter.get("optional", False)],
                    "experimental": command.get("experimental", False),
                    "deprecated": command.get("deprecated", False),
                }
    return commands


def checkable(commands):
    """Commands this layer can say anything about at all.

    A command with no required result field cannot be judged: its correct
    response is an empty object, which is what Starfish also returns for a
    method it does not implement.
    """
    return {method for method, command in commands.items()
            if command["result_keys"]}


def reachable(commands):
    """Commands this layer can actually send and then judge.

    A command with a required parameter cannot be sent at all, because the
    protocol says the parameter exists but not where its value comes from.
    Together with checkable() this leaves 85 of 661.

    Only these belong in the list. The rest are facts about the protocol, not
    about Starfish: they would never change, so recording them would leave a
    file where almost every line is noise. The full 661 picture stays in the
    coverage report, which is derived rather than stored.
    """
    return {method for method, command in commands.items()
            if command["result_keys"] and not command["required_params"]}


def revision(protocol_dir):
    return (protocol_dir / "revision.txt").read_text().strip()
