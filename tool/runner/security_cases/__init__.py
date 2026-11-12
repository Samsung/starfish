"""Shared contracts and discovery for browser security regressions."""

import contextlib
from dataclasses import dataclass, field, replace
import fnmatch
import functools
import http.server
import importlib
from pathlib import Path
import pkgutil
import threading
from typing import Callable, Mapping
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parents[3]
TESTS = ROOT / "tests"
RESULT_PREFIX = "PRODUCT_SECURITY:"


@dataclass(frozen=True)
class Case:
    name: str
    url: str
    required_output: tuple = ()
    forbidden_output: tuple = ()
    timeout: int = None
    internal: bool = True
    validate: Callable = None


@dataclass(frozen=True)
class Suite:
    name: str
    description: str
    cases: Callable
    features: Mapping = field(default_factory=dict)


@dataclass
class Context:
    base_url: str
    features: Mapping = field(default_factory=dict)
    tests_root: Path = TESTS
    routes: dict = field(default_factory=dict)
    fixtures: contextlib.ExitStack = field(default_factory=contextlib.ExitStack)

    def file(self, path):
        return (self.tests_root / path).resolve().as_uri()

    def http(self, path):
        return self.base_url + "/" + path.lstrip("/")

    def read(self, path):
        return (self.tests_root / path).read_text()

    def add_route(self, path, callback, methods=("GET",)):
        keys = [(method.upper(), path) for method in methods]
        if any(key in self.routes for key in keys):
            raise ValueError("duplicate security fixture route: " + path)
        self.routes.update({key: callback for key in keys})


def discover_suites():
    """A module in this package exports SUITE; the runner needs no edits."""
    suites = {}
    for item in sorted(pkgutil.iter_modules(__path__), key=lambda item: item.name):
        if item.name.startswith("_"):
            continue
        module = importlib.import_module(__name__ + "." + item.name)
        suite = getattr(module, "SUITE", None)
        if not isinstance(suite, Suite):
            raise ValueError(item.name + " must export a Suite as SUITE")
        if not suite.name or "/" in suite.name or suite.name in suites:
            raise ValueError("invalid or duplicate security group: " + suite.name)
        suites[suite.name] = suite
    return suites


def configure_features(suites, overrides=()):
    features = {}
    for suite in suites.values():
        for name, default in suite.features.items():
            if name in features and features[name] != default:
                raise ValueError("conflicting feature default: " + name)
            features[name] = default
    for setting in overrides:
        name, separator, value = setting.partition("=")
        if not separator or not value or name not in features:
            raise ValueError("unknown or invalid feature setting: " + setting)
        features[name] = value
    return features


def collect_cases(suites, context, groups=(), patterns=(), internal_only=False):
    unknown = set(groups) - suites.keys()
    if unknown:
        raise ValueError("unknown security group: " + ", ".join(sorted(unknown)))
    collected = []
    names = set()
    for name, suite in suites.items():
        if groups and name not in groups:
            continue
        for case in suite.cases(context):
            if not case.name:
                raise ValueError("empty security case name in " + name)
            case = replace(case, name=name + "/" + case.name)
            if case.name in names:
                raise ValueError("duplicate security case: " + case.name)
            if case.timeout is not None and case.timeout < 1:
                raise ValueError("invalid case timeout: " + case.name)
            names.add(case.name)
            if not internal_only or case.internal:
                collected.append(case)
    for pattern in patterns:
        if not any(fnmatch.fnmatchcase(case.name, pattern) for case in collected):
            raise ValueError("case pattern matched nothing: " + pattern)
    if patterns:
        collected = [case for case in collected
                     if any(fnmatch.fnmatchcase(case.name, pattern)
                            for pattern in patterns)]
    if not collected:
        raise ValueError("no security cases selected")
    return collected


@contextlib.contextmanager
def server(features):
    context = Context("", features)

    class Handler(http.server.SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass

        def dispatch(self):
            callback = context.routes.get((self.command, urlsplit(self.path).path))
            if callback:
                callback(self)
            elif self.command in ("GET", "HEAD"):
                getattr(super(), "do_" + self.command)()
            else:
                self.send_error(501, "No fixture for this request method")

        do_GET = do_HEAD = do_POST = do_PUT = dispatch
        do_PATCH = do_DELETE = do_OPTIONS = dispatch

    handler = functools.partial(Handler, directory=str(context.tests_root))
    with http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler) as httpd:
        context.base_url = "http://127.0.0.1:%d" % httpd.server_port
        thread = threading.Thread(target=httpd.serve_forever, daemon=True)
        thread.start()
        try:
            with context.fixtures:
                yield context
        finally:
            httpd.shutdown()
            thread.join()
