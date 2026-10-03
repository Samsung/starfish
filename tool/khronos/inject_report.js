// Bridge between the Khronos WebGL conformance harness and the Starfish test
// shell.
//
// tool/runner/http_server.py prepends this file to the harness scripts it
// serves out of the third_party/webgl submodule (js-test-pre.js and
// more/unit.js -- the two harnesses the suite uses), so the upstream files
// are never modified. Both harnesses report every assertion through
// window.parent.webglTestHarness, the hook Khronos's own iframe-based runner
// (webgl-conformance-tests.html) plugs into; a test page loaded at top level
// has window.parent === window, so defining that object here, before the
// harness script runs, receives the same callbacks with no iframe involved.
// It must come first: the sdk harness caches window.parent.webglTestHarness
// in a const at load time.
//
// Output contract (parsed by tool/drivers/basics/starfish_basic_test.py,
// which counts PASS/FAIL words in stdout):
//   FAIL <assertion message>                      one line per failed assertion
//   KHRONOS DONE pass=<n> fail=<n> skipped=<n>    once, on notifyFinished()
//   [PASS] / [FAIL]                               printed natively by testEnd()
// Passing assertions are not echoed (some tests make 100k+ of them); the
// verdict is testEnd(true) only when at least one assertion passed and none
// failed, so a page whose harness never reports anything cannot pass. A
// skipped assertion (reportSkippedTestResultsToHarness, used for cases the
// platform legitimately doesn't support) arrives with success=true and is
// counted as a pass, as Khronos's own runner does; it is tallied separately
// only so the summary line shows it.
(function () {
    if (window.__starfishKhronosLoaded) {
        return;
    }
    window.__starfishKhronosLoaded = true;

    var pass = 0;
    var fail = 0;
    var skipped = 0;
    window.webglTestHarness = {
        reportResults: function (url, success, msg, isSkipped) {
            if (success) {
                pass++;
                if (isSkipped) {
                    skipped++;
                }
            } else {
                fail++;
                console.log('FAIL ' + msg);
            }
        },
        notifyFinished: function (url) {
            console.log('KHRONOS DONE pass=' + pass + ' fail=' + fail +
                        ' skipped=' + skipped);
            if (typeof testEnd === 'function') {
                testEnd(fail === 0 && pass > 0);
            }
        }
    };
})();
