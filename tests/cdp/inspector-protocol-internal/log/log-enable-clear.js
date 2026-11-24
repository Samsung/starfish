// The Log domain toggles and clears without errors.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp} = await testRunner.startBlank('Tests Log.enable, clear, disable');
  for (const method of ['enable', 'clear', 'disable']) {
    const response = await dp.Log[method]();
    testRunner.log(`Log.${method}: ${response.error ? 'error' : 'ok'}`);
  }
  testRunner.completeTest();
});
