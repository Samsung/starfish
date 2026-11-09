// The expected output is from real Chrome 155.0.8059.79.
// Pins the compact CDP property contract without depending on the rest of an
// AXNode dump (name sources, tree ids, and layout-generated children).
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {session, dp} = await testRunner.startHTML(`
    <span id="label">Account</span>
    <input id="text" aria-labelledby="label" aria-invalid="true"
           aria-keyshortcuts="Ctrl+K" aria-roledescription="account field"
           required>
    <textarea id="textarea" aria-invalid="false" aria-multiline="false"
              readonly required></textarea>
    <div id="aria-invalid" role="group" aria-invalid="unknown"></div>
    <div id="aria-grammar" role="group" aria-invalid="grammar"></div>
    <div id="aria-spelling" role="group" aria-invalid="spelling"></div>
    <div id="aria-undefined" role="group" aria-invalid="undefined"></div>
    <div id="aria-valid-override" role="group" aria-invalid="false"></div>
    <div id="grammar-textbox" role="textbox"
         aria-invalid="grammar"></div>
    <div id="aria-disabled-check" role="checkbox"
         aria-disabled="true"></div>
    <div id="rich-generic" contenteditable="true">Editable generic</div>
    <div id="rich-textbox" role="textbox"
         contenteditable="true">Editable textbox</div>
    <input id="native-check" type="checkbox" aria-invalid="false">
    <div id="mixed-check" role="checkbox" aria-checked="mixed"></div>
    <div id="mixed-switch" role="switch" aria-checked="mixed"></div>
    <div role="tree">
      <div id="treeitem" role="treeitem" aria-checked="true"
           aria-expanded="false" aria-level="3" aria-selected="false"></div>
    </div>
    <div role="listbox">
      <div id="checked-option" role="option" aria-checked="true"></div>
    </div>
    <div id="listbox" role="listbox"></div>
    <div id="tablist" role="tablist"></div>
    <div id="tree" role="tree" aria-multiselectable="true"></div>
    <div id="menubar" role="menubar" aria-orientation="vertical"></div>
    <div id="searchbox" role="searchbox" aria-multiline="true"
         aria-required="true"></div>
    <div id="searchbox-readonly" role="searchbox"
         aria-readonly="true"></div>
    <div id="treegrid-default" role="treegrid"></div>
    <div id="treegrid-horizontal" role="treegrid"
         aria-orientation="horizontal"></div>
    <div id="heading-default" role="heading"></div>
    <h4 id="heading-invalid" aria-level="3junk"></h4>
    <div role="list">
      <div id="level-decimal" role="listitem" aria-level="2.5"></div>
      <div id="level-negative" role="listitem" aria-level="-1"></div>
      <div id="level-zero" role="listitem" aria-level="0"></div>
      <div id="level-overflow" role="listitem"
           aria-level="999999999999999999999999"></div>
      <div id="level-plain" role="listitem"></div>
    </div>
    <ul><li id="list-top">Top
      <ul><li id="list-nested">Nested</li></ul></li></ul>
    <div id="level-heading-ten" role="heading" aria-level="10"></div>
    <div role="list">
      <div id="expanded-listitem" role="listitem"
           aria-expanded="true"></div>
    </div>
    <div id="readonly-ignored" role="button" aria-readonly="true"></div>
    <div role="tree">
      <div id="treeitem-top" role="treeitem" aria-selected="false">
        <div role="group">
          <div id="treeitem-nested" role="treeitem"
               aria-selected="false"></div>
          <div role="tree">
            <div id="treeitem-nested-tree" role="treeitem"
                 aria-selected="false"></div>
          </div>
        </div>
      </div>
    </div>
    <div id="unsupported-expanded" role="dialog" aria-expanded="true"></div>
    <div id="invalid-expanded" role="button" aria-expanded="bogus"></div>
    <div id="expanded-application" role="application"
         aria-expanded="true"></div>
    <div id="expanded-checkbox" role="checkbox"
         aria-expanded="true"></div>
    <div id="expanded-menuitemcheckbox" role="menuitemcheckbox"
         aria-expanded="false"></div>
    <div id="expanded-columnheader" role="columnheader"
         aria-expanded="true"></div>
    <div id="selected-outside-grid" role="columnheader"
         aria-selected="true"></div>
    <div role="grid"><div role="row">
      <div id="selected-header" role="columnheader"
           aria-selected="true"></div>
    </div></div>
    <select>
      <option id="option-old">Old choice</option>
      <option id="option" selected>New choice</option>
    </select>
    <select><option id="hidden-option" aria-hidden="true">Visible option</option></select>
    <button id="body-visible">Visible despite body aria-hidden</button>
    <div id="hidden-root" aria-hidden="true">
      <button id="hidden-child">Hidden child</button>
    </div>
  `, 'Tests typed and ordered accessibility properties');

  await session.evaluate(`
    document.documentElement.setAttribute('aria-hidden', 'true');
    document.body.setAttribute('aria-hidden', 'true');
    document.querySelector('#native-check').checked = true;
    document.querySelector('#text').focus();
  `);

  const documentId = (await dp.DOM.getDocument()).result.root.nodeId;

  async function axNode(selector) {
    const nodeId = (await dp.DOM.querySelector({nodeId: documentId, selector}))
                       .result.nodeId;
    return (await dp.Accessibility.getPartialAXTree(
                {nodeId, fetchRelatives: false}))
        .result.nodes[0];
  }

  function propertyText(property) {
    const value = property.value;
    let text = `${property.name}:${value.type}`;
    if (value.value !== undefined)
      text += `=${value.value}`;
    if (value.relatedNodes) {
      const targets = value.relatedNodes.map(node =>
          node.idref ? `#${node.idref}` : `<backend:${node.backendDOMNodeId}>`);
      text += `->${targets.join(',')}`;
    }
    return text;
  }

  async function logProperties(selector) {
    const node = await axNode(selector);
    const properties = (node.properties || []).map(propertyText).join(', ');
    testRunner.log(`${selector}: ${properties || '<none>'}`);
  }

  for (const selector of [
    '#text', '#textarea', '#aria-invalid', '#aria-grammar',
    '#aria-spelling', '#aria-undefined', '#aria-valid-override',
    '#grammar-textbox',
    '#aria-disabled-check', '#rich-generic', '#rich-textbox',
    '#native-check', '#mixed-check', '#mixed-switch', '#treeitem',
    '#checked-option', '#listbox', '#tablist', '#tree', '#menubar',
    '#searchbox', '#searchbox-readonly', '#treegrid-default', '#treegrid-horizontal',
    '#heading-default', '#heading-invalid', '#level-decimal',
    '#level-negative', '#level-zero', '#level-overflow', '#level-plain',
    '#list-top', '#list-nested', '#level-heading-ten',
    '#expanded-listitem', '#readonly-ignored',
    '#treeitem-top', '#treeitem-nested',
    '#treeitem-nested-tree', '#unsupported-expanded', '#invalid-expanded',
    '#expanded-application', '#expanded-checkbox',
    '#expanded-menuitemcheckbox', '#expanded-columnheader',
    '#selected-outside-grid', '#selected-header', '#option-old', '#option'
  ]) {
    await logProperties(selector);
  }

  // Hidden state is reported as an ignored reason.
  for (const selector of ['#hidden-root', '#hidden-child']) {
    const node = await axNode(selector);
    const reasons = (node.ignoredReasons || []).map(reason => {
      const targets = (reason.value.relatedNodes || [])
          .map(target => target.idref ? `#${target.idref}` : '<backend>');
      return `${reason.name}${targets.length ? '->' + targets.join(',') : ''}`;
    });
    testRunner.log(
        `${selector}: ignored=${node.ignored}, ` +
        `reasons=${reasons.join(', ') || '<none>'}`);
  }

  for (const selector of ['#body-visible', '#hidden-option']) {
    const node = await axNode(selector);
    testRunner.log(`${selector}: ignored=${node.ignored}`);
  }

  testRunner.completeTest();
});
