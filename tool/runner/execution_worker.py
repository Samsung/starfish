# Copyright (c) 2024-present Samsung Electronics Co., Ltd.
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

import os
import signal
import sys
import time
from subprocess import Popen, TimeoutExpired

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from repo_paths import REPO_ROOT  # noqa: E402

class WorkerRunner:
    def __init__(self, target_name):
        self.target_name = target_name
        self.working_directory = REPO_ROOT
        self.worker = None

    def run(self, data_dir=None):
        """data_dir: passed through as --data-dir=<data_dir> (the daemon
        binary's own flag, src/launcher/SharedWorkerEntry.cpp /
        ServiceWorkerEntry.cpp -- distinct from Starfish's --storage-dir=).
        None (default) leaves the daemon on its own default
        $HOME/Starfish-storage. Callers that also give client Starfish
        invocations their own --storage-dir= MUST pass the same directory
        here, or the daemon and its clients disagree on where the
        SharedWorker/ServiceWorker IPC socket lives (WorkerIPCAddress
        derives it from this same directory) and every test that actually
        round-trips through the daemon times out / fails.
        """
        print("run " + self.target_name)
        cmd = ["./" + self.target_name]
        if data_dir:
            cmd.append("--data-dir=" + data_dir)
        self.worker = Popen(
            cmd,
            cwd=self.working_directory,
            start_new_session=True,
        )
        time.sleep(1)

    def terminate(self):
        if not self.worker:
            return
        print("terminate " + self.target_name)
        if self.worker.poll() is None:
            try:
                os.killpg(os.getpgid(self.worker.pid), signal.SIGTERM)
            except OSError:
                pass
            try:
                self.worker.wait(timeout=10)
            except TimeoutExpired:
                try:
                    os.killpg(os.getpgid(self.worker.pid), signal.SIGKILL)
                except OSError:
                    pass
                try:
                    self.worker.wait(timeout=5)
                except TimeoutExpired:
                    print("warning: " + self.target_name +
                          " still running after SIGKILL")
        self.worker = None
