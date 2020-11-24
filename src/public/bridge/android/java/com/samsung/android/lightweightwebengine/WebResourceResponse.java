package com.samsung.android.lightweightwebengine;

import java.io.IOException;
import java.io.InputStream;
import java.util.Map;

public class WebResourceResponse {
    public WebResourceResponse(String mimeType, String encoding, InputStream data) {
    }

    public WebResourceResponse(String mimeType, String encoding, int statusCode, String reasonPhrase,
            Map<String, String> responseHeaders, InputStream data) {
    }

    public InputStream getData() {
        return null;
    }

    public String getEncoding() {
        return new String();
    }

    public String getMimeType() {
        return new String();
    }

    public String getReasonPhrase() {
        return new String();
    }

    Map<String, String> getResponseHeaders() {
        return null;
    }

    int getStatusCode() {
        return 0;
    }

    void setData(InputStream data) {
    }

    void setEncoding(String encoding) {
    }

    void setMimeType(String mimeType) {
    }

    void setResponseHeaders(Map<String, String> headers) {
    }

    void setStatusCodeAndReasonPhrase(int statusCode, String reasonPhrase) {
    }
}
