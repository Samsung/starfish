package com.samsung.android.mobileservice.lwe;

import android.net.Uri;

public class WebResourceRequestImpl implements SemWebResourceRequest {
    String mUrl;

    WebResourceRequestImpl(String url) {
        mUrl = url;
    }

    public Uri getUrl() {
        return Uri.parse(mUrl);
    }
}
