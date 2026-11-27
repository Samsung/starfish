# Internal CDP test rationale

Add a row when introducing or extending an internal test to cover a protocol
behavior that is not checked by the pinned Blink suite. Name the closest Blink
test, explain what it misses, and record the Chrome version and result for the
same test and expected output. Do not use the table as a test list.

| Internal test | Closest Blink test and what it checks | What the internal test adds | Chrome verification |
|---|---|---|---|
| [`dom/dom-node-path-exposure.js`](dom/dom-node-path-exposure.js) | `dom/dom-node-push-by-path-to-frontend.js` loads the entire tree before looking up nodes by path. | With no document request, the command returns no usable frontend node ID. With a shallow tree, the missing nodes arrive in `DOM.setChildNodes` before the command returns a usable ID for an element inside an iframe. | Chrome `155.0.8059.79` headless: both scripts matched their original expected output via CDP. |
