// Applies and clears Emulation overrides and checks what the page sees.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp, session} = await testRunner.startBlank(
      'Tests Emulation overrides');
  const page = expression => session.evaluate(expression);

  await dp.Emulation.setDeviceMetricsOverride(
      {width: 400, height: 300, deviceScaleFactor: 2, mobile: false});
  testRunner.log(`metrics override: ${await page('innerWidth + "x" + innerHeight')}` +
                 ` dpr=${await page('devicePixelRatio')}`);
  await dp.Emulation.clearDeviceMetricsOverride();

  await dp.Emulation.setEmulatedMedia({
    media: 'print',
    features: [{name: 'prefers-color-scheme', value: 'dark'}],
  });
  testRunner.log(`print media: ${await page('matchMedia("print").matches')}`);
  testRunner.log(
      `dark scheme: ${await page('matchMedia("(prefers-color-scheme: dark)").matches')}`);
  await dp.Emulation.setEmulatedMedia({media: '', features: []});
  testRunner.log(`media cleared: ${await page('matchMedia("print").matches')}`);

  await dp.Emulation.setUserAgentOverride({userAgent: 'Emulated/1.0'});
  testRunner.log(`userAgent: ${await page('navigator.userAgent')}`);

  const geo = await dp.Emulation.setGeolocationOverride(
      {latitude: 37.5, longitude: 127, accuracy: 10});
  testRunner.log(`setGeolocationOverride answers: ${!geo.error}`);
  const clearGeo = await dp.Emulation.clearGeolocationOverride();
  testRunner.log(`clearGeolocationOverride answers: ${!clearGeo.error}`);

  const visible = await dp.Emulation.setVisibleSize({width: 300, height: 200});
  testRunner.log(`setVisibleSize answers: ${!visible.error}`);

  await dp.Emulation.setScriptExecutionDisabled({value: true});
  await dp.Emulation.setScriptExecutionDisabled({value: false});
  testRunner.log(`script runs again: ${await page('1 + 1')}`);
  testRunner.completeTest();
});
