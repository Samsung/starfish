/**
*
    Retrieve the host attribute and examine its value.
*/
function HTMLAnchorElement01() {
    var success;
    var nodeList;
    var testNode;
    var host;

    nodeList = document.getElementsByTagName("a");
    if (nodeList.length != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    testNode = nodeList.item(0);
    host = testNode.host;
    if (host != "www.example.com:123") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   HTMLAnchorElement01();
}
