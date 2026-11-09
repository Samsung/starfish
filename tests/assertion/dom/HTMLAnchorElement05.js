/**
*
    Retrieve the host attribute and examine its value.
*/
function HTMLAnchorElement05() {
    var success;
    var nodeList;
    var testNode;
    var password;

    nodeList = document.getElementsByTagName("a");
    if (nodeList.length != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode = nodeList.item(0);
    password = testNode.password;
    if (password != "pass") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode.password = "word";
    href = testNode.href;
    if (href != "http://user:word@www.example.com:123?search=abc#example") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   HTMLAnchorElement05();
}
