/**
*
    Retrieve the hostname attribute and examine its value.
*/
function HTMLAnchorElement02() {
    var success;
    var nodeList;
    var testNode;
    var hostname;

    nodeList = document.getElementsByTagName("a");
    if (nodeList.length != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    testNode = nodeList.item(0);
    hostname = testNode.hostname;
    if (hostname != "www.example.com") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   HTMLAnchorElement02();
}
