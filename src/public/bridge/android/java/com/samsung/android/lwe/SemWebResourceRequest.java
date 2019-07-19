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

import android.net.Uri;

/**
 * This interface is used to implement a WebResourceRequest for a SemWebViewClient
 *
 * @deprecated This interface was deprecated in API level 29. This interface will be removed in a future Android release, and will not be supported anymore. Do not use this interface.
 */
public interface SemWebResourceRequest {
    /**
     * Gets the URL for which the resource request was made.
     *
     * @return The URL for which the resource request was made.
     * @since Lightweight Web Engine 1.0
     */
    Uri getUrl();
}
