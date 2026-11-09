var buffer = new ArrayBuffer(10);
var arr = new Int8Array(buffer, 3);
arr.forEach((val, i) => arr[i] = i > 0 ? arr[i-1] + 1 : 1);

var originalMessage = {
  'int8': new Int8Array([10, -20, 30]),
  'uint8': new Uint8Array([10, 20, 30]),
  'int16': new Int16Array([1000, -2000, 3000]),
  'uint16': new Uint16Array([1000, 2000, 3000]),
  'int32': new Int32Array([1000000, -2000000, 3000000]),
  'uint32': new Uint32Array([1000000, 2000000, 3000000]),
  'offset': arr
};
