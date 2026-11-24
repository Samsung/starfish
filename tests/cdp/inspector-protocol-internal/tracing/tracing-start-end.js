// Records a trace and receives it through Tracing events.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp, session} = await testRunner.startBlank(
      'Tests Tracing.start, end and getCategories');
  const {categories} = (await dp.Tracing.getCategories()).result;
  testRunner.log(`categories listed: ${categories.length > 0}`);

  const events = [];
  dp.Tracing.onDataCollected(({params}) => events.push(...params.value));
  const complete = dp.Tracing.onceTracingComplete();
  await dp.Tracing.start({transferMode: 'ReportEvents',
                          traceConfig: {includedCategories: categories}});
  await session.evaluate('document.body.textContent = "traced"');
  await dp.Tracing.end();
  await complete;
  testRunner.log(`trace has events: ${events.length > 0}`);

  const dump = await dp.Tracing.requestMemoryDump();
  testRunner.log(`requestMemoryDump answers: ${!dump.error}`);
  testRunner.completeTest();
});
