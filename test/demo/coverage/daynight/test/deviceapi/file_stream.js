/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

function FileStream(data, mode, encoding) {
  var _totalBytes = data.fileSize || 0;
  var _position = mode === 'a' ? _totalBytes : 0;

  Object.defineProperties(this, {
    eof: {
      value: false,
      enumerable: true,
      writable: false
    },
    position: {
      get: function() {
        return _position;
      },
      set: function(v) {
        _position = Math.max(0, v);
      },
      enumerable: true
    },
    bytesAvailable: {
      value: this.eof ? -1 : Math.max(0, _totalBytes - _position),
      enumerable: true,
      writable: false
    },
    _mode: {
      value: mode,
      writable: false,
      enumerable: false
    },
    _encoding: {
      value: encoding,
      writable: false,
      enumerable: false
    },
    _file: {
      value: data,
      writable: false,
      enumerable: false
    },
    _closed: {
      value: false,
      writable: true,
      enumerable: false
    }
  });
}

function _checkClosed(stream) {
  if (stream._closed) {
    throw new WebAPIException(WebAPIException.IO_ERR, 'Stream is closed.');
  }
}

FileStream.prototype.close = function() {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);
  this._closed = true;
};

function _checkReadAccess(mode) {
  if (mode !== 'r' && mode !== 'rw') {
    throw new WebAPIException(WebAPIException.IO_ERR, 'Stream is not in read mode.');
  }
}

function _checkWriteAccess(mode) {
  if (mode !== 'a' && mode !== 'w' && mode !== 'rw') {
    throw new WebAPIException(WebAPIException.IO_ERR, 'Stream is not in write mode.');
  }
}

FileStream.prototype.read = function() {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);

  var args = validator_.validateArgs(arguments, [
    {
      name: 'charCount',
      type: types_.LONG
    }
  ]);

  _checkClosed(this);
  _checkReadAccess(this._mode);

  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
        'Argument "charCount" missing');
  }
  if (!type_.isNumber(args.charCount)) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Argument "charCount" must be a number');
  }
  if (args.charCount <= 0) {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
        'Argument "charCount" must be greater than 0');
  }

  var _count = this.bytesAvailable;

  var data = {
    location: commonFS_.toRealPath(this._file.fullPath),
    offset: this.position || 0,
    length: args.charCount,// > _count ? _count : args.charCount,
    is_base64: false,
  };

  var result = native_.callSyncData('File_readSync', data);
  if (native_.isFailure(result)) {
    throw new WebAPIException(WebAPIException.IO_ERR, 'Could not read');
  }
  this.position = this.position + result.data_size;

  return result.content;
};

FileStream.prototype.readBytes = function() {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);

  var args = validator_.validateArgs(arguments, [
    {
      name: 'byteCount',
      type: types_.LONG
    }
  ]);

  _checkClosed(this);
  _checkReadAccess(this._mode);

  if (args.byteCount <= 0) {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
        'Argument "byteCount" must be greater than 0');
  }

  var _count = this.bytesAvailable;

  var data = {
    location: commonFS_.toRealPath(this._file.fullPath),
    offset: this.position || 0,
    length: args.byteCount > _count ? _count : args.byteCount,
    is_base64: false,
  };

  var result = native_.callSyncData('File_readSync', data, "octet");
  if (native_.isFailure(result.reply)) {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR, 'Could not read');
  }

  this.position = this.position + result.reply.data_size;

  return result.output;
};

FileStream.prototype.readBase64 = function() {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);

  var args = validator_.validateArgs(arguments, [
    {
      name: 'byteCount',
      type: types_.LONG
    }
  ]);

  _checkClosed(this);
  _checkReadAccess(this._mode);

  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Argument "byteCount" missing');
  }
  if (type_.isString(arguments[0]) && !arguments[0].length) {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
        'Argument "byteCount" must be a number');
  }
  if (!type_.isNumber(arguments[0])) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Argument "byteCount" must be a number');
  }
  if (args.byteCount <= 0) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Argument "byteCount" must be greater than 0');
  }

  var _count = this.bytesAvailable;

  var data = {
    location: commonFS_.toRealPath(this._file.fullPath),
    offset: this.position || 0,
    length: args.byteCount > _count ? _count : args.byteCount,
    is_base64: true
  };

  var result = native_.callSyncData('File_readSync', data);
  if (native_.isFailure(result.reply)) {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR, 'Could not read');
  }
  this.position = this.position + result.reply.data_size;

  return result.output;
};

FileStream.prototype.write = function() {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_WRITE);

  var args = validator_.validateArgs(arguments, [
    {
      name: 'stringData',
      type: types_.STRING
    }
  ]);

  _checkClosed(this);
  _checkWriteAccess(this._mode);
  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.NOT_FOUND_ERR,
        'Argument "stringData" missing');
  }

  var data = {
    location: commonFS_.toRealPath(this._file.fullPath),
    offset: this.position,
    length: args.stringData.length,
    is_base64: false,
    content: args.stringData,
  };
  var result = native_.callSyncData('File_writeSync', data, "string", args.stringData);
  if (native_.isFailure(result.reply)) {
    throw new WebAPIException(WebAPIException.IO_ERR, 'Could not write');
  }
  this.position = this.position + result.reply.data_size;
};

FileStream.prototype.writeBytes = function() {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_WRITE);

  var args = validator_.validateArgs(arguments, [
    {
      name: 'byteData',
      type: types_.ARRAY
    }
  ]);
  _checkClosed(this);
  _checkWriteAccess(this._mode);
  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Argument "byteData" missing');
  }

  var data = {
    location: commonFS_.toRealPath(this._file.fullPath),
    offset: this.position,
    length: args.byteData.length,
    is_base64: false,
  };

  var result = native_.callSyncData('File_writeSync', data, "octet", args.byteData);

  if (native_.isFailure(result.reply)) {
    throw new WebAPIException(WebAPIException.IO_ERR, 'Could not write');
  }
  this.position = this.position + result.reply.data_size;

};

function _isBase64(str) {
  var base64 = new RegExp('^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?$');
  return base64.test(str);
}

FileStream.prototype.writeBase64 = function() {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_WRITE);

  var args = validator_.validateArgs(arguments, [
    {
      name: 'base64Data',
      type: types_.STRING
    }
  ]);

  _checkClosed(this);
  _checkWriteAccess(this._mode);

  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Argument "base64Data" missing');
  }
  if (!args.base64Data.length || !_isBase64(args.base64Data)) {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
        'Data is not base64');
  }

  var data = {
    location: commonFS_.toRealPath(this._file.fullPath),
    offset: this.position,
    is_base64: true
  };

  var result = native_.callSyncData('File_writeSync', data, "string", args.base64Data);

  if (native_.isFailure(result)) {
    throw new WebAPIException(WebAPIException.IO_ERR, 'Could not write');
  }
  this.position = this.position + result.reply.data_size;
};
