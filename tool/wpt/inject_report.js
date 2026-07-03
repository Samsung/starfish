// Bridge between WPT's testharness.js/crashtest pages and the Starfish test
// shell.
//
// Injected into every served page via `wpt serve --inject-script`. On test
// completion it prints one machine-parseable line per subtest plus a summary,
// then terminates the shell through the engine's existing wptTestEnd() hook
// (which exits when HIDE_WINDOW is set). The runner parses the `WPTR ` lines.
//
// Output contract (each prefixed with `WPTR ` inside a console.log):
//   WPTR PASS <subtest name>
//   WPTR FAIL <subtest name>
//   WPTR DONE status=<0=OK|1=ERROR|2=TIMEOUT|3=PRECONDITION_FAILED> count=<n>
//   WPTR CRASHOK                                    (crashtest path only)
(function () {
    function finish() {
        if (typeof wptTestEnd === 'function') {
            wptTestEnd();
        } else if (typeof testEnd === 'function') {
            testEnd();
        }
    }
    function emit(tests, status) {
        for (var i = 0; i < tests.length; i++) {
            var t = tests[i];
            console.log('WPTR ' + (t.status === 0 ? 'PASS' : 'FAIL') + ' ' + t.name);
        }
        console.log('WPTR DONE status=' + status.status + ' count=' + tests.length);
        finish();
    }
    function register() {
        if (typeof add_completion_callback !== 'function') {
            return false;
        }
        add_completion_callback(emit);
        return true;
    }

    // Crashtests have no testharness.js and no completion contract of their
    // own -- the WPT pass condition is simply "the browser didn't crash
    // rendering this page". tool/wpt_runner.py's crashtest mode appends this
    // query marker so the two paths never both fire on the same page (e.g. a
    // page that happens to also load testharness.js for unrelated reasons).
    // The literal must match wpt_runner.py's CRASHTEST_QUERY
    // ("__starfish_crashtest=1") -- keep the two in sync (no shared constant
    // across Python/JS).
    if (/(^|[?&])__starfish_crashtest=1(&|$)/.test(location.search)) {
        var crashtestDone = function () {
            console.log('WPTR CRASHOK');
            finish();
        };
        // A wait page holds completion open by marking its root element with a
        // wait class, removed once its async work settles. Crashtests mostly
        // use `test-wait`, but some in the corpus use `reftest-wait` (both are
        // recognized by upstream wptrunner's own executors); missing the
        // latter would fire CRASHOK at load, before the crash-inducing async
        // work runs -- a false PASS. Check both.
        var isWaiting = function (root) {
            return root && (root.classList.contains('test-wait') ||
                            root.classList.contains('reftest-wait'));
        };
        var waitForTestWait = function () {
            // A crashtest may remove documentElement itself as the thing under
            // test (e.g. dom/nodes/crashtests/documentElement-remove-*.html,
            // css-page root-element-remove-*.html). If it's gone there is no
            // wait class to clear and nothing to observe, and the engine
            // already rendered the page to onload without crashing -- so the
            // pass condition is met. Guarding null also avoids a `null.classList`
            // TypeError that would otherwise throw here and silently hang the
            // run (a false TIMEOUT for a page the engine actually handled).
            var root = document.documentElement;
            if (!isWaiting(root)) {
                crashtestDone();
                return;
            }
            var mo = new MutationObserver(function () {
                if (!isWaiting(root)) {
                    mo.disconnect();
                    crashtestDone();
                }
            });
            mo.observe(root, {attributes: true, attributeFilter: ['class']});
        };
        if (document.readyState === 'complete') {
            waitForTestWait();
        } else {
            window.addEventListener('load', waitForTestWait);
        }
        return;
    }

    // The injected script runs before testharness.js defines
    // add_completion_callback, so poll briefly until it is available.
    if (!register()) {
        var tries = 0;
        var iv = setInterval(function () {
            if (register() || ++tries > 1000) {
                clearInterval(iv);
            }
        }, 5);
    }
})();
