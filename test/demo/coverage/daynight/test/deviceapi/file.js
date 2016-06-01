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
 
function File(data) {
  function fileSizeGetter() {
    var _realPath = commonFS_.toRealPath(this.fullPath);
    var _result = native_.callSync('File_statSync', {location: _realPath});
    var _aStatObj = native_.getResultObject(_result);
    return _aStatObj.isFile ? _aStatObj.size : undefined;
  }

  Object.defineProperties(this, {
    parent: {
      value: (function(data) {
        try {
          if (data.parent) { // prevent recursive - only one parent
            var _parentPath = data.path.substr(0, data.path.length - 1);
            var _location = {location: commonFS_.toRealPath(_parentPath)};
            var _result = native_.callSync('File_statSync', _location);
            var _statObj = native_.getResultObject(_result);
            var _info = commonFS_.getFileInfo(_statObj, true);
            return new File(_info);
          } else {
            return null;
          }
        } catch (err) {
          console.log(err.name, err.message);
          return null;
        }
      }(data)),
      writable: false,
      enumerable: true
    },
    readOnly: {value: data.readOnly, writable: false, enumerable: true},
    isFile: {value: data.isFile, writable: false, enumerable: true},
    isDirectory: {value: data.isDirectory, writable: false, enumerable: true},
    created: {value: data.created, writable: false, enumerable: true},
    modified: {value: data.modified, writable: false, enumerable: true},
    path: {value: data.path, writable: false, enumerable: true},
    name: {value: data.name, writable: false, enumerable: true},
    fullPath: {value: data.fullPath, writable: false, enumerable: true},
    fileSize: {enumerable: true, set: function() {
    }, get: fileSizeGetter},
    length: {value: data.length, writable: false, enumerable: true},
    mode: {value: data.mode, writable: false}
  });
}

File.prototype.toURI = function() {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);
  return 'file://' + commonFS_.toRealPath(this.fullPath);
};

function stringToRegex(str) {
  var _regString = '^';
  if (str === '') {
    return new RegExp(_regString, 'i');
  }

  str = str.replace(/[\-\[\]\/\{\}\(\)\*\+\?\.\\\^\$\|]/g, '\\$&');

  var _percentTokens = str.split('%');
  var i;
  for (i = 0; i < _percentTokens.length - 1; ++i) {
    _regString = _regString + _percentTokens[i];
    if (_regString[_regString.length - 1] === '\\') {
      _regString = _regString.split('');
      _regString.pop();
      _regString = _regString.join('') + '%';
    }
    else if (_regString.lastIndexOf('\*') !== _regString.length - 2) {
      _regString = _regString + '.*';
    }
  }
  return new RegExp(_regString + _percentTokens[i] + '$', 'i');
}


function createFilter(fileFilter) {
  if (type_.isNull(fileFilter)) {
    return null;
  }

  var FileFilter = {
    name: 'name',
    startModified: 'startModified',
    endModified: 'endModified',
    startCreated: 'startCreated',
    endCreated: 'endCreated'
  };

  var _fileFilter = {}, i;
  for (i in fileFilter) {
    if (!type_.isNullOrUndefined(fileFilter[i])) {
      if (Object.keys(FileFilter).indexOf(i) >= 0) {
        if (FileFilter.name === i) {
          _fileFilter[i] = stringToRegex(fileFilter[i]);
        } else {
          if (!(fileFilter[i] instanceof Date)) {
            throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
                'Invalid date');
          }
          _fileFilter[i] = fileFilter[i];
        }
      }
    }
  }

  return !type_.isEmptyObject(_fileFilter) ? _fileFilter : null;
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

File.prototype.listFiles = function(onsuccess, onerror, filter) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);

  var args = validator_.validateArgs(arguments, [
    {name: 'onsuccess', type: types_.FUNCTION},
    {name: 'onerror', type: types_.FUNCTION, optional: true, nullable: true},
    {name: 'filter', type: types_.DICTIONARY, optional: true, nullable: true}
  ]);

  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Arguments missing');
  }
  if (!this.isDirectory) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.IO_ERR,
          'File object which call this method is not directory'));
    }, 0);
    return;
  }

  var _fileFilter = null;

  if (args.has.filter) {
    _fileFilter = createFilter(args.filter);
  }

  var _myPath = this.fullPath;
  var _realMyPath = commonFS_.toRealPath(_myPath);

  var data = {
    pathToDir: _realMyPath
  };

  var callback = function(result) {
    if (native_.isFailure(result)) {
      native_.callIfPossible(args.onerror, native_.getErrorObject(result));
      return;
    }
    var aFiles = native_.getResultObject(result);
    var _result = [],
        i,
        _statObj,
        _fileInfo;
    for (i = 0; i < aFiles.length; ++i) {
      _statObj = aFiles[i];
      _fileInfo = commonFS_.getFileInfo(_statObj);

      if (_fileFilter === null) {
        _result.push(new File(_fileInfo));
      } else if (checkFile(_fileInfo, _fileFilter)) {
        _result.push(new File(_fileInfo));
      }
    }
    native_.callIfPossible(args.onsuccess, _result);
  };

  native_.call('File_readDir', data, callback);
};

