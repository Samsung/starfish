package com.samsung.android.mobileservice.lwe;

/**
 * A Web LWE client for a SemWebView
 */
public class SemWebLweClient {
    /**
     * Tell the host application the current progress of loading a page.
     *
     * @param view The WebView that initiated the callback.
     * @param newProgress Current page loading progress, represented by an integer
     *                    between 0 and 100.
     */
    public void onProgressChanged(SemWebView view, int newProgress) {
    }

    /*
    public boolean onShowFileChooser(SemWebView webView, ValueCallback<Uri[]> uris) {
        return false;
    }
    */
}
