package com.samsung.lwe.shell;

import android.graphics.Bitmap;
import android.support.design.widget.TextInputEditText;
import android.support.design.widget.TextInputLayout;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.widget.TextView;

import com.samsung.android.lwe.SemWebSettings;
import com.samsung.android.lwe.SemWebView;
import com.samsung.android.lwe.SemWebViewClient;

import java.lang.ref.WeakReference;

public class ShellActivity extends AppCompatActivity {
    private static final String mTag = "Shell";

    private static final class ActionListener implements TextView.OnEditorActionListener {
        private final WeakReference<ShellActivity> mShellActivityWeakReference;

        public class MyWebViewClient extends SemWebViewClient {
            public void onPageStarted(SemWebView view, String url, Bitmap favicon) {
                Log.d(mTag, "SemWebViewClient::OnPageStarted: " + url);
            }

            public void onPageFinished(SemWebView view, String url) {
                Log.d(mTag, "SemWebViewClient:OnPageFinished: " + url);
            }
        }

        public static ActionListener newInstance(ShellActivity activity) {
            WeakReference<ShellActivity> shellActivityWeakReference = new WeakReference<>(activity);
            return new ActionListener(shellActivityWeakReference);
        }

        private ActionListener(WeakReference<ShellActivity> mainActivityWeakReference) {
            this.mShellActivityWeakReference = mainActivityWeakReference;
        }
        @Override
        public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
            ShellActivity shell = mShellActivityWeakReference.get();

            if (shell != null) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    if (v.getText() != null) {
                        String url = v.getText().toString();
                        Log.d(mTag, "loadUrl : " + url);
                        shell.mWebView.setWebViewClient(new MyWebViewClient());
                        shell.mWebView.loadUrl(url);
                    }
                }
            }
            return true;
        }
    }

    private TextInputLayout mLayout;
    private TextInputEditText mEditText;
    private SemWebView mWebView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_shell);

        mLayout = findViewById(R.id.inputLayout);
        mEditText = findViewById(R.id.editText);
        mWebView = findViewById(R.id.webView);

        mEditText.setOnEditorActionListener(ActionListener.newInstance(this));
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(mTag, "keyCode : " + keyCode);
        if ((keyCode == KeyEvent.KEYCODE_BACK) && mWebView.canGoBack()) {
            mWebView.goBack();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
}
