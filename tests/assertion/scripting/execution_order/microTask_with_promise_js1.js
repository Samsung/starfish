window.count = 0;
if (window['testAssert'] === undefined) {
    testAssert = console.log
    testEnd = console.log
}
testAssert(count == 0);
console.log(`#${count++} first script tag executed`);
queueMicrotask(() => {
    testAssert(count == 1);
    console.log(`#${count++} micro task executed in first script tag`);
});
