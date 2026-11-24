// Lists the event listeners of a node with DOMDebugger.getEventListeners,
// with the capture flag each was registered with.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp} = await testRunner.startHTML(`
      <button id="b">b</button>
      <script>
        const b = document.getElementById('b');
        b.addEventListener('click', function onClick() {});
        b.addEventListener('keydown', () => {}, true);
      </script>`,
      'Tests DOMDebugger.getEventListeners');
  const objectOf = async expression =>
      (await dp.Runtime.evaluate({expression})).result.result.objectId;

  const {listeners} = (await dp.DOMDebugger.getEventListeners(
      {objectId: await objectOf('b')})).result;
  listeners.sort((x, y) => x.type.localeCompare(y.type));
  for (const l of listeners)
    testRunner.log(`${l.type}: useCapture=${l.useCapture}`);

  const onWindow = (await dp.DOMDebugger.getEventListeners(
      {objectId: await objectOf('window')})).result.listeners;
  testRunner.log(
      `window has none of them: ${onWindow.every(l => l.type !== 'click')}`);
  testRunner.completeTest();
});
