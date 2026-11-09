/**
*
    Retrieve the host attribute and examine its value.
*/
function HTMLAnchorElement04() {
    var success;
    var nodeList;
    var testNode;
    var username;

    nodeList = document.getElementsByTagName("a");
    if (nodeList.length != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode = nodeList.item(0);
    username = testNode.username;
    if (username != "user") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode.username = "name";
    href = testNode.href;
    if (href != "http://name:pass@www.example.com:123?search=abc#example") {
        console.log("FAIL", href);
        wptTestEnd();
        return;
    }
    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   HTMLAnchorElement04();
}
