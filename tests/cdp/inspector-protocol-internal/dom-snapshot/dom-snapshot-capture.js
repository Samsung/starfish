// Captures a page with DOMSnapshot and checks the flattened document names
// the nodes, their computed styles and their layout.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp} = await testRunner.startHTML(`
      <div id="outer" style="display: block; width: 100px; height: 40px">
        <span>text</span>
      </div>`,
      'Tests DOMSnapshot.captureSnapshot and getSnapshot');
  await dp.DOMSnapshot.enable();

  const {documents, strings} = (await dp.DOMSnapshot.captureSnapshot({
    computedStyles: ['display'],
    includeDOMRects: true,
  })).result;
  const doc = documents[0];
  const names = doc.nodes.nodeName.map(index => strings[index]);
  testRunner.log(`documents: ${documents.length}`);
  testRunner.log(`has DIV: ${names.includes('DIV')}`);
  testRunner.log(`has SPAN: ${names.includes('SPAN')}`);
  testRunner.log(
      `parents and names line up: ` +
      `${doc.nodes.parentIndex.length === doc.nodes.nodeName.length}`);
  const styles = doc.layout.styles.map(s => strings[s[0]]);
  testRunner.log(`a laid out node is display:block: ${styles.includes('block')}`);
  testRunner.log(`layout has bounds: ${doc.layout.bounds.length > 0}`);

  // The deprecated method answers with the same kind of capture.
  const legacy = (await dp.DOMSnapshot.getSnapshot(
      {computedStyleWhitelist: ['display']})).result;
  testRunner.log(`getSnapshot answers: ${!!legacy}`);

  await dp.DOMSnapshot.disable();
  testRunner.completeTest();
});