var Encoding = {
  'utf-8': 'utf-8',
  'iso-8859-1': 'iso-8859-1',
  'sjis': 'sjis',  // backward compatibility
  'null': 'utf-8',
  'undefined': 'utf-8'
};

function _checkEncoding(encoding) {
  encoding = String(encoding).toLowerCase();
  if (undefined === Encoding[encoding]) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Argument "encoding" has invalid value: ' + encoding);
  } else {
    return Encoding[encoding];
  }
}

File.prototype.openStream = function(mode, onsuccess, onerror, encoding) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);

  var args = validator_.validateArgs(arguments, [
    {name: 'mode', type: types_.ENUM, values: ['r', 'rw', 'w', 'a']},
    {name: 'onsuccess', type: types_.FUNCTION},
    {name: 'onerror', type: types_.FUNCTION, optional: true, nullable: true},
    {name: 'encoding', type: types_.STRING, optional: true, nullable: true}
  ]);

  if (arguments.length < 2) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Arguments missing');
  }
  if (this.mode === 'r' && args.mode !== 'r') {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR, 'Read only mode'));
    }, 0);
    return;
  }

  if (this.isDirectory) {
    var directoryMessage = 'This method should be called on file, not directory';
    setTimeout(function() {
      native_.callIfPossible(args.onerror, new WebAPIException(WebAPIException.IO_ERR,
          directoryMessage));
    }, 0);
    return;
  }

  args.encoding = _checkEncoding(args.encoding);

  var _realPath = commonFS_.toRealPath(this.fullPath);
  var _result = native_.callSync('File_statSync', {location: _realPath});
  if (native_.isFailure(_result)) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.IO_ERR, 'File does not exist'));
    }, 0);
    return;
  }

  var fileStream = new FileStream(this, args.mode, args.encoding);
  setTimeout(function() {
    native_.callIfPossible(args.onsuccess, fileStream);
  }, 0);
};

File.prototype.readAsText = function(onsuccess, onerror, encoding) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);

  var args = validator_.validateArgs(arguments, [
    {name: 'onsuccess', type: types_.FUNCTION},
    {name: 'onerror', type: types_.FUNCTION, optional: true, nullable: true},
    {name: 'encoding', type: types_.STRING, optional: true, nullable: true}
  ]);

  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Arguments missing');
  }
  if (this.isDirectory) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.IO_ERR,
              'File object which call this method is directory'));
    }, 0);
    return;
  }

  args.encoding = _checkEncoding(args.encoding);

  var data = {
    location: commonFS_.toRealPath(this.fullPath),
    offset: 0,
    length: 1024,
    encoding: args.encoding,
    is_base64: false
  };

  function readFile() {
    var result, encoded, str = '';

    do {
      result = native_.callSyncData('File_readSync', data);

      if (native_.isFailure(result)) {
        setTimeout(function() {
          native_.callIfPossible(args.onerror, native_.getErrorObject(result));
        }, 0);
        return;
      }
      str += result.output;
      data.offset += result.reply.data_size;
    } while (result.reply.data_size);

    setTimeout(function() {
      native_.callIfPossible(args.onsuccess, str);
    }, 0);
  }

  setTimeout(readFile, 0);
};

