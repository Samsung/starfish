# Copyright (c) 2026-present Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Repo-root anchor for the scripts under tool/.

Scripts here live at varying depths below the repo root, so each one deriving
the root by counting its own ".." hops means every move silently rewrites
paths that only fail much later (a missing ./Starfish, a tool/tool/imgdiff).
This module is the one place that count lives, and tool/ itself never moves.

A script reaches it by putting tool/ on sys.path and importing REPO_ROOT; if
that bootstrap hop is ever wrong the import fails immediately at startup,
rather than the script running on paths that point nowhere.
"""

import os

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
