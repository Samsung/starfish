// Highlights a node through the Overlay domain and reads back the box the
// overlay would draw for it.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp} = await testRunner.startHTML(`
      <div id="target"
           style="width: 80px; height: 30px; margin: 5px; padding: 3px">
      </div>`,
      'Tests Overlay.highlightNode and getHighlightObjectForTest');
  await dp.DOM.enable();
  await dp.Overlay.enable();
  const {result: {root}} = await dp.DOM.getDocument();
  const {result: {nodeId}} =
      await dp.DOM.querySelector({nodeId: root.nodeId, selector: '#target'});

  const highlightConfig = {
    contentColor: {r: 255, g: 0, b: 0, a: 0.5},
    paddingColor: {r: 0, g: 255, b: 0, a: 0.5},
    marginColor: {r: 0, g: 0, b: 255, a: 0.5},
  };
  const highlighted = await dp.Overlay.highlightNode({highlightConfig, nodeId});
  testRunner.log(`highlightNode succeeded: ${!highlighted.error}`);

  const {highlight} =
      (await dp.Overlay.getHighlightObjectForTest({nodeId})).result;
  const info = highlight.elementInfo;
  testRunner.log(`tagName: ${info.tagName}`);
  // The border box: content plus padding.
  testRunner.log(`size: ${info.nodeWidth}x${info.nodeHeight}`);

  const hidden = await dp.Overlay.hideHighlight();
  testRunner.log(`hideHighlight succeeded: ${!hidden.error}`);
  const inspect = await dp.Overlay.setInspectMode({mode: 'none'});
  testRunner.log(`setInspectMode succeeded: ${!inspect.error}`);

  await dp.Overlay.disable();
  testRunner.completeTest();
});