File.prototype.copyTo = function(originFilePath, destinationFilePath, overwrite, onsuccess, onerror) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_WRITE);

  var args = validator_.validateArgs(arguments, [
    {name: 'originFilePath', type: types_.STRING},
    {name: 'destinationFilePath', type: types_.STRING},
    {name: 'overwrite', type: types_.BOOLEAN},
    {name: 'onsuccess', type: types_.FUNCTION, optional: true, nullable: true},
    {name: 'onerror', type: types_.FUNCTION, optional: true, nullable: true}
  ]);

  if (arguments.length < 3) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Invalid arguments given');
  }

  if (this.isFile) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.IO_ERR,
          'File object which call this method is not directory'));
    }, 0);
    return;
  }

  var lastChar;
  var addFilenameToPath = false;
  if (args.destinationFilePath.length) {
    lastChar = args.destinationFilePath.substr(args.destinationFilePath.length - 1);
    if (lastChar === '/') {
      addFilenameToPath = true;
    }
  }

  var _realOriginalPath = commonFS_.toRealPath(args.originFilePath);
  var _realDestinationPath = commonFS_.toRealPath(args.destinationFilePath);

  if (!_realOriginalPath) {
      setTimeout(function() {
        native_.callIfPossible(args.onerror,
            new WebAPIException(WebAPIException.NOT_FOUND_ERR,
            'Source path is not valid'));
      }, 0);
      return;
  }

  if (!_realDestinationPath) {
      setTimeout(function() {
        native_.callIfPossible(args.onerror,
            new WebAPIException(WebAPIException.NOT_FOUND_ERR,
            'Destination path is not valid'));
      }, 0);
      return;
  }

  var resultOldPath = native_.callSync('File_statSync', {location: _realOriginalPath});
  if (native_.isFailure(resultOldPath)) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror, native_.getErrorObject(resultOldPath));
    }, 0);
    return;
  }
  var _oldNode = native_.getResultObject(resultOldPath);

  if (_oldNode.isFile && addFilenameToPath) {
    _realDestinationPath = _realDestinationPath + _realOriginalPath.split('/').pop();
  }

  if (!args.overwrite) {
    var resultNewPath = native_.callSync('File_statSync', {location: _realDestinationPath});
    if (native_.isSuccess(resultNewPath)) {
      setTimeout(function() {
        native_.callIfPossible(args.onerror,
            new WebAPIException(WebAPIException.IO_ERR, 'Overwrite is not allowed'));
      }, 0);
      return;
    }
  }

  if (!commonFS_.f_isSubDir(_realOriginalPath, this.fullPath)) {
    var m1 = 'Source file should be subdirectory of: ' + this.fullPath;
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR, m1));
    }, 0);
    return;
  }

  if (!commonFS_.isLocationAllowed(_realDestinationPath)) {
    var m2 = 'Destination is read only folder: ' + this.fullPath;
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR, m2));
    }, 0);
    return;
  }

  var data = {
    originFilePath: _realOriginalPath,
    destinationFilePath: _realDestinationPath,
    overwrite: args.overwrite
  };

  var callback = function(result) {
    if (native_.isFailure(result)) {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.IO_ERR, result));
      return;
    }
    native_.callIfPossible(args.onsuccess);
  };

  native_.call('File_copyTo', data, callback);
};

File.prototype.moveTo = function(originFilePath, destinationFilePath, overwrite, onsuccess, onerror) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_WRITE);

  var args = validator_.validateArgs(arguments, [
    {name: 'originFilePath', type: types_.STRING},
    {name: 'destinationFilePath', type: types_.STRING},
    {name: 'overwrite', type: types_.BOOLEAN},
    {name: 'onsuccess', type: types_.FUNCTION, optional: true, nullable: true},
    {name: 'onerror', type: types_.FUNCTION, optional: true, nullable: true}
  ]);

  if (arguments.length < 3) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Arguments missing');
  }
  if (this.isFile) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.IO_ERR,
          'File object which call this method is not directory'));
    }, 0);
    return;
  }

  var _realOriginalPath = commonFS_.toRealPath(args.originFilePath);
  var _realDestinationPath = commonFS_.toRealPath(args.destinationFilePath);

  if (!_realOriginalPath) {
      setTimeout(function() {
        native_.callIfPossible(args.onerror,
            new WebAPIException(WebAPIException.NOT_FOUND_ERR,
            'Source path is not valid'));
      }, 0);
      return;
  }

  if (!_realDestinationPath) {
      setTimeout(function() {
        native_.callIfPossible(args.onerror,
            new WebAPIException(WebAPIException.NOT_FOUND_ERR,
            'Destination path is not valid'));
      }, 0);
      return;
  }

  var resultOldPath = native_.callSync('File_statSync', {location: _realOriginalPath});
  if (native_.isFailure(resultOldPath)) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.NOT_FOUND_ERR,
          'Source file is not avalaible'));
    }, 0);
    return;
  }

  if (!args.overwrite) {
    var resultNewPath = native_.callSync('File_statSync', {location: _realDestinationPath});
    if (native_.isSuccess(resultNewPath)) {
      setTimeout(function() {
        native_.callIfPossible(args.onerror,
            new WebAPIException(WebAPIException.IO_ERR, 'Overwrite is not allowed'));
      }, 0);
      return;
    }
  }

  if (!commonFS_.f_isSubDir(_realOriginalPath, this.fullPath)) {
    var m1 = 'Source file should be subdirectory of: ' + this.fullPath;
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR, m1));
    }, 0);
    return;
  }

  if (this.mode === 'r' || !commonFS_.isLocationAllowed(_realDestinationPath)) {
    var m2 = 'Source/Destination is read only folder: ' + this.fullPath;
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR, m2));
    }, 0);
    return;
  }

  var data = {
    oldPath: _realOriginalPath,
    newPath: _realDestinationPath
  };

  var callback = function(result) {
    if (native_.isFailure(result)) {
      native_.callIfPossible(args.onerror, new WebAPIException(WebAPIException.IO_ERR,
          result));
      return;
    }
    native_.callIfPossible(args.onsuccess);
  };

  native_.call('File_rename', data, callback);
};

