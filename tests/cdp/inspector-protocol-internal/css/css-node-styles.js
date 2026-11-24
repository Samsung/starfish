// Reads and edits the styles of one node through the CSS domain: computed,
// inline and matched styles, the matched sheet's text, and rule edits that
// the page must observe through getComputedStyle.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp, session} = await testRunner.startHTML(`
      <style>#box { color: rgb(1, 2, 3); width: 50px; }</style>
      <div id="box" style="height: 20px">box</div>`,
      'Tests the CSS domain on one node');
  await dp.DOM.enable();
  await dp.CSS.enable();
  const {result: {root}} = await dp.DOM.getDocument();
  const {result: {nodeId}} =
      await dp.DOM.querySelector({nodeId: root.nodeId, selector: '#box'});
  const page = expression => session.evaluate(expression);

  const {computedStyle} =
      (await dp.CSS.getComputedStyleForNode({nodeId})).result;
  const computed = name => computedStyle.find(p => p.name === name).value;
  testRunner.log(`computed color: ${computed('color')}`);
  testRunner.log(`computed width: ${computed('width')}`);

  const {inlineStyle} = (await dp.CSS.getInlineStylesForNode({nodeId})).result;
  const height = inlineStyle.cssProperties.find(p => p.name === 'height');
  testRunner.log(`inline height: ${height.value}`);

  const {matchedCSSRules} =
      (await dp.CSS.getMatchedStylesForNode({nodeId})).result;
  const match = matchedCSSRules.find(
      m => m.rule.selectorList.text === '#box');
  testRunner.log(`rule #box matched: ${!!match}`);

  const styleSheetId = match.rule.styleSheetId;
  const {text} = (await dp.CSS.getStyleSheetText({styleSheetId})).result;
  testRunner.log(`sheet text names the rule: ${text.includes('#box')}`);

  // The rule sits on the sheet's first line, so its declaration range
  // starts there too.
  await dp.CSS.setStyleTexts({edits: [{
    styleSheetId,
    range: (match.rule.style && match.rule.style.range) ||
        {startLine: 0, startColumn: 0, endLine: 0, endColumn: 0},
    text: 'color: rgb(7, 8, 9); width: 50px',
  }]});
  testRunner.log(
      `color after setStyleTexts: ${await page('getComputedStyle(box).color')}`);

  await dp.CSS.setStyleSheetText(
      {styleSheetId, text: '#box { color: rgb(4, 5, 6); }'});
  testRunner.log(
      `color after setStyleSheetText: ${await page('getComputedStyle(box).color')}`);

  const {frameTree} = (await dp.Page.getFrameTree()).result;
  const created =
      (await dp.CSS.createStyleSheet({frameId: frameTree.frame.id})).result;
  await dp.CSS.addRule({
    styleSheetId: created.styleSheetId,
    ruleText: '#box { opacity: 0.5 }',
    location: {startLine: 0, startColumn: 0, endLine: 0, endColumn: 0},
  });
  testRunner.log(
      `opacity after addRule: ${await page('getComputedStyle(box).opacity')}`);

  await dp.CSS.disable();
  testRunner.completeTest();
});
