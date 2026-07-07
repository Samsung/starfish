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
import time
from subprocess import Popen, TimeoutExpired

class WorkerRunner:
    def __init__(self, target_name):
        self.target_name = target_name
        self.working_directory = os.path.dirname(os.path.abspath(__file__)) + "/../"
        self.worker = None

    def run(self):
        print("run " + self.target_name)
        self.worker = Popen(
            ["./" + self.target_name],
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
                    pass
        self.worker = None
