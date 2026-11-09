// Chromium reports <input type="number"> as a spinbutton with a
// number-typed AXValue. Every other control keeps the string form.
// The test checks both kinds of control.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp} = await testRunner.startBlank(
      'Tests that a number input reports a number-typed AX value');
  await dp.Accessibility.enable();

  const loadComplete = dp.Accessibility.onceLoadComplete();
  await dp.Page.navigate({
      url: 'data:text/html,<input type="number" value="5" aria-label="qty">'
           + '<input type="text" value="abc" aria-label="note">'});
  await loadComplete;

  const {result} = await dp.Accessibility.getFullAXTree();
  const nodes = {};
  for (const node of result.nodes) {
    if (node.name && node.name.value) {
      nodes[node.name.value] ??= node;
    }
  }
  for (const [label, name] of [['number input', 'qty'], ['text input', 'note']]) {
    const node = nodes[name];
    testRunner.log(
        `${label}: ${node.role.value} ${JSON.stringify(node.value)}`);
  }

  testRunner.completeTest();
});
