// The upstream test for this event, accessibility/accessibility-getRootNode.js,
// navigates to a file served over HTTP, which this harness has no server for.
// This one uses a data: URL so the event itself can still be checked.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp} = await testRunner.startBlank(
      'Tests that Accessibility.loadComplete reports the new root');
  await dp.Accessibility.enable();

  const loadComplete = dp.Accessibility.onceLoadComplete();
  await dp.Page.navigate(
      {url: 'data:text/html,<title>Page title</title><h1>Heading</h1>'});
  const {params} = await loadComplete;

  const root = params.root;
  testRunner.log(`role: ${root.role.value}`);
  testRunner.log(`name: ${root.name.value}`);
  testRunner.log(`ignored: ${root.ignored}`);
  testRunner.log(`has a frameId: ${typeof root.frameId === 'string'}`);
  testRunner.log(`has no parent: ${root.parentId === undefined}`);
  testRunner.log(`has children: ${root.childIds.length > 0}`);

  // The event resets what the client is known to hold, so the root it just
  // reported has to be addressable by getChildAXNodes.
  const children = await dp.Accessibility.getChildAXNodes({id: root.nodeId});
  testRunner.log(
      `getChildAXNodes on the reported root works: ` +
      `${children.result.nodes.length > 0}`);

  testRunner.completeTest();
});
