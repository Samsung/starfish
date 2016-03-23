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

    var DLog = require('./tizen.filesystem.DLog');
    var PlatformMode = require('./tizen.filesystem.PlatformMode');

    var _common = require('./tizen.Common');
    var C = _common.Common;
    var _callSync = C.getCallSync('filesystem');
    _common = undefined;

    function CommonFS() {
        Object.defineProperties(this, {
            PRIVILEGE_FILESYSTEM_READ : {
                value : 'http://tizen.org/privilege/filesystem.read',
                writable : false,
                enumerable : true
            },
            PRIVILEGE_FILESYSTEM_WRITE : {
                value : 'http://tizen.org/privilege/filesystem.write',
                writable : false,
                enumerable : true
            }
        });
    }

    CommonFS.prototype.checkPrivilege = function (privilege) {
        var args = { privilege : privilege };
        var result = _callSync('Filesystem_checkPrivilege', args);

        if (C.isFailure(result)) {
            throw C.getErrorObject(result);
        }
    };

    CommonFS.prototype.translateNodeErrToWebAPIException = function (err, message, expectedRet) {
        if (err instanceof WebAPIException){
            if (expectedRet){
                return err;
            } else {
                throw err;
            }
        }

        message = message || 'Unknown error';
        var exception;
        if (!err.code){
            exception = new WebAPIException('UnknownError', message);
            if (expectedRet){
                return exception;
            } else {
                throw exception;
            }
        }

        switch (err.code) {
        case 'EACCES':
        case 'EBADF':
            exception = new WebAPIException('IOError', message);
            break;
        case 'ENOENT':
            exception = new WebAPIException(WebAPIException.NOT_FOUND_ERR, message);
            break;
        default:
            exception = new WebAPIException('UnknownError', message);
        }

        if (expectedRet){
            return exception;
        } else {
            throw exception;
        }
    };

    CommonFS.prototype.getFileInfo = function (aPath, aStatObj, secondIter, aMode) {
        var _result = {},
            _pathTokens,
            _fileParentPath = '',
            i;
        DLog.fslogd('File chmod: ', aStatObj.mode.toString(8));

        _result.readOnly = true;
        if (process.getuid() == aStatObj.uid &&
           (aStatObj.mode & PlatformMode.PM_USER_WRITE) == PlatformMode.PM_USER_WRITE) {
            _result.readOnly = false;
        } else if (process.getgid() == aStatObj.gid &&
                  (aStatObj.mode & PlatformMode.PM_GROUP_WRITE) == PlatformMode.PM_GROUP_WRITE) {
            _result.readOnly = false;
        } else if (aStatObj.mode & PlatformMode.PM_OTHER_WRITE == PlatformMode.PM_OTHER_WRITE) {
            _result.readOnly = false;
        }

        if (aPath.indexOf('file://') === 0) {
            aPath = aPath.substr('file://'.length);
        }

        if (aPath[0] == '/') {
            aPath = aPath.substr(1);
            _fileParentPath = '/';
        }

        _result.isFile = aStatObj.isFile();
        _result.isDirectory = aStatObj.isDirectory();
        _result.created = aStatObj.ctime;
        _result.modified = aStatObj.mtime;
        _result.fullPath = _fileParentPath + aPath;
        _result.fileSize = _result.isFile ? aStatObj.size : undefined;
        _result.mode = aMode;
        if (_result.isDirectory) {
            try {
                _result.length = aStatObj.nlink;
            } catch (err) {
                DLog.fsloge('Read dir fails for ', aPath);
                _result.length = 0;
            }
        } else {
            _result.length = undefined;
        }

        _pathTokens = aPath.split('/');
        if (_pathTokens.length > 1) {
            for (i = 0; i < _pathTokens.length - 1; ++i) {
                _fileParentPath += _pathTokens[i] + '/';
            }
            _result.path = _fileParentPath;
            _result.name = _pathTokens[_pathTokens.length - 1];
            _result.parent = (secondIter) ? null : _fileParentPath;
        } else {
            _result.parent = null;
            _result.path = _fileParentPath == '/' ? _fileParentPath : aPath;
            _result.name = _fileParentPath == '/' ? aPath : '';
        }
        return _result;
    };

    CommonFS.prototype.cacheVirtualToReal = {
        'downloads' : { path: '/opt/usr/media/Downloads'},
        'documents' : { path: '/opt/usr/media/Documents'},
        'music'     : { path: '/opt/usr/media/Sounds'},
        'images'    : { path: '/opt/usr/media/Images'},
        'videos'    : { path: '/opt/usr/media/Videos'},
        'ringtones' : { path: '/opt/usr/share/settings/Ringtones'}
    };

    CommonFS.prototype.cacheRealToVirtual = {
        '/opt/usr/media/Downloads' : 'downloads',
        '/opt/usr/media/Documents' : 'documents',
        '/opt/usr/media/Sounds' : 'music',
        '/opt/usr/media/Images' : 'images',
        '/opt/usr/media/Videos' : 'videos',
        '/opt/usr/share/settings/Ringtones' :'ringtones'
    };

    CommonFS.prototype.isLocationAllowed = function (aPath) {
        if (aPath.indexOf(this.cacheVirtualToReal.ringtones.path) === 0) {
            return false;
        }
        if (aPath.indexOf(this.cacheVirtualToReal['wgt-package'].path) === 0) {
            return false;
        }

        return true;
    };

    CommonFS.prototype.toRealPath = function (aPath) {
        var _fileRealPath = '',
            _uriPrefix = 'file://',
            i;
        if (aPath.indexOf(_uriPrefix) === 0) {
            _fileRealPath = aPath.substr(_uriPrefix.length);
        } else if (aPath[0] != '/') {
            //virtual path
            var _pathTokens = aPath.split('/');
            if (this.cacheVirtualToReal[_pathTokens[0]] && (
                    this.cacheVirtualToReal[_pathTokens[0]].state === undefined ||
                    this.cacheVirtualToReal[_pathTokens[0]].state === 'MOUNTED')) {
                _fileRealPath = this.cacheVirtualToReal[_pathTokens[0]].path;
                for (i = 1; i < _pathTokens.length; ++i) {
                    _fileRealPath += '/' + _pathTokens[i];
                }
            } else {
                _fileRealPath = aPath;
            }
        } else {
            _fileRealPath = aPath;
        }

        return _fileRealPath;
    };

    module.exports = new CommonFS();
})();
