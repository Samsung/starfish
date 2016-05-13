function assert_equals(value1, value2, msg) {
    return value1 == value2;
}
function assert_true(value1, msg) {
    return value1 == true;
}
function assert_false(value1, msg) {
    return value1 == false;
}

function getObject(interface) {
    switch(interface) {
        case "Element":
            var e = document.createElementNS("http://example.com/", "example");
            assert_true(e instanceof Element);
            assert_false(e instanceof HTMLElement);
            assert_false(e instanceof SVGElement);
            return e;
        case "HTMLElement":
            var e = document.createElement("html");
            assert_true(e instanceof HTMLElement);
            return e;
        case "HTMLBodyElement":
            var e = document.createElement("body");
            assert_true(e instanceof HTMLBodyElement);
            return e;
        case "Document":
            assert_true(document instanceof Document);
            return document;
        case "Window":
            assert_true(window instanceof Window);
            return window;
    }
    assert_unreached();
}

function testSet(interface, attribute) {
    var result = true;
    var object = getObject(interface);
    function nop() {}

    result &= assert_equals(object[attribute], null, "Initially null");
    object[attribute] = nop;
    result &= assert_equals(object[attribute], nop, "Return same function");
    object[attribute] = "";
    result &= assert_equals(object[attribute], null, "Return null after setting string");
    object[attribute] = null;
    result &= assert_equals(object[attribute], null, "Finally null");
    console.log((result ? "PASS " : "FAIL ") + "Set " + interface + "." + attribute);
}

function testEnumerate(interface, attribute) {
    var result = true;
    var object = getObject(interface);
    result &= assert_true((attribute in object));
    console.log((result ? "PASS " : "FAIL ") + "Enumerate " + interface + "." + attribute);
}

function testReflect(interface, attribute) {
    var result = true;
    var element = getObject(interface);
    result &= assert_false(element.hasAttribute(attribute), "Initially missing");
    element.setAttribute(attribute, "return");
    result &= assert_equals(element.getAttribute(attribute), "return", "Return same string");
    result &= assert_equals(typeof element[attribute], "function", "Convert to function");
    element.removeAttribute(attribute);
    console.log((result ? "PASS " : "FAIL ") + "Reflect " + interface + "." + attribute);
}

function testForwardToWindow(interface, attribute) {
    var result = true;
    var element = getObject(interface);
    window[attribute] = null;
    element.setAttribute(attribute, "return");
    result &= assert_equals(typeof window[attribute], "function", "Convert to function");
    result &= assert_equals(window[attribute], element[attribute], "Forward content attribute");
    function nop() {}
    element[attribute] = nop;
    result &= assert_equals(window[attribute], nop, "Forward IDL attribute");
    window[attribute] = null;
    console.log((result ? "PASS " : "FAIL ") + "Forward " + interface + "." + attribute + " to Window");
}