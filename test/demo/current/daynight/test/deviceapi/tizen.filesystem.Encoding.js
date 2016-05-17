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

    var T = require('./tizen.Common').Type;

    var Encoding = (function () {
        var DEFAULT_ENCODING = 'utf-8';
        var ENCODINGS_WEBAPI_TO_NODE = {
            'utf-8': 'utf8',
            // FIXME add support for ISO-8859-1 and SJIS
            // consistent with wrt-plugin implementation
            // wrt-plugins implementation returns always string in 'utf-8'
            // if this behavior should be changed, the library should be added
            // node-iconv (for js support) or libiconv (for c/c++ support)
            'iso-8859-1': 'utf8',
            'sjis': 'utf8'
        };

        return {
            checkIsValid : function (encoding) {
                return !!this.getNodeValue(encoding);
            },
            getNodeValue : function (encoding) {
                return ENCODINGS_WEBAPI_TO_NODE[T.isNullOrUndefined(encoding) ? DEFAULT_ENCODING
                        : encoding.toLowerCase()];
            }
        };
    })();

    module.exports = Encoding;
})();
