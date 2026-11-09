/**
*
    Retrieve the host attribute and examine its value.
*/
function HTMLAnchorElement06() {
    var success;
    var nodeList;
    var testNode;
    var search;

    nodeList = document.getElementsByTagName("a");
    if (nodeList.length != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode = nodeList.item(0);
    search = testNode.search;
    if (search != "?search=abc") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode.search= "search=def";
    href = testNode.href;
    if (href != "http://user:pass@www.example.com:123?search=def#example") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode.search= "?search=ghi";
    href = testNode.href;
    if (href != "http://user:pass@www.example.com:123?search=ghi#example") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    testNode.search= "??search=jkl";
    href = testNode.href;
    if (href != "http://user:pass@www.example.com:123??search=jkl#example") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   HTMLAnchorElement06();
}
