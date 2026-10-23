#!/usr/bin/env python3
# Copyright (c) 2026-present Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""URL selection for the pinned WPT server's default ports."""

from urllib.parse import parse_qs, urlsplit, urlunsplit

HOST = "web-platform.test"
HTTP_PORT = 8000
HTTPS_PORT = 8443
H2_PORT = 9000
ORIGINS = {"http": "http://%s:%d" % (HOST, HTTP_PORT),
           "https": "https://%s:%d" % (HOST, HTTPS_PORT),
           "h2": "https://%s:%d" % (HOST, H2_PORT)}


def server_protocol(url):
    """Match tools/manifest/item.py and wptrunner/wpttest.py in pinned WPT.

    ServiceWorker generated variants require TLS even without a .https.
    filename flag; h2 takes priority over the other flags.
    """
    parts = urlsplit(url)
    flags = (set(parts.path.rsplit("/", 1)[-1].split(".")[1:-1]) |
             set(parse_qs(parts.query).get("wpt_flags", [])))
    if "h2" in flags:
        return "h2"
    if flags & {"https", "serviceworker", "serviceworker-module"}:
        return "https"
    return "http"


def test_url(path, url_base="/"):
    return (ORIGINS[server_protocol(path)] + url_base.rstrip("/") +
            "/" + path.lstrip("/"))


def with_test_protocol(url):
    """Upgrade local WPT URLs requiring TLS, preserving host and variants.

    Explicit secure URLs and ordinary HTTP tests retain their origin. The
    alternate HTTP port maps to the alternate HTTPS port for legacy lists.
    """
    parts = urlsplit(url)
    host = parts.hostname or ""
    if not any(host == domain or host.endswith("." + domain)
               for domain in (HOST, "not-web-platform.test")):
        return url
    protocol = server_protocol(url)
    if protocol == "http":
        return url
    if protocol == "h2":
        port = H2_PORT
    elif parts.scheme == "https":
        return url
    else:
        port = {HTTP_PORT: HTTPS_PORT, HTTP_PORT + 1: HTTPS_PORT + 1}.get(parts.port)
        if port is None:
            raise ValueError("unknown WPT HTTP port: %s" % url)
    return urlunsplit(("https", "%s:%d" % (host, port), parts.path,
                       parts.query, parts.fragment))
