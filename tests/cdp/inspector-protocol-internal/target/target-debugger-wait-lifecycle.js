(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  await testRunner.startBlank(
      'Tests Target debugger waiting, ordered filters, and live metadata.');
  const browser = testRunner.browserP();
  const target = browser.Target;
  const check = (name, value) => testRunner.log(`${name}: ${value}`);
  const pageURL = (title, script = '') =>
      'data:text/html,' + encodeURIComponent(
          `<title>${title}</title><script>${script}</script>`);
  const created = [];
  const attachments = [];
  target.onAttachedToTarget(event => attachments.push(event.params));
  await target.setDiscoverTargets({discover: true});
  await target.setAutoAttach({
    autoAttach: true, waitForDebuggerOnStart: true, flatten: true
  });

  const initialURL = pageURL('Initial');
  const attached = target.onceAttachedToTarget();
  const first = await target.createTarget({url: initialURL});
  const firstId = first.result.targetId;
  created.push(firstId);
  const attachment = (await attached).params;
  check('new attached target waits', attachment.waitingForDebugger);
  const dp = testRunner.createSessionFor(attachment.sessionId).protocol;
  await dp.Page.enable();
  await dp.Runtime.enable();
  await dp.Emulation.setDeviceMetricsOverride({
    width: 800, height: 600, deviceScaleFactor: 1, mobile: false
  });
  const viewport = await dp.Runtime.evaluate({
    expression: 'JSON.stringify([innerWidth, innerHeight])',
    returnByValue: true
  });
  check('viewport configured while waiting',
      viewport.result.result.value === '[800,600]');
  await dp.Runtime.addBinding({name: 'reportFirstScript'});
  await dp.Page.addScriptToEvaluateOnNewDocument({
    source: 'globalThis.beforeFirstScript = 42;'
  });
  const replacementURL = pageURL('Replacement',
      'reportFirstScript(String(globalThis.beforeFirstScript));' +
      'globalThis.firstScriptValue = globalThis.beforeFirstScript;');
  await dp.Page.navigate({url: replacementURL});
  const reported = dp.Runtime.onceBindingCalled(
      event => event.params.name === 'reportFirstScript');
  const loaded = dp.Page.onceLoadEventFired();
  await dp.Runtime.runIfWaitingForDebugger();
  check('first script sees initialization and binding',
      (await reported).params.payload === '42');
  await loaded;
  const state = await dp.Runtime.evaluate({
    expression: 'JSON.stringify([location.href, firstScriptValue])',
    returnByValue: true
  });
  check('queued navigation replaces initial URL',
      state.result.result.value === JSON.stringify([replacementURL, 42]));

  // Observe ordinary navigation and title changes, not synthetic resume events.
  const nextURL = pageURL('Navigated');
  const navigationInfo = target.onceTargetInfoChanged(event =>
    event.params.targetInfo.targetId === firstId &&
    event.params.targetInfo.url === nextURL);
  const nextLoad = dp.Page.onceLoadEventFired();
  await dp.Page.navigate({url: nextURL});
  check('discovery receives navigation info',
      (await navigationInfo).params.targetInfo.url === nextURL);
  await nextLoad;
  const titleInfo = target.onceTargetInfoChanged(event =>
    event.params.targetInfo.targetId === firstId &&
    event.params.targetInfo.title === 'Changed title');
  await dp.Runtime.evaluate({expression: "document.title = 'Changed title'"});
  check('discovery receives title info',
      (await titleInfo).params.targetInfo.title === 'Changed title');

  const blankInfo = target.onceTargetInfoChanged(event =>
    event.params.targetInfo.targetId === firstId &&
    event.params.targetInfo.url === 'about:blank');
  const blankLoad = dp.Page.onceLoadEventFired();
  await dp.Page.navigate({url: 'about:blank'});
  await blankInfo;
  await blankLoad;
  const targets = await target.getTargets();
  check('getTargets preserves actual about:blank',
      targets.result.targetInfos.find(info => info.targetId === firstId).url ===
          'about:blank');

  await target.setAutoAttach({
    autoAttach: false, waitForDebuggerOnStart: true, flatten: true
  });
  const free = await target.createTarget({url: pageURL('Not paused')});
  created.push(free.result.targetId);
  const manual = await target.attachToTarget({
    targetId: free.result.targetId, flatten: true
  });
  const freeDP = testRunner.createSessionFor(manual.result.sessionId).protocol;
  const freeState = await freeDP.Runtime.evaluate({expression: 'document.title'});
  check('disabled autoAttach does not pause scripting',
      !freeState.error && freeState.result.result.value === 'Not paused');

  const filtered = await target.setAutoAttach({
    autoAttach: true, waitForDebuggerOnStart: true, flatten: true,
    filter: [{type: 'page', exclude: true}, {}]
  });
  check('ordered exclusion filter accepted', !filtered.error);
  const excluded = await target.createTarget({url: pageURL('Excluded')});
  created.push(excluded.result.targetId);
  // The createTarget response is a protocol barrier for its attachment event.
  check('first matching exclusion prevents attachment',
      !attachments.some(info =>
        info.targetInfo.targetId === excluded.result.targetId));
  const excludedAttach = await target.attachToTarget({
    targetId: excluded.result.targetId, flatten: true
  });
  const excludedDP =
      testRunner.createSessionFor(excludedAttach.result.sessionId).protocol;
  const excludedState = await excludedDP.Runtime.evaluate({
    expression: 'document.title'
  });
  check('excluded target does not wait',
      !excludedState.error && excludedState.result.result.value === 'Excluded');

  // Auto-attachment alone subscribes the owning session to target info updates.
  await target.setDiscoverTargets({discover: false});
  await target.setAutoAttach({
    autoAttach: true, waitForDebuggerOnStart: false, flatten: true,
    filter: [{type: 'page'}]
  });
  const autoTitle = target.onceTargetInfoChanged(event =>
    event.params.targetInfo.targetId === excluded.result.targetId &&
    event.params.targetInfo.title === 'Auto-attached title');
  await excludedDP.Runtime.evaluate({
    expression: "document.title = 'Auto-attached title'"
  });
  check('auto-attach owner receives title info without discovery',
      (await autoTitle).params.targetInfo.title === 'Auto-attached title');

  const autoURL = pageURL('Auto-attached navigation');
  const autoNavigation = target.onceTargetInfoChanged(event =>
    event.params.targetInfo.targetId === excluded.result.targetId &&
    event.params.targetInfo.url === autoURL);
  await excludedDP.Page.enable();
  const autoLoad = excludedDP.Page.onceLoadEventFired();
  await excludedDP.Page.navigate({url: autoURL});
  check('auto-attach owner receives navigation info without discovery',
      (await autoNavigation).params.targetInfo.url === autoURL);
  await autoLoad;
  await target.setAutoAttach({
    autoAttach: false, waitForDebuggerOnStart: false, flatten: true
  });
  for (const targetId of created)
    await target.closeTarget({targetId});
  testRunner.completeTest();
});
