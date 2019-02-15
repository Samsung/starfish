/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

package com.samsung.android.lwe.shell;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.inputmethod.EditorInfo;
import android.widget.TextView;
import com.samsung.android.lwe.SemWebView;
import android.support.design.widget.TextInputEditText;
import android.support.design.widget.TextInputLayout;

import java.lang.ref.WeakReference;

public class MainActivity extends AppCompatActivity {
    String defaultURL = "http://www.apache.org/licenses/";

    private TextInputLayout mLayout;
    private TextInputEditText mEditText;
    private SemWebView mWebView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mLayout = findViewById(R.id.inputLayout);
        mEditText = findViewById(R.id.editText);
        mWebView = findViewById(R.id.webView);

        mEditText.setOnEditorActionListener(ActionListener.newInstance(this));

        mWebView.loadUrl(defaultURL);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.i("shell", "keyCode : " + keyCode);
        if ((keyCode == KeyEvent.KEYCODE_BACK) && mWebView.canGoBack()) {
            mWebView.goBack();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    private static final class ActionListener implements TextView.OnEditorActionListener {
        private final WeakReference<MainActivity> mShellActivityWeakReference;

        public static ActionListener newInstance(MainActivity activity) {
            WeakReference<MainActivity> shellActivityWeakReference = new WeakReference<>(activity);
            return new ActionListener(shellActivityWeakReference);
        }

        private ActionListener(WeakReference<MainActivity> mainActivityWeakReference) {
            this.mShellActivityWeakReference = mainActivityWeakReference;
        }

        @Override
        public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
            MainActivity shell = mShellActivityWeakReference.get();

            if (shell != null) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    if (v.getText() != null) {
                        Log.i("shell", "loadurl : " + v.getText().toString());
                        shell.mWebView.loadUrl(v.getText().toString());
                    }

                }
            }
            return true;
        }
    }
}
