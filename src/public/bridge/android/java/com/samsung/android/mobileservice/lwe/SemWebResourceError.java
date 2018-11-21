/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */
package com.samsung.android.mobileservice.lwe;


/**
 * This class encapsulates information about errors occured during loading of Web resources.
 * See SemWebViewClient.onReceivedError(WebView, WebResourceRequest, WebResourceError).
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
