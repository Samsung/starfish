// Evaluates, inspects and releases remote objects, compiles and runs a
// script, and calls back into the client through a binding.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp} = await testRunner.startBlank(
      'Tests Runtime objects, scripts and bindings');
  await dp.Runtime.enable();

  const byValue = (await dp.Runtime.evaluate(
      {expression: '({a: 1, b: [2, 3]})', returnByValue: true})).result;
  testRunner.log(`returnByValue: ${JSON.stringify(byValue.result.value)}`);
  const awaited = (await dp.Runtime.evaluate(
      {expression: 'Promise.resolve(7)', awaitPromise: true})).result;
  testRunner.log(`awaitPromise: ${awaited.result.value}`);
  const thrown = (await dp.Runtime.evaluate(
      {expression: 'throw new Error("boom")'})).result;
  testRunner.log(`exception reported: ${!!thrown.exceptionDetails}`);

  const {result: object} = (await dp.Runtime.evaluate(
      {expression: '({x: 1, get y() { return 2; }})', objectGroup: 'g'}))
      .result;
  const own = (await dp.Runtime.getProperties(
      {objectId: object.objectId, ownProperties: true})).result.result;
  testRunner.log(`own properties: ${own.map(p => p.name).sort().join(',')}`);
  const called = (await dp.Runtime.callFunctionOn({
    objectId: object.objectId,
    functionDeclaration: 'function(n) { return this.x + n; }',
    arguments: [{value: 41}],
    returnByValue: true,
  })).result;
  testRunner.log(`callFunctionOn: ${called.result.value}`);
  await dp.Runtime.releaseObject({objectId: object.objectId});

  const {scriptId} = (await dp.Runtime.compileScript({
    expression: '6 * 7', sourceURL: 'compiled.js', persistScript: true,
  })).result;
  const ran = (await dp.Runtime.runScript({scriptId, returnByValue: true}))
                  .result;
  testRunner.log(`runScript: ${ran.result.value}`);

  await dp.Runtime.addBinding({name: 'notify'});
  const binding = dp.Runtime.onceBindingCalled();
  await dp.Runtime.evaluate({expression: 'notify("hello")'});
  const {params} = await binding;
  testRunner.log(`bindingCalled: ${params.name}(${params.payload})`);
  await dp.Runtime.removeBinding({name: 'notify'});

  const heap = (await dp.Runtime.getHeapUsage()).result;
  testRunner.log(`heap used > 0: ${heap.usedSize > 0}`);
  await dp.Runtime.runIfWaitingForDebugger();
  await dp.Runtime.disable();
  testRunner.completeTest();
});
