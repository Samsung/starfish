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
 * This class is a Web LWE client for a SemWebView.
 *
 * @deprecated This class was deprecated in API level 29. This class will be removed in a future Android release, and will not be supported anymore. Do not use this class.
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
