package com.samsung.lwe.shell;

import android.support.design.widget.TextInputEditText;
import android.support.design.widget.TextInputLayout;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.widget.TextView;

import com.samsung.android.mobileservice.lwe.SemWebView;

import java.lang.ref.WeakReference;


public class ShellActivity extends AppCompatActivity {

    private static final class ActionListener implements TextView.OnEditorActionListener {
        private final WeakReference<ShellActivity> mShellActivityWeakReference;

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

            if (shell != null ) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    if (v.getText() != null) {
                        Log.i("Shell", "try loadurl : " + v.getText().toString());
                        shell.mWebView.loadUrl(v.getText().toString());
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
}
