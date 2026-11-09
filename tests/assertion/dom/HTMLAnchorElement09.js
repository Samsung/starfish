/**
*
    Retrieve the host attribute and examine its value.
*/
function HTMLAnchorElement09() {
    var success;
    var nodeList;
    var testNode;
    var origin;

    nodeList = document.getElementsByTagName("a");
    if (nodeList.length != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode = nodeList.item(0);
    origin = testNode.origin;
    if (origin != "http://www.example.com:123") {
        console.log("FAIL", origin);
        wptTestEnd();
        return;
    }

    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   HTMLAnchorElement09();
}
