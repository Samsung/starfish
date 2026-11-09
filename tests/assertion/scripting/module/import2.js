console.log("loading testImport");
testAssert(order == 0)
order++;

function testImport(a) {
    return a + a;
}

export { testImport };
