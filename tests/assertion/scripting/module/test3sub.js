import { testImport } from "./import2.js"

testAssert(order == 1)
order++;
console.log("test3 sub");

export { testImport };
