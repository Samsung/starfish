/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#ifdef OS_POSIX
#include <unistd.h>
#include <spawn.h>
#include <wait.h>
#endif

#include "platform/process/base/Process.h"

namespace Starfish {

PID ProcessUtil::getCurrentProcId()
{
    return getpid();
}

bool ProcessUtil::launchProcess(const std::vector<std::string>& argv,
                                PID* processID)
{
    std::vector<char*> argvExec;
    argvExec.reserve(argv.size() + 1);
    for (const auto& arg : argv) {
        argvExec.push_back(const_cast<char*>(arg.c_str()));
    }
    argvExec.push_back(nullptr);

    // TODO: fill up file_actionsp, attrp, and envp if needed.
    posix_spawnattr_t attr;
    posix_spawnattr_t* attrp = nullptr;
    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_t* file_actionsp = nullptr;
    char** envp = nullptr;

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP | SA_SIGINFO;
    sa.sa_sigaction = [](int sig, siginfo_t* sip, void* ucp) {
        STARFISH_LOG_INFO("signal (%s) is sent from process %d\n",
                          strsignal(sig), sip->si_pid);
        while (waitpid(sip->si_pid, 0, WNOHANG) > 0) {
            // NOTE: we use non-blocking call to be sure this signal handler
            // will not block if a child was cleaned up in another part of the
            // program.
        }
    };

    if (sigaction(SIGCHLD, &sa, 0) == -1) {
        return false;
    }

    PID pid = -1;

    if (posix_spawnp(&pid, argvExec[0], file_actionsp, attrp, &argvExec[0],
                     envp) != 0) {
        return false;
    }

    if (attrp != nullptr) {
        if (posix_spawnattr_destroy(attrp) != 0) {
            return false;
        }
    }

    if (file_actionsp != nullptr) {
        if (posix_spawn_file_actions_destroy(file_actionsp) != 0) {
            return false;
        }
    }

    STARFISH_LOG_INFO("process %d created a child process %d\n",
                      getCurrentProcId(), pid);

    *processID = pid;

    return true;
}

bool ProcessUtil::launchProcessOnDoubleFork(
    const std::vector<std::string>& argv, PID* processID)
{
    // NOTE: 1.6.2 How do I prevent them from occuring? in
    // http://www.faqs.org/faqs/unix-faq/programmer/faq/
    int status = 0;

    PID pid = fork();

    if (pid < 0) {
        return false;
    }

    if (pid > 0) {
        // Parent process
        STARFISH_LOG_INFO("process %d created a child process %d\n",
                          getCurrentProcId(), pid);

        *processID = pid;
        waitpid(pid, &status, 0);
    } else {
        // Child process
        if (setsid() == -1) {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }

        PID grandChild_pid = fork();

        if (grandChild_pid < 0) {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }

        if (grandChild_pid > 0) {
            STARFISH_LOG_INFO("process %d created a child process %d\n",
                              getCurrentProcId(), grandChild_pid);
            _exit(EXIT_SUCCESS);
        } else {
            // Grandchild process
            std::vector<char*> argvExec;
            argvExec.reserve(argv.size() + 1);
            for (const auto& arg : argv) {
                argvExec.push_back(const_cast<char*>(arg.c_str()));
            }
            argvExec.push_back(nullptr);

            execvp(argvExec[0], &argvExec[0]);
        }
    }

    return true;
}

bool ProcessUtil::killProcess(PID pid, bool isWait)
{
    bool result = (kill(pid, SIGTERM) == 0);

    // ESRCH: No such process
    if (!result && (errno == ESRCH)) {
        return true;
    }

    if (result && isWait) {
        // NOTE: create a concrete waiting strategy if needed
        int approximateWaitingSecond = 30;
        int killedPid = -1;
        bool isKilled = false;

        do {
            killedPid = waitpid(pid, 0, WNOHANG);
            // ECHILD: No child processes
            if (killedPid == pid || errno == ECHILD) {
                isKilled = true;
                break;
            }
            sleep(1);
            approximateWaitingSecond--;
        } while (approximateWaitingSecond <= 0);

        if (!isKilled) {
            result = (kill(pid, SIGKILL) == 0);
        }
    }

    return result;
}

} // namespace Starfish

#endif // #ifdef STARFISH_ENABLE_SERVICE_WORKER
