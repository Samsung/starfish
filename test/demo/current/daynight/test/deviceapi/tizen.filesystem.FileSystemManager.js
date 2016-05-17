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
    var CommonFS = require('./tizen.filesystem.Common');
    var DLog = require('./tizen.filesystem.DLog');
    var File = require('./tizen.filesystem.File');
    var Mode = require('./tizen.filesystem.Mode');

    var _common = require('./tizen.Common');
    var T = _common.Type;
    var Converter = _common.Converter;
    var AV = _common.ArgumentValidator;
    var C = _common.Common;
    var _callSync = C.getCallSync('filesystem');
    _common = undefined;

    var _internalStr = 'INTERNAL';
    var _externalStr = 'EXTERNAL';
    var _mountedStr = 'MOUNTED';
    var _removedStr = 'REMOVED';
    var _unmountableStr = 'UNMOUNTABLE';

    var PATH_MAX = 4096;

    // class FileSystemManager ////////////////////////////////////////////////////
    var FileSystemManager = function() {
        Object.defineProperties(this, {
            maxPathLength : {value: PATH_MAX, writable: false, enumerable: true},
            _isWidgetPathFound : {value: false, writable: true}
        });
    };

    function _toVirtualPath(aPath) {
        var _virtualPath = aPath;
        if( _virtualPath.indexOf('file://') === 0 ){
            _virtualPath = _virtualPath.substr('file://'.length);
        }
        for(var real_path in CommonFS.cacheRealToVirtual) {
            if( _virtualPath.indexOf(real_path) === 0) {
                return _virtualPath.replace(
                        real_path,
                        CommonFS.cacheRealToVirtual[real_path]);
            }
        }

        return aPath;
    }

    function _initializeCache(manager) {
        if (!manager._isWidgetPathFound) {
            // Disable for PC testing
            var result = _callSync('Filesystem_getWidgetPaths', {});
            //
            // Enable for PC testing
            // var result = {"wgt-package": "/tmp",
            //               "wgt-private": "/tmp",
            //               "wgt-private-tmp": "/tmp"};
            // Disable for PC testing
            if (C.isFailure(result)) {
                throw C.getErrorObject(result);
            }

            CommonFS.cacheVirtualToReal['wgt-package'] = {
                    path: result['wgt-package'],
                    type: _internalStr,
                    state: _mountedStr
            };
            CommonFS.cacheVirtualToReal['wgt-private'] = {
                    path: result['wgt-private'],
                    type: _internalStr,
                    state: _mountedStr
            };
            CommonFS.cacheVirtualToReal['wgt-private-tmp'] = {
                    path: result['wgt-private-tmp'],
                    type: _internalStr,
                    state: _mountedStr
            };

            CommonFS.cacheRealToVirtual[result['wgt-package']] = 'wgt-package';
            CommonFS.cacheRealToVirtual[result['wgt-private']] = 'wgt-private';
            CommonFS.cacheRealToVirtual[result['wgt-private-tmp']] = 'wgt-private-tmp';

            var result = _callSync('FileSystemManager_fetchStorages', {});
            if (C.isFailure(result)) {
                throw C.getErrorObject(result);
            }
            console.log(result);
            var data = C.getResultObject(result);
            data.forEach(function (i) {
                CommonFS.cacheVirtualToReal[i.name] = {
                        path: i.path,
                        type: i.type,
                        state: i.state
                };
                CommonFS.cacheRealToVirtual[i.path] = i.name;
            });
            manager._isWidgetPathFound = true;
        }
    }

    FileSystemManager.prototype.resolve = function() {
        DLog.fslogd('Entered: FileSystemManager.resolve()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        var args = AV.validateMethod(arguments, [
            {
                name : 'location',
                type : AV.Types.STRING
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
                name : 'mode',
                type : AV.Types.ENUM,
                values : Mode.getModes(),
                optional : true,
                nullable : true
            }
        ]);

        if (!args.has.mode) {
            args.mode = 'rw';
        }

        _initializeCache(this);

        if (args.location[0] === '/'){
            setTimeout(function() {
                C.callIfPossible(args.onerror,
                        C.getNotFound('Global path without \'file://\' prefix is not valid.'));
            }, 0);
            return;
        }
        var _realPath = CommonFS.toRealPath(args.location);
        var _isLocationAllowed = CommonFS.isLocationAllowed(_realPath);

        if (args.mode != 'r' && !_isLocationAllowed) {
            setTimeout(function() {
                C.callIfPossible(args.onerror,
                        C.getInvalidValues('Provided arguments are not valid.'));
            }, 0);
            return;
        }

        fsp_fs.stat(_realPath, function(aErr, aStatObj) {
            var _result;
            if (aErr) {
                C.callIfPossible(args.onerror, CommonFS.translateNodeErrToWebAPIException(aErr,
                        'Problem during resolve function', true));
            } else {
                var _path = (args.location.indexOf('file://') === 0) ?
                        _toVirtualPath(args.location) : args.location;
                if (_path[_path.length-1] == '/') {
                    _path = _path.substr(0, _path.length-1);
                }
                _result = CommonFS.getFileInfo(_path, aStatObj, false, args.mode);
                if (_result.readOnly && args.mode != 'r') {
                    C.throwIOError();
                } else {
                    args.onsuccess(new File(_result));
                }
            }
        });
    };

    function Storage(data) {
        Object.defineProperties(
            this, {
            label: {value: data.name, writable: false, enumerable: true},
            type: {value: data.type, writable: false, enumerable: true},
            state: {value: data.state, writable: false, enumerable: true}
        });
    }

    FileSystemManager.prototype.getStorage = function() {
        DLog.fslogd('Entered: FileSystemManager.getStorage()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_READ);

        var args = AV.validateMethod(arguments, [
            {
                name : 'label',
                type : AV.Types.STRING
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
            }
        ]);

        _initializeCache(this);

        var cachedObj = CommonFS.cacheVirtualToReal[args.label];
        if (undefined === cachedObj) {
            setTimeout(function() {
                C.callIfPossible(args.onerror,
                        new WebAPIException(WebAPIException.NOT_FOUND_ERR,
                                'Storage not found.'));
            }, 0);
        } else {
            setTimeout(function(){
                args.onsuccess(new Storage({
                    name: args.label,
                    type: cachedObj.type ? cachedObj.type : _internalStr,
                    state: cachedObj.state ? cachedObj.state : _mountedStr
                }));
            }, 0);
        }
    };

    FileSystemManager.prototype.listStorages = function() {
        DLog.fslogd('Entered: FileSystemManager.listStorages()');

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
            }
        ]);

        _initializeCache(this);

        var _storages = [];

        for (var _storage in CommonFS.cacheVirtualToReal) {
            var storageObj = CommonFS.cacheVirtualToReal[_storage];
            _storages.push(new Storage({
                name: _storage,
                type: storageObj.type ? storageObj.type : _internalStr,
                state: storageObj.state ? storageObj.state : _mountedStr
            }));
        }

        setTimeout(function() {args.onsuccess(_storages);}, 0);
    };

    var _storageListeners = {};
    var _nextId = 0;

    function _StorageStateChangeListener(event) {
        console.log("Javascript listener triggered");
        var e = JSON.parse(event);
        var s = new Storage(e);
        for (var watchId in _storageListeners) {
            if (_storageListeners.hasOwnProperty(watchId)) {
                _storageListeners[watchId](s);
            }
        }
    }

    FileSystemManager.prototype.addStorageStateChangeListener = function() {
        DLog.fslogd('Entered: FileSystemManager.addStorageStateChangeListener()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_WRITE);

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
            }
        ]);

        var register = false;
        if (T.isEmptyObject(_storageListeners)) {
            register = true;
        }

        var result={};
        var _id = ++_nextId;
        _storageListeners[_id] = args.onsuccess;

        setTimeout(function(){
            if (register) {
                native.addListener('StorageStateChangeListener',
                        _StorageStateChangeListener);
                result = _callSync('FileSystemManager_registerStorageStateChangeListener', {});

                if (C.isFailure(result)) {
                    C.callIfPossible(args.onerror, C.getErrorObject(result));
                }
            } else {
                _callSync('FileSystemManager_triggerStorageStateChangeListeners', {});
            }
        }, 0);
        return Converter.toLong(_id);
    };

    FileSystemManager.prototype.removeStorageStateChangeListener = function() {
        DLog.fslogd('Entered: FileSystemManager.removeStorageStateChangeListener()');

        CommonFS.checkPrivilege(CommonFS.PRIVILEGE_FILESYSTEM_WRITE);

        var args = AV.validateMethod(arguments, [
            {
                name : 'watchId',
                type : AV.Types.LONG
            }
        ]);

        var id = args.watchId;

        if (T.isNullOrUndefined(_storageListeners[id])) {
            C.throwNotFound('Watch ID not found.');
        }

        delete _storageListeners[id];

        if (T.isEmptyObject(_storageListeners)) {
            // FIXME: implement C++ side
            _callSync('FileSystemManager_unregisterStorageStateChangeListener', {});
            native.removeListener('StorageStateChangeListener',
                                  _StorageStateChangeListener);
        }
    };

    module.exports = FileSystemManager;
})();
