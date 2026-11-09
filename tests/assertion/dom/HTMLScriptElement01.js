/**
*
    Retrieve the event and for attribute and examine its value.
*/
function HTMLScriptElement01() {
    var success;
    var nodeList;
    var testNode;
    var eventAttr;
    var htmlForAttr;

    nodeList = document.getElementsByTagName("script");
    if (nodeList.length != 3) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    testNode = nodeList.item(2);
    eventAttr = testNode.event;
    htmlForAttr = testNode.htmlFor;
    if (eventAttr != "onload" || htmlForAttr != "window") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   HTMLScriptElement01();
}
