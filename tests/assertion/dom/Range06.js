/**
*
    Retrieve the host attribute and examine its value.
*/
function Range06() {
    var success;
    var nodeList;
    var testNode1;
    var testNode2;

    nodeList = document.getElementsByTagName("p");
    if (nodeList.length != 2) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    testNode1 = nodeList.item(0);
    testNode1.appendChild(document.createTextNode("P1 "));
    testNode1.appendChild(document.createTextNode("P2 "));
    if (testNode1.childNodes.length != 2) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    testNode2 = nodeList.item(1);
    testNode2.appendChild(document.createTextNode("P3 "));
    testNode2.appendChild(document.createTextNode("P4 "));
    testNode2.appendChild(document.createTextNode("P5 "));
    if (testNode2.childNodes.length != 3) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    var range = document.createRange();
    range.setStart(testNode1, 0);
    range.setEnd(testNode2, 3);

    range.selectNode(testNode1);
    if (range.startContainer != document.body) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.startOffset != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.endContainer != document.body) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.endOffset != 2) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   Range06();
}