File.prototype.createDirectory = function(dirPath) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_WRITE);

  var args = validator_.validateArgs(arguments, [
    {name: 'dirPath', type: types_.STRING}
  ]);

  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
        'Invalid path');
  }
  if (!args.dirPath.length) {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
        'Invalid path');
  }

  var _newPath = this.fullPath + '/' + args.dirPath,
          _statObj,
          _fileInfo,
          _realNewPath = commonFS_.toRealPath(_newPath);

  if (!_realNewPath) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
          'Path is not valid'));
    }, 0);
    return;
  }

  if (this.isDirectory) {
    if (this.mode === 'r') {
      throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
          'Invalid path or readonly access');
    }

    var _resultExist = native_.callSync('File_statSync', {location: _realNewPath});
    if (native_.isSuccess(_resultExist)) {
      throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
          'Directory already exist');
    }

    var result = native_.callSync('FileSystemManager_mkdirSync', {location: _realNewPath});
    if (native_.isFailure(result)) {
      throw new WebAPIException(WebAPIException.IO_ERR, native_.getErrorObject(result));
    }

    var _result = native_.callSync('File_statSync', {location: _realNewPath});
    _statObj = native_.getResultObject(_result);

    _fileInfo = commonFS_.getFileInfo(_statObj, false, this.mode);
    return new File(_fileInfo);
  } else {
    throw new WebAPIException(WebAPIException.IO_ERR,
        'File object which call this method is not directory');
  }
};

File.prototype.createFile = function(relativeFilePath) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_WRITE);

  var args = validator_.validateArgs(arguments, [
    {name: 'relativeFilePath', type: types_.STRING}
  ]);

  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
        'Argument "relativeFilePath" missing');
  }
  if (this.isFile) {
    throw new WebAPIException(WebAPIException.IO_ERR,
        'File object which call this method is not directory');
  }

  if (!commonFS_.f_isCorrectRelativePath(args.relativeFilePath) || this.mode === 'r') {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
        'Invalid path or readonly acces');
  }

  var _outputPath = this.fullPath + '/' + args.relativeFilePath;
  var _outputRealPath = commonFS_.toRealPath(_outputPath);
  if (!_outputRealPath) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
          'Path is not valid'));
    }, 0);
    return;
  }
  var _resultExist = native_.callSync('File_statSync', {location: _outputRealPath});

  if (native_.isSuccess(_resultExist)) {
    throw new WebAPIException(WebAPIException.IO_ERR, 'Overwrite is not allowed');
  }

  var result = native_.callSync('File_createSync', {location: _outputRealPath});
  if (native_.isFailure(result)) {
    throw native_.getErrorObject(result);
  }

  var _result = native_.callSync('File_statSync', {location: _outputRealPath});
  var _statObj = native_.getResultObject(_result);
  var _fileInfo = commonFS_.getFileInfo(_statObj, false, this.mode);

  return new File(_fileInfo);

};

