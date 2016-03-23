var _common = require('tizen.Common');
var xwalk = {
    utils: {
        type: _common.Type,
        converter: _common.Converter,
        validator: _common.ArgumentValidator,
        NativeManager : _common.Common,
        checkPrivilegeAccess : function () {
        },
        privilege : {
            FILESYSTEM_READ : 1,
            FILESYSTEM_WRITE : 2
        },
    }
};
