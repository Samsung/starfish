log("5");
if (window.testRunner)
    testRunner.notifyDone();

try {
    wptTestEnd();
} catch(e) {
    console.log(e);
}
