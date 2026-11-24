// Follows a page load through Network events, reads the document's body,
// then blocks the page's script and sees the load report it as failed.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp, session} = await testRunner.startBlank(
      'Tests Network request events, headers, bodies and blocking');
  await dp.Page.enable();
  await dp.Network.enable();
  await dp.Network.setExtraHTTPHeaders({headers: {'X-Test': 'yes'}});
  await dp.Network.setCacheDisabled({cacheDisabled: true});
  await dp.Network.setUserAgentOverride({userAgent: 'NetworkUA/1.0'});

  const name = url => url.split('/').pop();
  const events = [];
  let documentRequest;
  dp.Network.onRequestWillBeSent(({params}) => {
    events.push(`request ${name(params.request.url)} ` +
                `X-Test=${params.request.headers['X-Test']}`);
    if (params.type === 'Document')
      documentRequest = params.requestId;
  });
  dp.Network.onResponseReceived(({params}) => events.push(
      `response ${name(params.response.url)} ${params.response.status} ` +
      `${params.type}`));
  dp.Network.onLoadingFailed(
      ({params}) => events.push(`failed ${params.errorText}`));

  let loaded = dp.Page.onceLoadEventFired();
  await dp.Page.navigate({url: testRunner.url('resources/network-page.html')});
  await loaded;
  testRunner.log(`script ran: ${await session.evaluate('window.scriptRan')}`);
  const {body} =
      (await dp.Network.getResponseBody({requestId: documentRequest})).result;
  testRunner.log(`document body has the title: ${
      body.includes('<title>network page</title>')}`);

  await dp.Network.setBlockedURLs({urls: ['*network-script*']});
  loaded = dp.Page.onceLoadEventFired();
  await dp.Page.reload();
  await loaded;
  testRunner.log(`script ran after blocking: ${
      await session.evaluate('String(window.scriptRan)')}`);
  await dp.Network.setBlockedURLs({urls: []});

  for (const line of events)
    testRunner.log(line);
  await dp.Network.clearBrowserCache();
  await dp.Network.disable();
  testRunner.completeTest();
});
