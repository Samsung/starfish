/**
*
    Retrieve the host attribute and examine its value.
*/
function HTMLAnchorElement07() {
    var success;
    var nodeList;
    var testNode;
    var hash;

    nodeList = document.getElementsByTagName("a");
    if (nodeList.length != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode = nodeList.item(0);
    hash = testNode.hash;
    if (hash != "#example") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode.hash = "section";
    href = testNode.href;
    if (href != "http://user:pass@www.example.com:123?search=abc#section") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode.hash = "#research";
    href = testNode.href;
    if (href != "http://user:pass@www.example.com:123?search=abc#research") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode.hash = "##other";
    href = testNode.href;
    if (href != "http://user:pass@www.example.com:123?search=abc##other") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   HTMLAnchorElement07();
}
