// Walks and edits the document through the DOM domain and checks each edit
// against what the page itself sees.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp, session} = await testRunner.startHTML(`
      <div id="list" class="c1 c2"><p id="a">A</p><p id="b">B</p></div>
      <input id="field">
      <div id="target" style="width: 40px; height: 20px">t</div>`,
      'Tests DOM domain node queries and edits');
  const page = expression => session.evaluate(expression);
  await dp.DOM.enable();
  const {result: {root}} = await dp.DOM.getDocument({depth: -1});
  const query = async selector => (await dp.DOM.querySelector(
      {nodeId: root.nodeId, selector})).result.nodeId;

  const {nodeIds} = (await dp.DOM.querySelectorAll(
      {nodeId: root.nodeId, selector: 'p'})).result;
  testRunner.log(`querySelectorAll p: ${nodeIds.length}`);

  const list = await query('#list');
  const {node} = (await dp.DOM.describeNode({nodeId: list})).result;
  testRunner.log(`describeNode: ${node.nodeName} children=${node.childNodeCount}`);
  const {attributes} = (await dp.DOM.getAttributes({nodeId: list})).result;
  testRunner.log(`attributes: ${attributes.join(' ')}`);
  const {classNames} = (await dp.DOM.collectClassNamesFromSubtree(
      {nodeId: list})).result;
  testRunner.log(`class names: ${classNames.sort().join(' ')}`);
  await dp.DOM.requestChildNodes({nodeId: list, depth: 1});

  const a = await query('#a');
  await dp.DOM.setAttributeValue({nodeId: a, name: 'title', value: 'tip'});
  testRunner.log(`setAttributeValue: ${await page('a.title')}`);
  await dp.DOM.removeAttribute({nodeId: a, name: 'title'});
  testRunner.log(`removeAttribute: ${await page('a.hasAttribute("title")')}`);
  const {outerHTML} = (await dp.DOM.getOuterHTML({nodeId: a})).result;
  testRunner.log(`getOuterHTML: ${outerHTML}`);
  await dp.DOM.setOuterHTML({nodeId: a, outerHTML: '<p id="a">A2</p>'});
  testRunner.log(`setOuterHTML: ${await page('document.getElementById("a").textContent')}`);

  const b = await query('#b');
  await dp.DOM.markUndoableState();
  const {result: {nodeId: copy}} = await dp.DOM.copyTo(
      {nodeId: b, targetNodeId: await query('#target')});
  testRunner.log(`copyTo: ${await page('target.querySelectorAll("#b").length')}`);
  await dp.DOM.moveTo({nodeId: copy, targetNodeId: list});
  testRunner.log(`moveTo: ${await page('list.children.length')}`);
  await dp.DOM.removeNode({nodeId: copy});
  testRunner.log(`removeNode: ${await page('list.children.length')}`);

  // getDocument({depth: -1}) pushed the whole tree, text nodes included.
  const find = (node, test) => test(node) ? node :
      (node.children || []).map(c => find(c, test)).find(Boolean);
  const bNode = find(root, n => (n.attributes || []).join() === 'id,b');
  await dp.DOM.setNodeValue({nodeId: bNode.children[0].nodeId, value: 'B2'});
  testRunner.log(`setNodeValue: ${await page('document.getElementById("b").textContent')}`);

  const target = await query('#target');
  const {model} = (await dp.DOM.getBoxModel({nodeId: target})).result;
  testRunner.log(`box model: ${model.width}x${model.height}`);
  const {quads} = (await dp.DOM.getContentQuads({nodeId: target})).result;
  testRunner.log(`content quads: ${quads.length} of ${quads[0].length} numbers`);
  const x = (quads[0][0] + quads[0][2]) / 2, y = (quads[0][1] + quads[0][5]) / 2;
  const hit = (await dp.DOM.getNodeForLocation({x: Math.round(x),
                                                y: Math.round(y)})).result;
  const {node: hitNode} =
      (await dp.DOM.describeNode({backendNodeId: hit.backendNodeId})).result;
  testRunner.log(`getNodeForLocation: ${hitNode.nodeName}`);
  await dp.DOM.scrollIntoViewIfNeeded({nodeId: target});

  const {object} = (await dp.DOM.resolveNode({nodeId: target})).result;
  testRunner.log(`resolveNode: ${object.className}`);
  const {nodeId: requested} =
      (await dp.DOM.requestNode({objectId: object.objectId})).result;
  testRunner.log(`requestNode round trip: ${requested === target}`);
  const {nodeIds: pushed} = (await dp.DOM.pushNodesByBackendIdsToFrontend(
      {backendNodeIds: [hit.backendNodeId]})).result;
  testRunner.log(`pushNodesByBackendIdsToFrontend: ${pushed.length}`);

  await dp.DOM.focus({nodeId: await query('#field')});
  testRunner.log(`focus: ${await page('document.activeElement.id')}`);

  const {searchId, resultCount} =
      (await dp.DOM.performSearch({query: 'target'})).result;
  testRunner.log(`performSearch results: ${resultCount > 0}`);
  if (resultCount > 0) {
    const {nodeIds: found} = (await dp.DOM.getSearchResults(
        {searchId, fromIndex: 0, toIndex: resultCount})).result;
    testRunner.log(`getSearchResults: ${found.length === resultCount}`);
  }
  await dp.DOM.discardSearchResults({searchId});

  const flat = (await dp.DOM.getFlattenedDocument({depth: -1})).result;
  testRunner.log(`getFlattenedDocument: ${flat.nodes.length > 5}`);

  const undo = await dp.DOM.undo();
  const redo = await dp.DOM.redo();
  testRunner.log(`undo/redo answer: ${!undo.error && !redo.error}`);
  await dp.DOM.disable();
  testRunner.completeTest();
});
