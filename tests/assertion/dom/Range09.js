/**
*
    Retrieve the host attribute and examine its value.
*/
function Range09() {
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

    var range2 = range.cloneRange();
    if (range2.startContainer != testNode1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    if (range2.endContainer != testNode2) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    if (range2.startOffset != 0) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    if (range2.endOffset != 3) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   Range09();
}
