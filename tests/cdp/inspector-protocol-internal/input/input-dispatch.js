// Drives mouse, keyboard, text, IME, touch and gesture input through the
// Input domain and lists the DOM events the page received.
(async function(/** @type {import('test_runner').TestRunner} */ testRunner) {
  const {dp, session} = await testRunner.startHTML(`
      <input id="field" style="position: absolute; left: 0; top: 0;
                                width: 200px; height: 40px">
      <div style="height: 3000px"></div>
      <script>
        window.seen = new Set();
        for (const type of ['mousemove', 'mousedown', 'mouseup', 'click',
                            'keydown', 'keypress', 'keyup', 'input',
                            'touchstart', 'touchmove', 'touchend'])
          document.addEventListener(type, e => seen.add(e.type), true);
      </script>`,
      'Tests Input dispatch methods');
  const page = expression => session.evaluate(expression);

  for (const type of ['mouseMoved', 'mousePressed', 'mouseReleased'])
    await dp.Input.dispatchMouseEvent(
        {type, x: 50, y: 20, button: 'left', clickCount: 1});
  await dp.Input.dispatchMouseEvent(
      {type: 'mouseWheel', x: 100, y: 300, deltaX: 0, deltaY: 120});

  await dp.Input.dispatchKeyEvent({type: 'keyDown', key: 'a', text: 'a'});
  await dp.Input.dispatchKeyEvent({type: 'char', text: 'a'});
  await dp.Input.dispatchKeyEvent({type: 'keyUp', key: 'a'});
  await dp.Input.dispatchKeyEvent({type: 'rawKeyDown', key: 'b'});
  await dp.Input.dispatchKeyEvent({type: 'keyUp', key: 'b'});
  await dp.Input.insertText({text: 'xyz'});
  testRunner.log(`inserted text reached the field: ${
      (await page('field.value')).endsWith('xyz')}`);

  // Past the touch slop, so the move reaches the page.
  const point = (x, y) => ({x, y, id: 1});
  await dp.Input.dispatchTouchEvent(
      {type: 'touchStart', touchPoints: [point(60, 20)]});
  await dp.Input.dispatchTouchEvent(
      {type: 'touchMove', touchPoints: [point(60, 90)]});
  await dp.Input.dispatchTouchEvent({type: 'touchEnd', touchPoints: []});

  const tap = await dp.Input.synthesizeTapGesture({x: 50, y: 20, tapCount: 1});
  testRunner.log(`synthesizeTapGesture answers: ${!tap.error}`);
  const scroll = await dp.Input.synthesizeScrollGesture(
      {x: 100, y: 300, yDistance: -200});
  testRunner.log(`synthesizeScrollGesture answers: ${!scroll.error}`);
  const pinch = await dp.Input.synthesizePinchGesture(
      {x: 100, y: 100, scaleFactor: 1.5});
  testRunner.log(`synthesizePinchGesture answers: ${!pinch.error}`);

  testRunner.log(`events: ${await page('[...seen].sort().join(" ")')}`);
  testRunner.completeTest();
});
