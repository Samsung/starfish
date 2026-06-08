// Bridge between WPT's testharness.js and the Starfish test shell.
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
(function () {
    function emit(tests, status) {
        for (var i = 0; i < tests.length; i++) {
            var t = tests[i];
            console.log('WPTR ' + (t.status === 0 ? 'PASS' : 'FAIL') + ' ' + t.name);
        }
        console.log('WPTR DONE status=' + status.status + ' count=' + tests.length);
        if (typeof wptTestEnd === 'function') {
            wptTestEnd();
        } else if (typeof testEnd === 'function') {
            testEnd();
        }
    }
    function register() {
        if (typeof add_completion_callback !== 'function') {
            return false;
        }
        add_completion_callback(emit);
        return true;
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
