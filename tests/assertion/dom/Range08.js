/**
*
    Retrieve the host attribute and examine its value.
*/
function Range08() {
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

    var sourceRange1 = document.createRange();
    var sourceRange2 = document.createRange();
    var sourceRange3 = document.createRange();

    sourceRange1.setStart(testNode1, 0);
    sourceRange1.setEnd(testNode1, 1);
    sourceRange2.setStart(testNode2, 0);
    sourceRange2.setEnd(testNode2, 3);
    sourceRange3.setStart(testNode1.childNodes[0], 0);
    sourceRange3.setEnd(testNode1.childNodes[1], 0);

    if (range.compareBoundaryPoints(0, sourceRange1) != 0) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.compareBoundaryPoints(1, sourceRange1) != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.compareBoundaryPoints(2, sourceRange1) != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.compareBoundaryPoints(3, sourceRange1) != -1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    if (range.compareBoundaryPoints(0, sourceRange2) != -1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.compareBoundaryPoints(1, sourceRange2) != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.compareBoundaryPoints(2, sourceRange2) != 0) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.compareBoundaryPoints(3, sourceRange2) != -1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    if (range.compareBoundaryPoints(0, sourceRange3) != -1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.compareBoundaryPoints(1, sourceRange3) != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.compareBoundaryPoints(2, sourceRange3) != 1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    if (range.compareBoundaryPoints(3, sourceRange3) != -1) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   Range08();
}
