(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp} = await testRunner.startBlank(
      'Tests Accessibility.getAXNodeAndAncestors selector errors');
  await dp.Accessibility.enable();

  const response = await dp.Accessibility.getAXNodeAndAncestors({});
  testRunner.log(
      `missing selector returns expected error: ` +
      `${response.error?.code === -32000 &&
         response.error?.message ===
             'Either nodeId, backendNodeId or objectId must be specified'}`);

  testRunner.completeTest();
});
