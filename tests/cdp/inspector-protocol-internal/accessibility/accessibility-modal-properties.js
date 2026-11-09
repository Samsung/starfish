// The expected output is from real Chrome 155.0.8059.79.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {session, dp} = await testRunner.startHTML(`
    <button id="outside">Outside</button>
    <dialog id="modal"><button id="inside">Inside</button></dialog>
    <dialog id="modal-false" aria-modal="false">
      <button>Takes focus instead of the dialog</button>
    </dialog>
    <dialog id="nonmodal" open>Non-modal</dialog>
    <dialog id="nonmodal-aria" open aria-modal="true">ARIA modal override</dialog>
    <div id="aria-true" role="dialog" aria-modal="true"></div>
    <div id="aria-false" role="dialog" aria-modal="false"></div>
    <div id="alert" role="alertdialog" aria-modal="true"></div>
    <div id="focused-dialog" role="dialog" aria-modal="false"
         tabindex="0"></div>
  `, 'Tests native and ARIA modal dialog accessibility');

  const documentId = (await dp.DOM.getDocument()).result.root.nodeId;

  async function axNode(selector) {
    const nodeId = (await dp.DOM.querySelector({nodeId: documentId, selector}))
                       .result.nodeId;
    return (await dp.Accessibility.getPartialAXTree(
                {nodeId, fetchRelatives: false}))
        .result.nodes[0];
  }

  function state(node) {
    const properties = node.properties || [];
    const wanted = properties.filter(property =>
        ['focusable', 'focused', 'modal'].includes(property.name));
    return wanted.map(property =>
        `${property.name}:${property.value.type}=${property.value.value}`)
        .join(', ');
  }

  async function logNode(selector) {
    const node = await axNode(selector);
    testRunner.log(
        `${selector}: role=${node.role.value}, chromeRole=${node.chromeRole.value}` +
        `${state(node) ? `, ${state(node)}` : ''}`);
  }

  await logNode('#nonmodal');
  await logNode('#nonmodal-aria');
  await logNode('#aria-true');
  await logNode('#aria-false');
  await logNode('#alert');

  await session.evaluate(`document.querySelector('#focused-dialog').focus()`);
  await logNode('#focused-dialog');

  await session.evaluate(`document.querySelector('#modal').showModal()`);
  await logNode('#modal');

  const outside = await axNode('#outside');
  const reason = outside.ignoredReasons[0];
  testRunner.log(
      `#outside: ignored=${outside.ignored}, reason=${reason.name}:${reason.value.type}` +
      `->#${reason.value.relatedNodes[0].idref}`);

  const inside = await axNode('#inside');
  testRunner.log(`#inside: ignored=${inside.ignored}`);

  await session.evaluate(`
    document.querySelector('#modal').close();
    document.querySelector('#modal-false').showModal();
  `);
  await logNode('#modal-false');

  testRunner.completeTest();
});
