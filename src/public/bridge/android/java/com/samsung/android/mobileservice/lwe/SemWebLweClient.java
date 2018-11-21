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
 * This class is a Web LWE client for a SemWebView.
 */
public class SemWebLweClient {
    /**
     * Constructs a new SemWebLweClient
     *
     * @since Lightweight Web Engine 1.0
     */
    public SemWebLweClient() {
    }

    /**
     * Tell the host application the current progress of loading a page.
     *
     * @param view The WebView that initiated the callback.
     * @param newProgress Current page loading progress, represented by an integer
     *                    between 0 and 100
     * @since Lightweight Web Engine 1.0
     */
    public void onProgressChanged(SemWebView view, int newProgress) {
    }
}
