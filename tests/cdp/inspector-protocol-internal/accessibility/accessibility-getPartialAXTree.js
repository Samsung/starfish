(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp} = await testRunner.startHTML(`
    <main>
      <button id="target">Button name</button>
      <div id="hidden" hidden>Hidden text</div>
    </main>
  `, 'Tests Accessibility.getPartialAXTree selectors and relatives');

  const documentResponse = await dp.DOM.getDocument();
  const documentId = documentResponse.result.root.nodeId;
  const targetResponse =
      await dp.DOM.querySelector({nodeId: documentId, selector: '#target'});
  const targetId = targetResponse.result.nodeId;
  const targetDescription = await dp.DOM.describeNode({nodeId: targetId});
  const targetBackendId = targetDescription.result.node.backendNodeId;
  const targetObject = await dp.DOM.resolveNode({nodeId: targetId});
  const targetObjectId = targetObject.result.object.objectId;

  async function testSelector(label, selector) {
    const response = await dp.Accessibility.getPartialAXTree(
        {...selector, fetchRelatives: false});
    const nodes = response.result.nodes;
    testRunner.log(
        `${label} selects only the requested node: ` +
        `${nodes.length === 1 &&
           nodes[0].backendDOMNodeId === targetBackendId}`);
  }

  await testSelector('nodeId', {nodeId: targetId});
  await testSelector('backendNodeId', {backendNodeId: targetBackendId});
  await testSelector('objectId', {objectId: targetObjectId});

  const relatives =
      await dp.Accessibility.getPartialAXTree({nodeId: targetId});
  const relativeNodes = relatives.result.nodes;
  testRunner.log(
      `requested node is first: ` +
      `${relativeNodes[0].backendDOMNodeId === targetBackendId}`);
  testRunner.log(
      `default fetch includes children and ancestors: ` +
      `${relativeNodes.length > 1 &&
         relativeNodes.some(node => node.role?.value === 'RootWebArea')}`);

  const hiddenResponse =
      await dp.DOM.querySelector({nodeId: documentId, selector: '#hidden'});
  const hiddenTree = await dp.Accessibility.getPartialAXTree(
      {nodeId: hiddenResponse.result.nodeId, fetchRelatives: false});
  const hiddenNode = hiddenTree.result.nodes[0];
  testRunner.log(
      `node without an AX object is returned as ignored: ` +
      `${hiddenTree.result.nodes.length === 1 && hiddenNode.ignored &&
         hiddenNode.ignoredReasons?.[0].name === 'notRendered'}`);

  const missingNode = await dp.Accessibility.getPartialAXTree({});
  testRunner.log(
      `missing selector returns an error: ${Boolean(missingNode.error)}`);

  testRunner.completeTest();
});
