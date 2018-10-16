package com.samsung.android.mobileservice.lwe;

import android.net.Uri;

/**
 * This interface is used to implement a WebResourceRequest for a SemWebViewClient
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
