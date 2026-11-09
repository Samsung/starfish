/**
*
    Retrieve the host attribute and examine its value.
*/
function Text02() {
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
    testNode1.appendChild(document.createTextNode("P1"));
    if (testNode1.childNodes.length != 2) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    testNode2 = nodeList.item(1);
    testNode2.appendChild(document.createTextNode("P2"));
    if (testNode2.childNodes.length != 2) {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    document.body.normalize();
    if (testNode1.childNodes[0].textContent != "P1P1") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }
    testNode2 = nodeList.item(1);
    if (testNode2.childNodes[0].textContent != "P2P2") {
        console.log("FAIL");
        wptTestEnd();
        return;
    }

    console.log("PASS");
    wptTestEnd();
}

function runTest() {
   Text02();
}
