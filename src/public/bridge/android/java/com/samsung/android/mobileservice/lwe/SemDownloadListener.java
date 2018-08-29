package com.samsung.android.mobileservice.lwe;

/**
 * This interface is used to implement a download listener for a SemWebView
 */
public interface SemDownloadListener {
    /**
     * Notify the host application that a file should be downloaded.
     *
     * @param url The full url to the content that should be downloaded
     * @param userAgent The user agent to be used for the download
     * @param contentDisposition Content-disposition http header, if present
     * @param mimetype The mimetype of the content reported by the server
     * @param contentLength The file size reported by the server
     * @since Lightweight Web Engine 1.0
     */
    void onDownloadStart(String url, String userAgent, String contentDisposition, String mimetype,
                         long contentLength);
}