File.prototype.resolve = function(filePath) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);

  var args = validator_.validateArgs(arguments, [
    {name: 'filePath', type: types_.STRING}
  ]);

  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Argument "filePath" missing');
  }
  if (this.isFile) {
    throw new WebAPIException(WebAPIException.IO_ERR,
        'File object which call this method is not directory');
  }

  if (!commonFS_.f_isCorrectRelativePath(args.filePath)) {
    throw new WebAPIException(WebAPIException.INVALID_VALUES_ERR, 'Invalid path');
  }

  var _newPath = this.fullPath + '/' + args.filePath;
  var _realPath = commonFS_.toRealPath(_newPath);
  if (!_realPath) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
          'Path is not valid'));
    }, 0);
    return;
  }
  var _result = native_.callSync('File_statSync', {location: _realPath});
  if (native_.isFailure(_result)) {
    throw new WebAPIException(WebAPIException.NOT_FOUND_ERR, native_.getErrorObject(_result));
  }
  var _statObj = native_.getResultObject(_result);
  var _fileInfo = commonFS_.getFileInfo(_statObj, false, this.mode);
  return new File(_fileInfo);

};

File.prototype.deleteDirectory = function(directoryPath, recursive, onsuccess, onerror) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_WRITE);

  var args = validator_.validateArgs(arguments, [
    {name: 'directoryPath', type: types_.STRING},
    {name: 'recursive', type: types_.BOOLEAN},
    {name: 'onsuccess', type: types_.FUNCTION, optional: true, nullable: true},
    {name: 'onerror', type: types_.FUNCTION, optional: true, nullable: true}
  ]);

  if (arguments.length < 2) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Arguments missing');
  }
  if (this.mode === 'r') {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
              'Invalid path or readonly access'));
    }, 0);
    return;
  }

  var _myPath = commonFS_.toRealPath(args.directoryPath);

  if (_myPath !== undefined && !commonFS_.f_isSubDir(_myPath, this.fullPath)) {
    var m1 = 'Deleted directory should be under the current directory: ' + this.fullPath;
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR, m1));
    }, 0);
    return;
  }

  var _result = native_.callSync('File_statSync', {location: _myPath});
  if (native_.isFailure(_result)) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.NOT_FOUND_ERR, 'Directory not found'));
    }, 0);
    return;
  }
  var _statObj = native_.getResultObject(_result);
  var _info = commonFS_.getFileInfo(_statObj);
  var _node = new File(_info);

  if (!_node.isDirectory) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
          'It is file not directory'));
    }, 0);
    return;
  } else {
    _node.listFiles(
        function(files) {
          if (files.length > 0) {
            if (!args.recursive) {
              native_.callIfPossible(args.onerror,
                  new WebAPIException(WebAPIException.IO_ERR,
                  'Non empty folder ' + _myPath + ' passed for non recursive delete'));
              return;
            }
          }
          var data = {
            pathToDelete: _myPath
          };

          var callback = function(result) {
            if (native_.isFailure(result)) {
              native_.callIfPossible(args.onerror, native_.getErrorObject(result));
            }
            native_.callIfPossible(args.onsuccess);
          };

          native_.call('File_removeDirectory', data, callback);
        },
        function() {
          native_.callIfPossible(args.onerror,
              new WebAPIException(WebAPIException.IO_ERR,
              'List files failed for ' + _myPath));
        }
    );
  }
};

File.prototype.deleteFile = function(filePath, onsuccess, onerror) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_WRITE);

  var args = validator_.validateArgs(arguments, [
    {name: 'filePath', type: types_.STRING},
    {name: 'onsuccess', type: types_.FUNCTION, optional: true, nullable: true},
    {name: 'onerror', type: types_.FUNCTION, optional: true, nullable: true}
  ]);

  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Argument "filePath" missing');
  }
  if (this.isFile) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.IO_ERR,
          'File object which call this method is not directory'));
    }, 0);
    return;
  }

  var _fileRealPath = commonFS_.toRealPath(args.filePath);

  var _result = native_.callSync('File_statSync', {location: _fileRealPath});
  if (native_.isFailure(_result)) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.NOT_FOUND_ERR, 'File is not avalaible'));
    }, 0);
    return;
  }
  var _statObj = native_.getResultObject(_result);

  if (_statObj.isDirectory) {
    var message = 'Requested object is a directory.';
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR, message));
    }, 0);
    return;
  }

  if (!commonFS_.f_isSubDir(_fileRealPath, this.fullPath) || this.mode === 'r') {
    var _message = 'Deleted file [' + args.filePath + '] should have write access ' +
            'and should be subdirectory of: [' + this.fullPath + ']';
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR, _message));
    }, 0);
    return;
  }

  var data = {
    pathToFile: _fileRealPath
  };

  var callback = function(result) {
    if (native_.isFailure(result)) {
      native_.callIfPossible(args.onerror, native_.getErrorObject(result));
      return;
    }
    native_.callIfPossible(args.onsuccess);
  };

  native_.call('File_unlinkFile', data, callback);
};
