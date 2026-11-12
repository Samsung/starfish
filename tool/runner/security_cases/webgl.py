"""WebGL security cases; feature options belong to the group."""

import base64

from . import Case, Suite


def cases(context):
    mode = context.features["webgl"]
    if mode == "on":
        # This page is already in internal.res.
        yield Case("buffers", context.http(
            "assertion/canvas/webgl-buffer-input-validation.html"),
            internal=False)
    elif mode == "off":
        source = """<script>
var passed = !document.createElement('canvas').getContext('webgl') &&
             !document.createElement('canvas').getContext('webgl2');
console.log('PRODUCT_SECURITY:' + JSON.stringify({
  passed: passed, testHooks: typeof testEnd === 'function'
}));
</script>"""
        url = "data:text/html;base64," + base64.b64encode(source.encode()).decode()
        yield Case("disabled", url, internal=False)
    else:
        raise ValueError("feature webgl must be on or off")


SUITE = Suite("webgl", "Buffer validation or WebGL API absence", cases,
              features={"webgl": "on"})
