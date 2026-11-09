// Bridge between Gecko's mochitest harness and the Starfish test shell.
//
// tool/runner/http_server.py prepends this file to the SimpleTest.js it
// serves out of the third_party/gecko submodule, so no upstream file is
// modified. SimpleTest.js expects two things only Firefox has: the dump()
// global, through which it prints one TEST-* line per check when no
// TestRunner frame hosts the test, and SpecialPowers, the privileged test
// API it calls while loading and while finishing. This file provides
// dump() and the SpecialPowers members SimpleTest.js itself calls; a test
// that needs any other privileged API fails, as it would in a browser
// without it.
//
// Output contract (parsed by tool/drivers/tests/gecko_test.py): SimpleTest's
// TEST-PASS / TEST-UNEXPECTED-FAIL / TEST-KNOWN-FAIL / ... lines on stdout,
// then wptTestEnd()'s "wptTestEnd() called" line, which ends the run.
// SimpleTest's last step, after every cleanup, is flushPrefEnv's callback
// showing the report, so that is where the run ends.
(function () {
    if (window.SpecialPowers) {
        return;
    }

    // SimpleTest.js reads opener to tell a popup from a top-level test.
    // Starfish is a single-window engine and leaves Window.opener
    // unimplemented (docs/Spec.md), so give it what a window nobody opened
    // has.
    if (!("opener" in window)) {
        window.opener = null;
    }

    window.dump = function (text) {
        console.log(String(text).replace(/\n$/, ""));
    };

    function done() {
        if (typeof wptTestEnd === "function") {
            wptTestEnd();
        }
    }

    function nothing() {}

    window.SpecialPowers = {
        // SimpleTest.js load
        setAsDefaultAssertHandler: nothing,
        Services: {
            scriptloader: { loadSubScript: nothing },
            sysinfo: { getProperty: function () { return ""; } }
        },
        // checks and error logging
        assertionCount: function () { return 0; },
        wrap: function (object) { return object; },
        Ci: { nsIException: function () {} },
        Cu: {
            getGlobalForObject: function () { return window; },
            isRemoteProxy: function () { return false; }
        },
        executeSoon: function (callback) { setTimeout(callback, 0); },
        registerConsoleListener: nothing,
        postConsoleSentinel: nothing,
        // SimpleTest.finish()
        DOMWindowUtils: {
            isTestControllingRefreshes: false,
            restoreNormalRefresh: nothing,
            enterChaosMode: nothing,
            leaveChaosMode: nothing
        },
        removeFiles: nothing,
        registeredServiceWorkers: function () { return Promise.resolve([]); },
        removeAllServiceWorkerData: function () { return Promise.resolve(); },
        cleanupAllClipboard: nothing,
        flushPermissions: function (callback) { callback(); },
        flushPrefEnv: function (callback) {
            callback();
            setTimeout(done, 0);
        },
        notifyObservers: nothing
    };
})();
