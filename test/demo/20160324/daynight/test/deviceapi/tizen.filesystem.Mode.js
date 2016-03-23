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

    var Mode = (function () {
        // node flags description:
        // 'r' - Open file for reading. An exception occurs if the file does not exist
        // 'a' - Open file for appending. The file is created if it does not exist
        // 'w' - Open file for writing. The file is created (if it does not exist) or truncated
        //       (if it exists)
        // 'r+' - Open file for reading and writing. An exception occurs if the file does not exist
        var FILE_MODES_WEBAPI_TO_NODE = {
            'r': 'r',
            'a': 'a',
            'w': 'w',
            'rw': 'r+'
        };

        return {
            getModes : function () {
                return Object.keys(FILE_MODES_WEBAPI_TO_NODE);
            },
            checkIsValid : function (mode) {
                return !!this.getNodeValue(mode);
            },
            getNodeValue : function (mode){
                return FILE_MODES_WEBAPI_TO_NODE[String(mode).toLowerCase()];
            }
        };
    })();

    module.exports = Mode;
})();
