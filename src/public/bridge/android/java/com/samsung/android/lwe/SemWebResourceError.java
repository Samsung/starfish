/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

package com.samsung.android.lwe;


/**
 * This class encapsulates information about errors occured during loading of Web resources.
 * See SemWebViewClient.onReceivedError(WebView, WebResourceRequest, WebResourceError).
 *
 * @deprecated This class was deprecated in API level 29. This class will be removed in a future Android release, and will not be supported anymore. Do not use this class.
 */
public class SemWebResourceError {

    private int mErrorCode = 0;
    private CharSequence mErrorDescription = "";

    /**
     * Creates a SemWebResourceError object.
     *
     * @hide Internal use only
     * @param code ErrorCode
     * @param description Description of the error message
     * @since Lightweight Web Engine 1.0
     */
    public SemWebResourceError(int code, CharSequence description) {
        mErrorCode = code;
        mErrorDescription = description;
    }

    /**
     * Gets the string describing the error. Descriptions are localized, and thus can be used for
     * communicating the problem to the user.
     *
     * @return The description of the error
     * @since Lightweight Web Engine 1.0
     */
    public CharSequence getDescription() {
        return mErrorDescription;
    }

    /**
     * Gets the error code of the error. The code corresponds to one of the ERROR_* constants in
     * SemWebViewClient.
     *
     * @return The error code of the error
     * @since Lightweight Web Engine 1.0
     */
    public int getErrorCode() {
        return mErrorCode;
    }
}
