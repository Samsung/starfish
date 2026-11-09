// Bridge between WebKit's LayoutTests harnesses and the Starfish test shell.
//
// tool/runner/http_server.py prepends this file to the harness scripts it
// serves out of the third_party/webkit submodule (resources/js-test-pre.js,
// resources/js-test.js and the DOM conformance suites' selfhtml.js), and out
// of third_party/blink, whose web tests kept the same testRunner API
// (resources/js-test.js and resources/testharnessreport.js), so the
// upstream files are never modified. Those harnesses drive DumpRenderTree's
// window.testRunner: dumpAsText() asks for a text result, waitUntilDone()
// holds the dump until notifyDone(), and otherwise the page is dumped once
// it has loaded. The dump is the document's innerText, the same text an
// -expected.txt file holds.
//
// Output contract (parsed by tool/drivers/tests/webkit_test.py and
// blink_test.py): the dump on stdout, followed by wptTestEnd()'s
// "wptTestEnd() called" line, which ends the run. Only the testRunner calls
// the harnesses themselves make are provided; a test that needs any other
// one (eventSender, internals, mock geolocation, ...) fails, as it would in
// a browser without them.
(function () {
    if (window.testRunner) {
        return;
    }

    var waiting = false;
    var dumped = false;

    function dump() {
        if (dumped) {
            return;
        }
        dumped = true;
        var root = document.documentElement;
        console.log(root ? root.innerText : "");
        if (typeof wptTestEnd === "function") {
            wptTestEnd();
        }
    }

    window.testRunner = {
        dumpAsText: function () {},
        dumpAsTextWithPixelResults: function () {},
        dumpChildFramesAsText: function () {},
        // Blink's testharnessreport.js sets these up before it runs.
        setPopupBlockingEnabled: function () {},
        setDumpJavaScriptDialogs: function () {},
        setDumpConsoleMessages: function () {},
        overridePreference: function () {},
        waitUntilDone: function () {
            waiting = true;
        },
        // DumpRenderTree dumps once the current task has finished, so text a
        // test writes right after notifyDone() still belongs to its result.
        notifyDone: function () {
            setTimeout(dump, 0);
        }
    };

    window.addEventListener("load", function () {
        setTimeout(function () {
            if (!waiting) {
                dump();
            }
        }, 0);
    });
})();
