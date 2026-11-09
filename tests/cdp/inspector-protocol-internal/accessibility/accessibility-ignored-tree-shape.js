// Pins which ignored nodes stay in the accessibility tree. The upstream
// tests check the values of ignored nodes; this one checks the shape, which
// is what decides where a client finds a node's children.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp} = await testRunner.startHTML(
      `
      <div id="plain">text</div>
      <div id="presentational" role="presentation"><button>inside</button></div>
      <span id="wrapper"><button id="wrapped">wrapped</button></span>
    `,
      'Tests which ignored nodes stay in the accessibility tree');

  const {result} = await dp.Accessibility.getFullAXTree({});
  const byId = new Map(result.nodes.map(node => [node.nodeId, node]));

  function describe(node) {
    if (node.ignored) {
      return `${node.role.value} (ignored)`;
    }
    return node.name?.value ? `${node.role.value} "${node.name.value}"`
                            : node.role.value;
  }

  // The chain from the root down to the first node with a role of its own.
  let node = result.nodes[0];
  const chain = [];
  while (node) {
    chain.push(describe(node));
    const next = node.childIds.map(id => byId.get(id)).find(Boolean);
    if (!next || !next.ignored) {
      if (next) {
        chain.push(describe(next));
      }
      break;
    }
    node = next;
  }
  testRunner.log(`root chain: ${chain.join(' > ')}`);

  // A <span> carries no role and lays out no box of its own, so it is left
  // out and its button is reported one level up.
  const wrapped = result.nodes.find(n => n.name?.value === 'wrapped');
  const wrappedParent = byId.get(wrapped.parentId);
  const parentNode = (await dp.DOM.describeNode(
                          {backendNodeId: wrappedParent.backendDOMNodeId}))
                         .result.node;
  testRunner.log(`parent of the wrapped button: ${parentNode.localName}`);
  testRunner.log(`that parent lists it: ` +
                 `${wrappedParent.childIds.includes(wrapped.nodeId)}`);

  // role="presentation" drops the element's own semantics, but its button
  // still has to be reachable.
  const inside = result.nodes.find(n => n.name?.value === 'inside');
  testRunner.log(`button inside a presentational div is reported: ` +
                 `${Boolean(inside)}`);
  testRunner.log(`that button is not ignored: ${inside.ignored === false}`);

  testRunner.completeTest();
});
