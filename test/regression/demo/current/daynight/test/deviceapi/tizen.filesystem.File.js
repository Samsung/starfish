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

    var fsp_fs = require('fs');
    var fs_path = require('path');
    var CommonFS = require('./tizen.filesystem.Common');
    var DLog = require('./tizen.filesystem.DLog');
    var Encoding = require('./tizen.filesystem.Encoding');
    var FileStream = require('./tizen.filesystem.FileStream');
    var Mode = require('./tizen.filesystem.Mode');

    var _common = require('./tizen.Common');
    var T = _common.Type;
    var AV = _common.ArgumentValidator;
    var C = _common.Common;
    _common = undefined;

    // TODO WebAPIError should be passed to callbacks instead of WebAPIException

    function _mkdirParent(dirPath, mode){
        try {
            fsp_fs.mkdirSync(dirPath, mode);
        } catch (error) {
            if (error && error.errno === 34) {
                //Create all the parents recursively
                _mkdirParent(fs_path.dirname(dirPath), mode);
                _mkdirParent(dirPath, mode);
            }
        }
    };

    // class File //////////////////////////////////////////////////////
    function File(data) {
        DLog.fslogd('Entered: File()');

        function fileSizeGetter() {
            var _realPath = CommonFS.toRealPath(this.fullPath);
            var aStatObj = fsp_fs.statSync(_realPath);
            return aStatObj.isFile() ? aStatObj.size : undefined;
        }

        Object.defineProperties(this, {
            parent : {
                value : (function(data) {
                    try {
                        if (data.parent) { // prevent recursive - only one parent
                            var _parentPath = data.path.substr(0, data.path.length -1);
                            var _statObj = fsp_fs.statSync(CommonFS.toRealPath(_parentPath));
                            var _info = CommonFS.getFileInfo(_parentPath, _statObj, true);
                            return new File(_info);
                        } else {
                            return null;
                        }
                    } catch (err) {
                        DLog.fsloge(err.name, err.message);
                        return null;
                    }
                }(data)),
                writable : false,
                enumerable : true
            },
            readOnly : {value: data.readOnly, writable: false, enumerable: true},
            isFile : {value: data.isFile, writable: false, enumerable: true},
            isDirectory : {value: data.isDirectory, writable: false, enumerable: true},
            created : {value: data.created, writable: false, enumerable: true},
            modified : {value: data.modified, writable: false, enumerable: true},
            path : {value: data.path, writable: false, enumerable: true},
            name : {value: data.name, writable: false, enumerable: true},
            fullPath : {value: data.fullPath, writable: false, enumerable: true},
            fileSize : {enumerable: true, set : function(){}, get : fileSizeGetter},
            length : {value: data.length, writable: false, enumerable: true},
            mode : {value: data.mode, writable: false},
            f_isSubDir : {value: function(fullPathToCheck){
                return (-1 !== fullPathToCheck.indexOf(CommonFS.toRealPath(this.fullPath)));
            }, writable: false},
            f_isCorrectRelativePath: { value : function(relativePath){
                return ((-1 === relativePath.indexOf('/')) &&
                        (-1 === relativePath.indexOf('\\')) &&
                        (-1 === relativePath.indexOf('?')) &&
                        (-1 === relativePath.indexOf('*')) &&
                        (-1 === relativePath.indexOf(':')) &&
                        (-1 === relativePath.indexOf('"')) &&
                        (-1 === relativePath.indexOf('<')) &&
                        (-1 === relativePath.indexOf('>')));
            }}
        });
    }

    File.prototype.toURI = function() {
        DLog.fslogd('Entered: File.toURI()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        return 'file://' + CommonFS.toRealPath(this.fullPath);
    };

    function stringToRegex(str) {
        var _regString = '^';
        if (str === '') {
            return new RegExp(_regString, 'i');
        }

        var _percentTokens = str.split('%');
        var i;
        for (i = 0; i < _percentTokens.length-1; ++i) {
            _regString = _regString + _percentTokens[i];
            if (_regString[_regString.length-1] == '\\') {
                _regString = _regString.replace('\\', '%');
            }
            else if (_regString.lastIndexOf('*') != _regString.length-1) {
                _regString = _regString + '.*';
            }
        }

        return new RegExp(_regString + _percentTokens[i] + '$', 'i');
    }

    function createFilter(fileFilter) {
        if (T.isNull(fileFilter)) {
            return null;
        }

        var _fileFilter = {},
            _anyField = false;

        if (!T.isNullOrUndefined(fileFilter.name)) {
            _anyField = true;
            _fileFilter.name = stringToRegex(fileFilter.name);
        }

        if (!T.isNullOrUndefined(fileFilter.startModified)) {
            if (!(fileFilter.startModified instanceof Date)) {
                DLog.fslogd('Invalid date');
                C.throwTypeMismatch('Invalid date');
            } else {
                _anyField = true;
                _fileFilter.startModified = fileFilter.startModified;
            }
        }

        if (!T.isNullOrUndefined(fileFilter.endModified)) {
            if (!(fileFilter.endModified instanceof Date)) {
                DLog.fslogd('Invalid date');
                C.throwTypeMismatch('Invalid date');
            } else {
                _anyField = true;
                _fileFilter.endModified = fileFilter.endModified;
            }
        }

        if (!T.isNullOrUndefined(fileFilter.startCreated)) {
            if (!(fileFilter.startCreated instanceof Date)) {
                DLog.fslogd('Invalid date');
                C.throwTypeMismatch('Invalid date');
            } else {
                _anyField = true;
                _fileFilter.startCreated = fileFilter.startCreated;
            }
        }

        if (!T.isNullOrUndefined(fileFilter.endCreated)) {
            if (!(fileFilter.endCreated instanceof Date)) {
                DLog.fslogd('Invalid date');
                C.throwTypeMismatch('Invalid date');
            } else {
                _anyField = true;
                _fileFilter.endCreated = fileFilter.endCreated;
            }
        }

        return _anyField ? _fileFilter : null;
    }

    function matchRange(value, min, max) {
        if (min !== undefined && value < min) {
            return false;
        }
        if (max !== undefined && value > max) {
            return false;
        }
        return true;
    }

    function matchName(value, filter_name) {
        if (filter_name === undefined || filter_name.test(value)) {
            return true;
        }
        return false;
    }

    function checkFile(file, fileFilter) {
        if (!matchName(file.name, fileFilter.name)) {
            return false;
        }

        if (!matchRange(file.modified, fileFilter.startModified, fileFilter.endModified)) {
            return false;
        }

        if (!matchRange(file.created, fileFilter.startCreated, fileFilter.endCreated)) {
            return false;
        }

        return true;
    }

    File.prototype.listFiles = function listFiles() {
        DLog.fslogd('Entered: File.listFiles()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        var args = AV.validateMethod(arguments, [
            {
                name : 'onsuccess',
                type : AV.Types.FUNCTION
            },
            {
                name : 'onerror',
                type : AV.Types.FUNCTION,
                optional : true,
                nullable : true
            },
            {
                name : 'filter',
                type : AV.Types.DICTIONARY,
                optional : true,
                nullable : true
            }
        ]);

        if (!this.isDirectory) {
            DLog.fsloge('File object which call this method is not directory');
            setTimeout(function() {
                C.callIfPossible(args.onerror,
                        C.getIOError('File object which call this method is not directory'));
            }, 0);
            return;
        }

        var _fileFilter = null;

        if (args.has.filter) {
            _fileFilter = createFilter(args.filter);
        }

        var _myPath = this.fullPath;
        var _realMyPath = CommonFS.toRealPath(_myPath);

        fsp_fs.readdir(
                _realMyPath,
                function(aError, aFiles) {
                    var _result = [],
                        i,
                        _resolvedPath,
                        _statObj,
                        _fileInfo;
                    if (aError) {
                        //TODO: Add error translation
                        args.onerror(aError);
                    } else {
                        //FIXME: Implement asynchronous loop
                        for (i = 0; i < aFiles.length; ++i) {
                            try {
                                _resolvedPath = _myPath + '/' + aFiles[i];
                                _statObj = fsp_fs.statSync(_realMyPath + '/' + aFiles[i]);
                                _fileInfo = CommonFS.getFileInfo(_resolvedPath, _statObj);
                                if (_fileFilter === null) {
                                    _result.push(new File(_fileInfo));
                                } else if (checkFile(_fileInfo, _fileFilter)) {
                                    _result.push(new File(_fileInfo));
                                }
                            } catch(err) {
                                DLog.fsloge('Error: ', err.name);
                            }
                        }
                        args.onsuccess(_result);
                    }
                }
        );
    };

    // openStream costants

    File.prototype.openStream = function() {
        DLog.fslogd('Entered: File.openStream()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        var args = AV.validateMethod(arguments, [
            {
                name : 'mode',
                type : AV.Types.ENUM,
                values : Mode.getModes()
            },
            {
                name : 'onsuccess',
                type : AV.Types.FUNCTION
            },
            {
                name : 'onerror',
                type : AV.Types.FUNCTION,
                optional : true,
                nullable : true
            },
            {
                name : 'encoding',
                type : AV.Types.STRING,
                optional : true,
                nullable : true
            }
        ]);

        // TODO should FileMode be visible to user? (see WIDL)

        if (this.mode === 'r' && args.mode !== 'r') {
            DLog.fsloge('Read only mode, but asked: ' + args.mode);
            setTimeout(function() {
                C.callIfPossible(args.onerror, C.getInvalidValues('Read only mode'));
            }, 0);
            return;
        }

        if (this.isDirectory) {
            var directoryMessage = 'This method should be called on file, not directory';
            DLog.fsloge(directoryMessage);
            setTimeout(function() {
                C.callIfPossible(args.onerror, C.getIOError(directoryMessage));
            }, 0);
            return;
        }

        //UTC_filesystem_file_openStream_P_002 test requires default encoding type
        //if encoding parameter is undefined
        if (args.encoding === 'undefined') {
            args.encoding = undefined;
        }

        if (!Encoding.checkIsValid(args.encoding)) {
            var encodingMessage = 'Argument "encoding" has invalid value';
            DLog.fsloge(encodingMessage);
            C.throwTypeMismatch(encodingMessage);
        }

        var nodeMode = Mode.getNodeValue(args.mode);
        var nodeEncoding = Encoding.getNodeValue(args.encoding);

        fsp_fs.open(CommonFS.toRealPath(this.fullPath), nodeMode, function (error, fileDescriptor) {
            if (error) {
                var m = 'File not found.';
                DLog.fsloge(m);
                C.callIfPossible(args.onerror, C.getIOError(m));
            } else {
                var fileStream = new FileStream(fileDescriptor, nodeMode, nodeEncoding);
                args.onsuccess(fileStream);
            }
        });
    };

    File.prototype.readAsText = function() {
        DLog.fslogd('Entered: File.readAsText()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        var args = AV.validateMethod(arguments, [
            {
                name : 'onsuccess',
                type : AV.Types.FUNCTION
            },
            {
                name : 'onerror',
                type : AV.Types.FUNCTION,
                optional : true,
                nullable : true
            },
            {
                name : 'encoding',
                type : AV.Types.STRING,
                optional : true,
                nullable : true
            }
        ]);

        if (this.isDirectory) {
            DLog.fsloge('File object which call this method is directory');
            setTimeout(function() {
                C.callIfPossible(args.onerror,
                        C.getIOError('File object which call this method is directory'));
            }, 0);
            return;
        }

        // 104857600 = 100MB
        if (this.fileSize > 104857600) {
            DLog.fsloge('File is too large');
            C.throwNotSupported('Text is too big');
        }

        if (!Encoding.checkIsValid(args.encoding)) {
            var encodingMessage = 'Argument "encoding" has invalid value';
            DLog.fsloge(encodingMessage);
            C.throwTypeMismatch(encodingMessage);
        }

        var nodeEncoding = Encoding.getNodeValue(args.encoding);
        var source = fsp_fs.createReadStream(CommonFS.toRealPath(this.fullPath));

        try {
            source.setEncoding(nodeEncoding);
        } catch (err) {
            var encodingMessage = 'Unsupported encoding type:' + args.encoding;
            DLog.fsloge(encodingMessage);
            C.throwNotSupported(encodingMessage);
        }

        var strValue = '';
        source.on('end', function(){
            C.callIfPossible(args.onsuccess, strValue);
        });

        source.on('data', function(chunk){
            strValue += chunk.toString();
        });

        source.on('error', function(err){
            DLog.fsloge(err.name, err.message);
            C.callIfPossible(args.onerror, CommonFS.translateNodeErrToWebAPIException(err,
                    'Problem during readAsText function', true));
        });
    };

    File.prototype.copyTo = function() {
        DLog.fslogd('Entered: File.copyTo()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_WRITE);

        var args = AV.validateMethod(arguments, [
            {
                name : 'originFilePath',
                type : AV.Types.STRING
            },
            {
                name : 'destinationFilePath',
                type : AV.Types.STRING
            },
            {
                name : 'overwrite',
                type : AV.Types.BOOLEAN
            },
            {
                name : 'onsuccess',
                type : AV.Types.FUNCTION,
                optional : true,
                nullable : true
            },
            {
                name : 'onerror',
                type : AV.Types.FUNCTION,
                optional : true,
                nullable : true
            }
        ]);

        if (this.isFile) {
            DLog.fsloge('File object which call this method is not directory');
            setTimeout(function() {
                C.callIfPossible(args.onerror,
                        C.getIOError('File object which call this method is not directory'));
            }, 0);
            return;
        }

        var _realOriginalPath = CommonFS.toRealPath(args.originFilePath);
        var _realDestinationPath = CommonFS.toRealPath(args.destinationFilePath);

        //check if origin path is under current directory
        if (!this.f_isSubDir(_realOriginalPath)) {
            var m = 'Source file should be subdirectory of: ' + this.fullPath;
            DLog.fsloge(m);
            setTimeout(function() {
                C.callIfPossible(args.onerror, C.getNotFound(m));
            }, 0);
            return;
        }

        //check destination path, it cannot be a file and has to exist.
        var _statObj;
        var _filename;
        try {
            _statObj = fsp_fs.statSync(_realDestinationPath);
        } catch(err){
            try {
                _filename = fs_path.basename(_realDestinationPath);
                _realDestinationPath = fs_path.dirname(_realDestinationPath) + '/';
                _statObj = fsp_fs.statSync(_realDestinationPath);
            } catch (err) {
                var m1 = 'Destination folder does not exist.';
                DLog.fsloge(m1);
                setTimeout(function() {
                    C.callIfPossible(args.onerror, C.getInvalidValues(m1));
                }, 0);
                return;
            }
        }

        //IO Error if destination folder exists and overwrite parameter is set to false
        if (!_filename && (_statObj.isDirectory() && 
                !args.overwrite || this.f_isSubDir(_realDestinationPath))) {
            DLog.fsloge('Incorrect folder or permissions');
            setTimeout(function() {
                C.callIfPossible(args.onerror,
                        C.getIOError('Incorrect folder or permissions'));
            }, 0);
            return;
        }

        var _isDestAllowed = CommonFS.isLocationAllowed(_realDestinationPath);
        if (_statObj.isFile() || !_isDestAllowed) {
            var m2 = 'Destination path should be directory: ' + this.fullPath;
            DLog.fsloge(m2);
            setTimeout(function() {
                C.callIfPossible(args.onerror, C.getInvalidValues(m2));
            }, 0);
            return;
        }

        //additional function used to recursive copying
        var _fileList = [];
        var _node;

        var _copyEnd = function(aPath) {
            aPath = CommonFS.toRealPath(aPath);
            fsp_fs.stat(aPath, function (aErr, aStatObj) {
                if (aErr) {
                    C.callIfPossible(args.onerror, C.getUnknownError());
                } else {
                    var _res = CommonFS.getFileInfo(aPath, aStatObj, false, aStatObj.mode);
                    C.callIfPossible(args.onsuccess, new File(_res));
                }
            });
        };

        var _createDir = function(aPath) {
            try {
                if(!fsp_fs.existsSync(aPath)) {
                    fsp_fs.mkdirSync(aPath);
                }
                return true;
            } catch (err) {
                DLog.fsloge('Creating folder failed: ', err.name);
                return false;
            }
        };

        var _copyTask = function() {
            var _childNode;
            if (_fileList.length) {
                _childNode = _fileList[0];
                _fileList.splice(0,1);
                if (_childNode.isFile) {
                    _node.copyTo(_childNode.fullPath, args.destinationFilePath,
                            args.overwrite, _copyTask, args.onerror);
                } else {
                    var _realDestPath = _realDestinationPath + '/' + _childNode.name;
                    if (_createDir(_realDestPath)) {
                        _node.copyTo(_childNode.fullPath, _realDestPath,
                                args.overwrite, _copyTask, args.onerror);
                    }
                    else {
                        setTimeout(function() {
                            C.callIfPossible(args.onerror, C.getUnknownError());
                        }, 0);
                        return;
                    }
                }
            } else {
                _copyEnd(args.destinationFilePath);
            }
        };

        var _fileListSuccess = function(aFiles) {
            if (aFiles.length > 0) {
                _fileList = aFiles;
                _copyTask();
            } else {
                _copyEnd(_realDestinationPath);
            }
        };

        var _fileListError = function(aErr) {
            DLog.fslogd(aErr.name, aErr.message);
            setTimeout(function(){
                C.callIfPossible(args.onerror, C.getUnknownError());
            }, 0);
            return;
        };

        fsp_fs.stat(_realOriginalPath, function (aErr, aStatObj){
            var _result;
            if (aErr) {
                DLog.fsloge('Get source folder/file failed');
                setTimeout(function() {
                    C.callIfPossible(args.onerror,
                            C.getIOError('Get source folder/file failed'));
                }, 0);
                return;
            } else {
                _result = CommonFS.getFileInfo(_realOriginalPath, aStatObj, false, aStatObj.mode);
                _node = new File(_result);

                if (_node.isFile) {
                    if (!args.overwrite && fsp_fs.existsSync(_realDestinationPath)) {
                        setTimeout(function() {
                            C.callIfPossible(args.onerror,
                                    C.getIOError('Overwrite not allowed'));
                        }, 0);
                        return;
                    }

                    var source = fsp_fs.createReadStream(_realOriginalPath);
                    var filename = _filename ?
                            _filename : _realOriginalPath.substr(_realOriginalPath.lastIndexOf('/'));
                    var destination = fsp_fs.createWriteStream(_realDestinationPath + filename);

                    source.on('end', function(){
                        _copyEnd(_realDestinationPath + filename);
                    });

                    source.on('error', function(err){
                        DLog.fsloge(err.name, err.message);
                        setTimeout(function() {
                            C.callIfPossible(args.onerror, C.getUnknownError());
                        }, 0);
                        return;
                    });
                    source.pipe(destination);
                } else {
                    _node.listFiles (_fileListSuccess, _fileListError);
                }
            }
        });
    };

    File.prototype.moveTo = function moveTo() {
        DLog.fslogd('Entered: File.moveTo()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_WRITE);

        var args = AV.validateMethod(arguments, [
            {
                name : 'originFilePath',
                type : AV.Types.STRING
            },
            {
                name : 'destinationFilePath',
                type : AV.Types.STRING
            },
            {
                name : 'overwrite',
                type : AV.Types.BOOLEAN
            },
            {
                name : 'onsuccess',
                type : AV.Types.FUNCTION,
                optional : true,
                nullable : true
            },
            {
                name : 'onerror',
                type : AV.Types.FUNCTION,
                optional : true,
                nullable : true
            }
        ]);

        if (this.isFile) {
            DLog.fsloge('File object which call this method is not directory');
            setTimeout(function() {
                C.callIfPossible(args.onerror,
                        C.getIOError('File object which call this method is not directory'));
            }, 0);
            return;
        }

        var _realOriginalPath = CommonFS.toRealPath(args.originFilePath);
        var _realDestinationPath = CommonFS.toRealPath(args.destinationFilePath);

        if (!fsp_fs.existsSync(_realOriginalPath)) {
            C.callIfPossible(args.onerror, C.getNotFound('Source file is not avalaible'));
            return;
        }

        if (!args.overwrite) {
            if (fsp_fs.existsSync(_realDestinationPath)) {
                DLog.fsloge('Overwrite is not allowed');
                C.callIfPossible(args.onerror, C.getIOError('Overwrite is not allowed'));
                return;
            }
        }

        if (!this.f_isSubDir(_realOriginalPath)) {
            var m1 = 'Source file should be subdirectory of: ' + this.fullPath;
            DLog.fsloge(m1);
            C.callIfPossible(args.onerror, C.getInvalidValues(m1));
            return;
        }

        if (this.mode === 'r' || !CommonFS.isLocationAllowed(_realDestinationPath)) {
            var m2 = 'Source/Destination is read only folder: ' + this.fullPath;
            DLog.fsloge(m2);
            setTimeout(function(){
                C.callIfPossible(args.onerror, C.getInvalidValues(m2));
            }, 0);
            return;
        }

        fsp_fs.rename(_realOriginalPath, _realDestinationPath, function(err) {
            if (err) {
                DLog.fsloge(err.name, err.message);
                var callbackError = C.getIOError('Problem during moving function')
                if (err.errno === 34){
                    callbackError = C.getNotFound('Destination path is not valid')
                }

                setTimeout(function() {
                    C.callIfPossible(args.onerror, callbackError);
                }, 0);
                return;
            }

            // TODO documentation say that success callback should be invoke without
            // any perameter
            // UTC test expect the success callback should be called with parameter which should be
            // copied file
            // wrt-plugin implementation call callback with parameter
            var _statObj = fsp_fs.statSync(_realDestinationPath);
            var _fileInfo = CommonFS.getFileInfo(_realDestinationPath, _statObj);
            C.callIfPossible(args.onsuccess, new File(_fileInfo));
        });
    };

    File.prototype.createDirectory = function() {
        DLog.fslogd('Entered: File.createDirectory()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_WRITE);

        var args = AV.validateMethod(arguments, [
            {
                name : 'dirPath',
                type : AV.Types.STRING
            }
        ]);

        var _newPath = this.fullPath + '/' + args.dirPath,
            _statObj,
            _fileInfo,
            _realNewPath = CommonFS.toRealPath(_newPath);

        if (this.isDirectory) {
            if (this.mode === 'r') {
                DLog.fsloge('Invalid path or readonly access');
                C.throwInvalidValues('Invalid path or readonly access');
            }

            _mkdirParent(_realNewPath);
            _statObj = fsp_fs.statSync(_realNewPath);
            _fileInfo = CommonFS.getFileInfo(_realNewPath, _statObj, false, this.mode);
            return new File(_fileInfo);
        } else {
            DLog.fsloge('File object which call this method is not directory');
            C.throwIOError('File object which call this method is not directory');
        }
    };

    File.prototype.createFile = function() {
        DLog.fslogd('Entered: File.createFile()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_WRITE);

        var args = AV.validateMethod(arguments, [
            {
                name : 'relativeFilePath',
                type : AV.Types.STRING
            }
        ]);

        if (this.isFile) {
            DLog.fsloge('File object which call this method is not directory');
            C.throwIOError('File object which call this method is not directory');
        }

        if (!this.f_isCorrectRelativePath(args.relativeFilePath) || this.mode === 'r') {
            DLog.fsloge('Invalid path or readonly acces');
            C.throwInvalidValues('Invalid path or readonly acces');
        }

        var _outputPath = this.fullPath + '/' + args.relativeFilePath;
        var _outputRealPath = CommonFS.toRealPath(_outputPath);
        var _statObj, _fileInfo;

        if (fsp_fs.existsSync(_outputRealPath)) {
            DLog.fsloge('Overwrite is not allowed');
            C.throwIOError('Overwrite is not allowed');
        }

        try {
            fsp_fs.openSync(_outputRealPath, 'w', '0666');
            _statObj = fsp_fs.statSync(_outputRealPath);
            _fileInfo = CommonFS.getFileInfo(_outputPath, _statObj, false, this.mode);
        } catch(err) {
            DLog.fsloge(err.name, err.message);
            CommonFS.translateNodeErrToWebAPIException(err,
                    'Problem during create file', false);
            // TODO: translate error and throw them
        }
        return new File(_fileInfo);
    };

    File.prototype.resolve = function() {
        DLog.fslogd('Entered: File.resolve()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        var args = AV.validateMethod(arguments, [
            {
                name : 'filePath',
                type : AV.Types.STRING
            }
        ]);

        if (this.isFile) {
            DLog.fsloge('File object which call this method is not directory');
            C.throwIOError('File object which call this method is not directory');
        }

        if (!this.f_isCorrectRelativePath(args.filePath)){
            DLog.fsloge('Invalid path');
            C.throwInvalidValues('Invalid path');
        }

        var _newPath = this.fullPath + '/' + args.filePath;
        var _statObj;
        try{
            _statObj = fsp_fs.statSync(CommonFS.toRealPath(_newPath));
        } catch(err){
            DLog.fsloge(err.name, err.message);
            CommonFS.translateNodeErrToWebAPIException(err,
                    'Problem during resolve function', false);
        }
        // TODO move access rights from the File object on which this
        //      resolve() method is called.
        var _fileInfo = CommonFS.getFileInfo(_newPath, _statObj, false, this.mode);
        return new File(_fileInfo);
    };

    File.prototype.deleteDirectory = function() {
        DLog.fslogd('Entered: File.deleteDirectory()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_WRITE);

        var args = AV.validateMethod(arguments, [
            {
                name : 'directoryPath',
                type : AV.Types.STRING
            },
            {
                name : 'recursive',
                type : AV.Types.BOOLEAN
            },
            {
                name : 'onsuccess',
                type : AV.Types.FUNCTION,
                optional : true,
                nullable : true
            },
            {
                name : 'onerror',
                type : AV.Types.FUNCTION,
                optional : true,
                nullable : true
            }
        ]);

        if (this.mode === 'r') {
            DLog.fsloge('Readonly access');
            setTimeout(function() {
                C.callIfPossible(args.onerror,
                        C.getInvalidValues('Invalid path or readonly access'));
            }, 0);
            return;
        }

        DLog.fslogd('Entered: File.deleteDirectory() path [', args.directoryPath, '] for [',
                this.fullPath, ']');
        var _myPath = CommonFS.toRealPath(args.directoryPath);

        fsp_fs.stat(_myPath, function (aErr, aStatObj) {
            var _result, _node;
            var _deleterMain = function () {
                fsp_fs.rmdir(_myPath, function (aErr) {
                    if (aErr) {
                        DLog.fslogd('rmdir failed ', _myPath);
                        //TODO: Error translation
                        C.callIfPossible(args.onerror, C.getNotFound(aErr));
                    } else {
                        C.callIfPossible(args.onsuccess);
                    }
                });
            };
            if (aErr) {
                DLog.fslogd('Stat failed ', _myPath);
                //TODO: Error translation
                C.callIfPossible(args.onerror, C.getNotFound(aErr));
            } else {
                _result = CommonFS.getFileInfo(_myPath, aStatObj);
                _node = new File(_result);
                if (!_node.isDirectory) {
                    var notDirectoryMessage = 'It is file not directory';
                    DLog.fslogd(notDirectoryMessage, _myPath);
                    C.callIfPossible(args.onerror, C.getInvalidValues(notDirectoryMessage));
                } else {
                    _node.listFiles(
                        function (aFiles) {
                            if (aFiles.length > 0) {
                                if (args.recursive) {
                                    var _deleterTask = function () {
                                        var _childNode;
                                        if (aFiles.length) {
                                            _childNode = aFiles[0];
                                            aFiles.splice(0, 1);
                                            if (_childNode.isDirectory) {
                                                _node.deleteDirectory(
                                                    _childNode.fullPath,
                                                    true,
                                                    _deleterTask,
                                                    function (aError) {
                                                        // TODO: Error translation
                                                        C.callIfPossible(args.onerror,
                                                                C.getNotFound(aError));
                                                    }
                                                );
                                            } else {
                                                _node.deleteFile(
                                                    _childNode.fullPath,
                                                    _deleterTask,
                                                    function (aError) {
                                                        // TODO: Error translation
                                                        C.callIfPossible(args.onerror,
                                                                C.getNotFound(aError));
                                                    }
                                                );
                                            }
                                        } else {
                                            _deleterMain();
                                        }
                                    };
                                    _deleterTask();
                                }  else {
                                    var _message = 'Non empty folder ' + _myPath +
                                               ' passed for non recursive delete';
                                    DLog.fslogd(_message);
                                    C.callIfPossible(args.onerror, C.getIOError(_message));
                                }
                            } else {
                                _deleterMain();
                            }
                        },
                        function () {
                            var listFilesFailedMessage = 'List files failed for ' + _myPath;
                            DLog.fslogd(listFilesFailedMessage);
                            C.callIfPossible(args.onerror, C.getIOError(listFilesFailedMessage));
                        }
                    );
                }
            }
        });
    };

    File.prototype.deleteFile = function() {
        DLog.fslogd('Entered: File.deleteFile()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        var args = AV.validateMethod(arguments, [
            {
                name : 'filePath',
                type : AV.Types.STRING
            },
            {
                name : 'onsuccess',
                type : AV.Types.FUNCTION,
                optional : true,
                nullable : true
            },
            {
                name : 'onerror',
                type : AV.Types.FUNCTION,
                optional : true,
                nullable : true
            }
        ]);

        if (this.isFile) {
            DLog.fsloge('File object which call this method is not directory');
            setTimeout(function() {
                C.callIfPossible(args.onerror,
                        C.getIOError('File object which call this method is not directory'));
            }, 0);
            return;
        }

        DLog.fslogd('Entered: File.deleteFile() path [', args.filePath, '] for [',
                this.fullPath, ']');

        var _fileRealPath = CommonFS.toRealPath(args.filePath);

        // TODO Fix UTC test
        // documentation say that this error should be provided by errorCallback
        // UTC test expect that this error should be thrown
        try {
            var _statObj = fsp_fs.statSync(_fileRealPath);
            if (_statObj.isDirectory()) {
                var message = 'Requested object is a directory.';
                DLog.fsloge(message);
                setTimeout(function() {
                    C.callIfPossible(args.onerror, C.getInvalidValues(message));
                }, 0);
                return;
            }
        } catch (err) {
            DLog.fsloge(err.name, err.message);
            setTimeout(function() {
                C.callIfPossible(args.onerror, C.getNotFound('File is not avalaible'));
            }, 0);
            return;
        }

        if (!this.f_isSubDir(_fileRealPath) || this.mode === 'r') {
            var _message = 'Deleted file [' + args.filePath + '] should have write access ' +
                           'and should be subdirectory of: [' + this.fullPath + ']';
            DLog.fsloge(_message);
            setTimeout(function() {
                C.callIfPossible(args.onerror, C.getInvalidValues(_message));
            }, 0);
            return;
        }

        if (_fileRealPath.split('/').pop() === ''){ // in case when file is directory
            DLog.fsloge('This function it is not able to delete directories');
            C.throwInvalidValues('This function it is not able to delete directories');
        }

        fsp_fs.unlink(_fileRealPath, function(err) {
            if (err) {
                DLog.fsloge(err.name, err.message);
                // return exception object and pass them to error callback
                C.callIfPossible(args.onerror, CommonFS.translateNodeErrToWebAPIException(err,
                        'Problem during delete', true));
                return;
            }
            C.callIfPossible(args.onsuccess);
        });
    };

    module.exports = File;
})();
