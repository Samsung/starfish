// Captures the page as PNG, JPEG, PDF and a screencast, and walks the
// Page domain's frame, layout, script injection and history methods.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp, session} = await testRunner.startHTML(
      `<body style="margin: 0; background: rgb(0, 128, 0)"><h1>page</h1></body>`,
      'Tests Page capture, frame and history methods');
  const page = expression => session.evaluate(expression);
  await dp.Page.enable();

  const png = (await dp.Page.captureScreenshot({format: 'png'})).result.data;
  testRunner.log(`png signature: ${atob(png).slice(1, 4)}`);
  const clipped = (await dp.Page.captureScreenshot({
    format: 'png', clip: {x: 0, y: 0, width: 50, height: 50, scale: 1},
  })).result.data;
  testRunner.log(`clipped capture is smaller: ${clipped.length < png.length}`);
  const pdf = (await dp.Page.printToPDF({landscape: true, scale: 0.5,
                                         printBackground: true})).result.data;
  testRunner.log(`pdf signature: ${atob(pdf).slice(0, 5)}`);

  const frame = dp.Page.onceScreencastFrame();
  await dp.Page.startScreencast({format: 'png', everyNthFrame: 1});
  await page('document.body.style.background = "rgb(0, 0, 128)"');
  const {params: {sessionId, data}} = await frame;
  testRunner.log(`screencast frame is png: ${atob(data).slice(1, 4) === 'PNG'}`);
  await dp.Page.screencastFrameAck({sessionId});
  await dp.Page.stopScreencast();

  const {frameTree} = (await dp.Page.getFrameTree()).result;
  testRunner.log(`frame tree has a main frame: ${!!frameTree.frame.id}`);
  const {frameTree: resources} = (await dp.Page.getResourceTree()).result;
  testRunner.log(`resource tree frame matches: ${resources.frame.id === frameTree.frame.id}`);
  const metrics = (await dp.Page.getLayoutMetrics()).result;
  testRunner.log(`layout viewport width > 0: ${metrics.cssLayoutViewport.clientWidth > 0}`);

  await dp.Page.setDocumentContent(
      {frameId: frameTree.frame.id, html: '<title>replaced</title>'});
  testRunner.log(`setDocumentContent: ${await page('document.title')}`);

  const {identifier} = (await dp.Page.addScriptToEvaluateOnNewDocument(
      {source: 'window.injected = 42;'})).result;
  const {executionContextId} = (await dp.Page.createIsolatedWorld(
      {frameId: frameTree.frame.id, worldName: 'world'})).result;
  testRunner.log(`isolated world context: ${typeof executionContextId}`);
  await dp.Page.setLifecycleEventsEnabled({enabled: true});
  const loaded = dp.Page.onceLoadEventFired();
  await dp.Page.reload();
  await loaded;
  testRunner.log(`injected script ran: ${await page('window.injected')}`);
  await dp.Page.removeScriptToEvaluateOnNewDocument({identifier});

  let navigated = dp.Page.onceLoadEventFired();
  await dp.Page.navigate({url: 'data:text/html,<title>second</title>'});
  await navigated;
  const history = (await dp.Page.getNavigationHistory()).result;
  testRunner.log(`history entries > 1: ${history.entries.length > 1}`);
  const back = history.entries[history.currentIndex - 1];
  navigated = dp.Page.onceLoadEventFired();
  await dp.Page.navigateToHistoryEntry({entryId: back.id});
  await navigated;
  testRunner.log(`back from second: ${await page('document.title') !== 'second'}`);
  await dp.Page.resetNavigationHistory();

  const front = await dp.Page.bringToFront();
  testRunner.log(`bringToFront answers: ${!front.error}`);
  const bypass = await dp.Page.setBypassCSP({enabled: true});
  testRunner.log(`setBypassCSP answers: ${!bypass.error}`);
  const manifest = await dp.Page.getAppManifest();
  testRunner.log(`getAppManifest answers: ${!manifest.error}`);

  const dialog = dp.Page.onceJavascriptDialogOpening();
  page('setTimeout(() => confirm("sure?"), 0)');
  const {params: opened} = await dialog;
  testRunner.log(`dialog: ${opened.type} "${opened.message}"`);
  const handled = await dp.Page.handleJavaScriptDialog({accept: true});
  testRunner.log(`handleJavaScriptDialog answers: ${!handled.error}`);

  await dp.Page.disable();
  testRunner.completeTest();
});
