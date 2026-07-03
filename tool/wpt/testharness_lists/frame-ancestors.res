# CSP frame-ancestors (content-security-policy/frame-ancestors/).
# Active lines pass reliably under wpt_runner -j1; "# [auto-fail]" lines are
# known failures, deferred: blocked-frame detection in these tests relies on a
# cross-origin window-access SecurityError that is not yet enforced for frames
# blocked by frame-ancestors (a separate follow-up from the XFO/frame-ancestors
# delivery fix).
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-cross-in-cross-none-block.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-cross-in-cross-self-block.html
http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-cross-in-cross-star-allow.html
http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-cross-in-cross-url-allow.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-cross-in-cross-url-block.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-cross-in-same-none-block.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-cross-in-same-self-block.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-cross-in-same-star-allow.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-cross-in-same-url-allow.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-cross-in-same-url-block.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-cross-in-sandboxed-cross-url-block.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-same-in-cross-none-block.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-same-in-cross-self-block.html
http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-same-in-cross-star-allow.html
http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-same-in-cross-url-allow.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-same-in-cross-url-block.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-same-in-same-none-block.html
http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-same-in-same-self-allow.html
http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-same-in-same-star-allow.html
http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-same-in-same-url-allow.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-nested-same-in-same-url-block.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-none-block.html
http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-overrides-xfo.html
http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-self-allow.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-self-block.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-star-allow-crossorigin.html
http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-star-allow-sameorigin.html
http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-url-allow.sub.html
# [auto-fail] http://web-platform.test:8000/content-security-policy/frame-ancestors/frame-ancestors-url-block.html
