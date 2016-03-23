/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd All Rights Reserved
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

(function () {
    'use strict';

    var fs = require('fs');
    var CommonFS = require('./tizen.filesystem.Common');
    var DLog = require('./tizen.filesystem.DLog');

    var _common = require('./tizen.Common');
    var Converter = _common.Converter;
    var AV = _common.ArgumentValidator;
    var C = _common.Common;
    _common = undefined;

    function FileStream(fileDescriptor, nodeMode, nodeEncoding) {
        DLog.fslogd('Entered: FileStream()');

        var stats = fs.fstatSync(fileDescriptor);
        var totalBytes = stats.size;

        // TODO consider doing all position updates through one function

        var position = nodeMode === 'a' ? totalBytes : 0;
        var getPosition = function () {
            return position;
        };
        var setPosition = function (value) {
            position = Math.max(0, value);

            this._updateEof();
        };

        var getBytesAvailable = function () {
            return (this.eof ? - 1 : Math.max(0, totalBytes - position));
        };

        Object.defineProperties(this, {
            // public
            position : {get: getPosition.bind(this), set: setPosition.bind(this), enumerable: true},
            eof : {value: false, writable: false, enumerable: true, configurable: true},
//            bytesAvailable : {value: stats.size, writable: true, enumerable: true},
            bytesAvailable : {get: getBytesAvailable.bind(this), enumerable: true},
            // private
            _totalBytes : {value: totalBytes, writable: false, enumerable: false},
            _fileDescriptor: {value: fileDescriptor, writable: false, enumerable: false},
            _closed: {value: false, writable: true, enumerable: false},
            _nodeMode: {value: nodeMode, writable: false, enumerable: false},
            _nodeEncoding: {value: nodeEncoding, writable: false, enumerable: false},
            _readToBuffer: {value: _readToBufferInternal.bind(this), writable: false,
                    enumerable: false},
            _writeBuffer: {value: _writeBufferInternal.bind(this), writable: false,
                    enumerable: false},
            _updatePosition: {value: _updatePositionInternal.bind(this), writable: false,
                    enumerable: false},
            _updateEof: {value: _updateEofInternal.bind(this), writable: false,
                    enumerable: false}
        });

        this._updateEof();
    }

    FileStream.prototype.close = function () {
        DLog.fslogd('Entered: FileStream.close()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        if (!this._closed) {
            try {
                fs.closeSync(this._fileDescriptor);
            } catch (e) {
                CommonFS.translateNodeErrToWebAPIException(e, 'Failed to close stream.');
            }
            this._closed = true;
        }
    };

    function _checkReadAccess(mode) {
        DLog.fslogd('Entered: _checkReadAccess(): ' + mode);

        if (mode !== 'r' && mode !== 'r+') {
            C.throwIOError('Stream is not in read mode.');
        }
    }

    function _checkWriteAccess(mode) {
        DLog.fslogd('Entered: _checkWriteAccess(): ' + mode);

        if (mode !== 'a' && mode !== 'w' && mode !== 'r+') {
            C.throwIOError('Stream is not in write mode.');
        }
    }

    FileStream.prototype.read = function () {
        DLog.fslogd('Entered: FileStream.read()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        var args = AV.validateMethod(arguments, [
            {
                name : 'charCount',
                type : AV.Types.LONG
            }
        ]);

        _checkReadAccess(this._nodeMode);

        if (args.charCount <= 0) {
            C.throwInvalidValues('Argument "charCount" must be greater than 0');
        }

        return this._readToBuffer(args.charCount).toString(this._nodeEncoding);
    };

    FileStream.prototype.readBytes = function () {
        DLog.fslogd('Entered: FileStream.readBytes()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        var args = AV.validateMethod(arguments, [
            {
                name : 'byteCount',
                type : AV.Types.LONG
            }
        ]);

        _checkReadAccess(this._nodeMode);

        if (args.byteCount <= 0) {
            C.throwInvalidValues('Argument "byteCount" must be greater than 0');
        }

        var buffer = this._readToBuffer(args.byteCount);

        var octetArray = [];
        for (var i = 0; i < buffer.length; i++) {
            octetArray[i] = Converter.toOctet(buffer[i]);
        }

        return octetArray;
    };

    FileStream.prototype.readBase64 = function () {
        DLog.fslogd('Entered: FileStream.readBase64()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        var args = AV.validateMethod(arguments, [
            {
                name : 'byteCount',
                type : AV.Types.LONG
            }
        ]);

        _checkReadAccess(this._nodeMode);

        if (args.byteCount <= 0) {
            C.throwInvalidValues('Argument "byteCount" must be greater than 0');
        }

        return this._readToBuffer(args.byteCount).toString('base64');
    };

    FileStream.prototype.write = function () {
        DLog.fslogd('Entered: FileStream.write()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_WRITE);

        var args = AV.validateMethod(arguments, [
            {
                name : 'stringData',
                type : AV.Types.STRING
            }
        ]);

        _checkWriteAccess(this._nodeMode);

        var buffer = new Buffer(args.stringData, this._nodeEncoding);
        this._writeBuffer(buffer);
    };

    FileStream.prototype.writeBytes = function () {
        DLog.fslogd('Entered: FileStream.writeBytes()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_WRITE);

        var args = AV.validateMethod(arguments, [
            {
                name : 'byteData',
                type : AV.Types.ARRAY,
                values : AV.Types.OCTET
            }
        ]);

        _checkWriteAccess(this._nodeMode);

        var buffer = new Buffer(args.byteData);
        this._writeBuffer(buffer);
    };

    FileStream.prototype.writeBase64 = function () {
        DLog.fslogd('Entered: FileStream.writeBase64()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_WRITE);

        var args = AV.validateMethod(arguments, [
            {
                name : 'base64Data',
                type : AV.Types.STRING
            }
        ]);

        _checkWriteAccess(this._nodeMode);

        var buffer = new Buffer(args.base64Data, 'base64');
        this._writeBuffer(buffer);
    };

    function _readToBufferInternal(length) {
        /*jshint validthis:true */
        DLog.fslogd('Entered: _readToBufferInternal()');

        var buffer = new Buffer(length);
        var bytesRead;

        try {
            bytesRead = fs.readSync(this._fileDescriptor, buffer, 0, length, this.position);
        } catch (e) {
            CommonFS.translateNodeErrToWebAPIException(e, 'Failed to read from stream.');
        }
        this._updatePosition(bytesRead);

        return buffer.slice(0, bytesRead);
    }

    function _writeBufferInternal(buffer) {
        /*jshint validthis:true */
        DLog.fslogd('Entered: _writeBufferInternal()');

        var bytesWritten;

        // TODO on Linux, positional writes don't work when the file is opened in append mode.
        // The kernel ignores the position argument and always appends the data to the end of the
        // file
        try {
            bytesWritten = fs.writeSync(this._fileDescriptor, buffer, 0, buffer.length,
                    this.position);
        } catch (e) {
            CommonFS.translateNodeErrToWebAPIException(e, 'Failed to write to stream.');
        }
        this._updatePosition(bytesWritten);
    }

    function _updatePositionInternal(positionDelta) {
        /*jshint validthis:true */
        DLog.fslogd('Entered: _updatePositionInternal()');

        // TODO If the r/w operation is not successful, the position of the stream is unchanged
        // hopefully this should be handled by the fact that bytes read/write will be 0
        this.position += positionDelta;
        this._updateEof();
    }

    function _updateEofInternal() {
        /*jshint validthis:true */
        DLog.fslogd('Entered: _updateEofInternal()');

        if (this.position > this._totalBytes) {
            Object.defineProperty(this, 'eof', {value: true});
        } else {
            Object.defineProperty(this, 'eof', {value: false});
        }
    }

    module.exports = FileStream;
})();
