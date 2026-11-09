/**
*
    Retrieve the port attribute and examine its value.
*/
function HTMLAnchorElement03() {
    var success;
    var nodeList;
    var testNode;
    var port;

    nodeList = document.getElementsByTagName("a");
    if (nodeList.length != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    testNode = nodeList.item(0);
    port = testNode.port;
    if (port != "123") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   HTMLAnchorElement03();
}
