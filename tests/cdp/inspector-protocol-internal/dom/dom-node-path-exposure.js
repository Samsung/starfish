(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  for (const shallow of [false, true]) {
    const {dp} = await testRunner.startHTML(
        `<iframe srcdoc='<div id="inner">frame</div>'></iframe>`,
        shallow ? 'Restores a DOM path from a shallow document' :
                  'Restores a DOM path without requesting the document');
    await dp.DOM.enable();
    let rootId = 0;
    if (shallow) {
      const {result: {root}} = await dp.DOM.getDocument({depth: 0});
      rootId = root.nodeId;
    }

    // Reconstruct the reachable tree using only what the frontend received.
    const known = new Map([[rootId, null]]);
    function add(node) {
      known.set(node.nodeId, node);
      if (node.contentDocument)
        add(node.contentDocument);
      for (const child of node.children || [])
        add(child);
    }
    dp.DOM.onSetChildNodes(({params}) => {
      if (known.has(params.parentId)) {
        for (const node of params.nodes)
          add(node);
      }
    });
    const {result: {nodeId}} = await dp.DOM.pushNodeByPathToFrontend({
      path: '0,HTML,1,BODY,0,IFRAME,d,#document,0,HTML,1,BODY,0,DIV',
    });
    if (!shallow) {
      testRunner.log(`unrequested document has no frontend ID: ${nodeId === 0}`);
      continue;
    }
    const node = known.get(nodeId);
    const exposed = node?.localName === 'div' && node.attributes.join() === 'id,inner';
    testRunner.log(`shallow document exposes the returned node: ${Boolean(exposed)}`);
  }
  testRunner.completeTest();
});
