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
 
function FileSystemStorage(data) {
  Object.defineProperties(this, {
    label: {value: data.label, writable: false, enumerable: true},
    type: {value: data.type, writable: false, enumerable: true},
    state: {value: data.state, writable: false, enumerable: true}
  });
}

var PATH_MAX = 4096;

function FileSystemManager() {
  Object.defineProperties(this, {
    maxPathLength: {value: PATH_MAX, writable: false, enumerable: true}
  });
}

FileSystemManager.prototype.resolve = function(location, onsuccess, onerror, mode) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);

  var args = validator_.validateArgs(arguments, [
    {name: 'location', type: types_.STRING},
    {name: 'onsuccess', type: types_.FUNCTION},
    {name: 'onerror', type: types_.FUNCTION, optional: true, nullable: true},
    {name: 'mode', type: types_.ENUM, values: Object.keys(FileMode), optional: true, nullable: true}
  ]);

  if (!args.has.mode) {
    args.mode = 'rw';
  }

  // resolving a path on unmounted storage should result in exception
  var storage = commonFS_.getStorage(args.location.split('/')[0]);
  if (storage && FileSystemStorageState.MOUNTED !== storage.state) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.NOT_FOUND_ERR,
          'Storage is not mounted.'));
    }, 0);
    return;
  }

  var _realPath = commonFS_.toRealPath(args.location);

  if (!_realPath) {
    // invalid real path means that virtual root does not exist
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.NOT_FOUND_ERR,
          'Specified virtual root does not exist.'));
    }, 0);
    return;
  }

  var _isLocationAllowed = commonFS_.isLocationAllowed(_realPath);

  if (args.mode !== 'r' && !_isLocationAllowed) {
    setTimeout(function() {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.INVALID_VALUES_ERR,
          'Provided arguments are not valid.'));
    }, 0);
    return;
  }

  var data = {
    location: _realPath
  };

  var callback = function(result) {
    if (native_.isFailure(result)) {
      native_.callIfPossible(args.onerror, native_.getErrorObject(result));
      return;
    }

    var aStatObj = native_.getResultObject(result);
    var _result = commonFS_.getFileInfo(aStatObj, false, args.mode);
    if (_result.readOnly && args.mode !== 'r') {
      native_.callIfPossible(args.onerror, new WebAPIException(WebAPIException.IO_ERR, 'File is read-only.'));
    } else {
      native_.callIfPossible(args.onsuccess, new File(_result));
    }
  };

  var ret = native_.call('File_stat', data, callback);
  if (native_.isFailure(ret)) {
      throw native_.getErrorObject(ret);
  }
};

FileSystemManager.prototype.getStorage = function(label, onsuccess, onerror) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);

  var args = validator_.validateArgs(arguments, [
    {name: 'label', type: types_.STRING},
    {name: 'onsuccess', type: types_.FUNCTION},
    {name: 'onerror', type: types_.FUNCTION, optional: true, nullable: true}
  ]);

  setTimeout(function() {
    var storage = commonFS_.getStorage(args.label);

    if (!storage) {
      native_.callIfPossible(args.onerror,
          new WebAPIException(WebAPIException.NOT_FOUND_ERR, 'Storage not found.'));
    } else {
      native_.callIfPossible(args.onsuccess, new FileSystemStorage(storage));
    }
  }, 0);
};

FileSystemManager.prototype.listStorages = function(onsuccess, onerror) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_READ);

  var args = validator_.validateArgs(arguments, [
    {name: 'onsuccess', type: types_.FUNCTION},
    {name: 'onerror', type: types_.FUNCTION, optional: true, nullable: true}
  ]);

  setTimeout(function() {
    var storages = [];
    var cache = commonFS_.getAllStorages();
    for (var i = 0; i < cache.length; ++i) {
      storages.push(new FileSystemStorage(cache[i]));
    }

    native_.callIfPossible(args.onsuccess, storages);
  }, 0);
};

var callbackId = 0;
var callbacks = {};

function nextCallbackId() {
  return ++callbackId;
}

function _StorageStateChangeListener(result) {
  commonFS_.clearCache();
  var storage = new FileSystemStorage(result);
  for (var id in callbacks) {
    native_.callIfPossible(callbacks[id], storage);
  }
}

FileSystemManager.prototype.addStorageStateChangeListener = function(onsuccess, onerror) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_WRITE);

  var args = validator_.validateArgs(arguments, [
    {name: 'onsuccess', type: types_.FUNCTION},
    {name: 'onerror', type: types_.FUNCTION, optional: true, nullable: true}
  ]);

  var register = false;
  if (type_.isEmptyObject(callbacks)) {
    register = true;
  }

  var id = nextCallbackId();
  callbacks[id] = args.onsuccess;

  if (register) {
    native_.addListener('StorageStateChangeListener', _StorageStateChangeListener);
    var result = native_.callSync('FileSystemManager_addStorageStateChangeListener', {});

    if (native_.isFailure(result)) {
      native_.callIfPossible(args.onerror, native_.getErrorObject(result));
      return;
    }
  }

  return id;
};

FileSystemManager.prototype.removeStorageStateChangeListener = function(watchId) {
  xwalk.utils.checkPrivilegeAccess(xwalk.utils.privilege.FILESYSTEM_WRITE);

  var args = validator_.validateArgs(arguments, [
    {name: 'watchId', type: types_.LONG}
  ]);

  if (!arguments.length) {
    throw new WebAPIException(WebAPIException.TYPE_MISMATCH_ERR,
        'Missing watchId');
  }
  var id = args.watchId;

  if (type_.isNullOrUndefined(callbacks[id])) {
    throw new WebAPIException(WebAPIException.NOT_FOUND_ERR, 'Watch ID not found.');
  }

  delete callbacks[id];

  if (type_.isEmptyObject(callbacks)) {
    native_.callSync('FileSystemManager_removeStorageStateChangeListener', {});
  }
};

exports = new FileSystemManager();
