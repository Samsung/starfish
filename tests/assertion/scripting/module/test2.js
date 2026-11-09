import { testImport } from "./import.js"


console.log(testImport(3));
testAssert(testImport(3) == 6);
testEnd();
