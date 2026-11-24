// Sets, lists and deletes cookies through the Network domain.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp} = await testRunner.startURL(
      'resources/network-blank.html', 'Tests Network cookie methods');
  await dp.Network.enable();
  const url = testRunner.url('resources/network-blank.html');

  await dp.Network.setCookie({name: 'a', value: '1', url});
  await dp.Network.setCookies({cookies: [{name: 'b', value: '2', url}]});
  const names = async () => (await dp.Network.getCookies({urls: [url]}))
      .result.cookies.map(c => c.name).sort().join(',');
  testRunner.log(`cookies: ${await names()}`);
  const all = (await dp.Network.getAllCookies()).result.cookies;
  testRunner.log(`getAllCookies has a: ${all.some(c => c.name === 'a')}`);
  await dp.Network.deleteCookies({name: 'a', url});
  testRunner.log(`after deleteCookies: ${await names()}`);
  await dp.Network.clearBrowserCookies();
  testRunner.log(`after clearBrowserCookies: "${await names()}"`);

  await dp.Network.disable();
  testRunner.completeTest();
});
