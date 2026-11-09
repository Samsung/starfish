/**
*
    Retrieve the host attribute and examine its value.
*/
function HTMLAnchorElement08() {
    var success;
    var nodeList;
    var testNode;
    var username;
    var password;
    var search;
    var hash;
    var origin;

    nodeList = document.getElementsByTagName("a");
    if (nodeList.length != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode = nodeList.item(0);
    username = testNode.username;
    password = testNode.password;
    search = testNode.search;
    hash = testNode.hash;
    origin = testNode.origin;
    if (username != "" || password != "" || search != "" || hash != "" || origin != "null") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   HTMLAnchorElement08();
}
