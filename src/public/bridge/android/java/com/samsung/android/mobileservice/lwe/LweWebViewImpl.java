/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

package com.samsung.android.mobileservice.lwe;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.SurfaceTexture;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.TextureView;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;

import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicInteger;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLSurface;

public class LweWebViewImpl implements LweWebView{
    static {
        try{
            System.loadLibrary("lightweightwebengine");
        }catch(final UnsatisfiedLinkError e){}
    }
    public enum ImeComposingStatus {
        NORMAL,
        COMPOSING_START,
        COMPOSING_END
    }

    static class TimerData {
        Runnable runnable;
        int fn;
        int data;
    }

    static HashMap<String, TimerData> sTimerMap = new HashMap<>();
    static HashMap<String, TimerData> sIdlerMap = new HashMap<>();
    static AtomicInteger sTimerUid = new AtomicInteger(0);

    private static String sTag = "LweWebViewImpl";
    private static float sDpr = 1;

    private static String sLocale = "ko-KR";
    private static String sTimezone = "Asia/Seoul";
    private static String sLocalStoragePath;
    private static String sCookiePath;
    private static String sCachePath;

    private static Handler sWebViewHandler;
    private static Integer sWebViewThreadLocker = new Integer(0);
    private static Looper sWebViewThreadLooper;
    private static Thread sWebViewThread;

    private long mWebViewInternalHandle;

    private int mWindowWidth;
    private int mWindowHeight;

    private boolean mCanGoBack = false;
    private boolean mCanGoForward = false;
    private String mCurrentURL = null;
    private SemWebViewClient mWebViewClient = null;
    private SemWebLweClient mWebLweClient = null;
    private SemDownloadListener mDownloadListener = null;

    private int mCacheMode = SemWebSettings.LOAD_DEFAULT;
    private int mDefaultFontSize;
    private String mDefaultUserAgent = null;
    private String mUserAgentString = null;
    private LweWebViewImpl.ImeComposingStatus mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
    private String mIMEComposingStr = null;
    private SemWebView mLWEView = null;
    private InputMethodManager mIMM = null;

    private EGL10 mEgl;
    private EGLDisplay mEglDisplay;
    private EGLContext mEglContext;
    private EGLSurface mEglSurface;


    private int findConfigAttrib(EGLConfig config, int attribute, int defaultValue) {
        int[] value = new int[1];
        if (mEgl.eglGetConfigAttrib(mEglDisplay, config, attribute, value)) {
            return value[0];
        }
        return defaultValue;
    }

    private void initGLContext() {
        if (mEglSurface != null && mEglContext != null) {
            return;
        }

        final int RED_SIZE = 8;
        final int GREEN_SIZE = 8;
        final int BLUE_SIZE = 8;
        final int ALPHA_SIZE = 8;

        final int DEPTH_SIZE = 0;
        final int STENCIL_SIZE = 1;

        mEgl = (EGL10) EGLContext.getEGL();
        mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

        if (mEglDisplay == EGL10.EGL_NO_DISPLAY) {
            throw new RuntimeException("eglGetDisplay failed");
        }

        int[] version = new int[2];
        if (!mEgl.eglInitialize(mEglDisplay, version)) {
            throw new RuntimeException("eglInitialize failed");
        }

        int[] configSpec = {
                EGL10.EGL_RED_SIZE, RED_SIZE, EGL10.EGL_GREEN_SIZE, GREEN_SIZE, EGL10.EGL_BLUE_SIZE, BLUE_SIZE,
                EGL10.EGL_ALPHA_SIZE, ALPHA_SIZE, EGL10.EGL_DEPTH_SIZE, DEPTH_SIZE, EGL10.EGL_STENCIL_SIZE, STENCIL_SIZE,
                EGL10.EGL_NONE
        };

        int[] value = new int[1];
        if (!mEgl.eglChooseConfig(mEglDisplay, configSpec, null, 1, value)) {
            throw new IllegalArgumentException("eglChooseConfig failed");
        }
        EGLConfig[] configs = new EGLConfig[value[0]];
        if (!mEgl.eglChooseConfig(mEglDisplay, configSpec, configs, value[0], value)) {
            throw new IllegalArgumentException("eglChooseConfig failed");
        }

        EGLConfig eglConfig = configs[0];
        for (EGLConfig c : configs) {
            int depth = findConfigAttrib(c, EGL10.EGL_DEPTH_SIZE, 0);
            int stencil = findConfigAttrib(c, EGL10.EGL_STENCIL_SIZE, 0);

            if ((depth >= DEPTH_SIZE) && (stencil >= STENCIL_SIZE)) {
                int r = findConfigAttrib(c, EGL10.EGL_RED_SIZE, 0);
                int g = findConfigAttrib(c, EGL10.EGL_GREEN_SIZE, 0);
                int b = findConfigAttrib(c, EGL10.EGL_BLUE_SIZE, 0);
                int a = findConfigAttrib(c, EGL10.EGL_ALPHA_SIZE, 0);
                if ((r == RED_SIZE) && (g == GREEN_SIZE)
                        && (b == BLUE_SIZE) && (a == ALPHA_SIZE)) {
                    eglConfig = c;
                    break;
                }
            }
        }

        int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
        int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 3,EGL10.EGL_NONE };

        mEglContext = mEgl.eglCreateContext(mEglDisplay, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list);
        mEglSurface = mEgl.eglCreateWindowSurface(mEglDisplay, eglConfig, mLWEView.getSurfaceTexture(), null);
    }

    private void destroyGLContext()
    {
        mEgl.eglDestroyContext(mEglDisplay, mEglContext);
        mEgl.eglDestroySurface(mEglDisplay, mEglSurface);
        mEgl.eglTerminate(mEglDisplay);
        mEglDisplay = null;
        mEglContext = null;
        mEglSurface = null;
    }

    public InputConnection getInputConnectionInstance(View view){
        return new ImeInputConnection(view);
    }

    public class ImeInputConnection extends BaseInputConnection {
        public ImeInputConnection(View view) {
            super(view, true);
            mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
        }

        @Override
        public boolean commitText(CharSequence text, int newCursorPosition) {
            final String newText = text.toString();
            synchronized (sWebViewThreadLocker) {
                if (sWebViewHandler != null) {
                    sWebViewHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            if (mWebViewInternalHandle != 0) {
                                int keyCode = (int) newText.charAt(0);
                                // only ascii printable
                                if ((32 <= keyCode) && (keyCode <= 126)) {
                                    dispatchKeyDown(mWebViewInternalHandle, keyCode, 0);
                                    dispatchKeyUp(mWebViewInternalHandle, keyCode, 0);
                                    // dispatchKeyPress(mWebViewInternalHandle,keyCode,0);
                                }
                            }
                        }
                    });
                }
            }
            return super.commitText(text, newCursorPosition);
        }

        @Override
        public boolean setComposingText(CharSequence text, int newCursorPosition) {
            final String newText = text.toString();
            synchronized (sWebViewThreadLocker) {
                if (sWebViewHandler != null) {
                    sWebViewHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            if (mWebViewInternalHandle != 0) {
                                mIMEComposingStr = newText.toString();
                                if (mComposingStatus == LweWebViewImpl.ImeComposingStatus.NORMAL) {
                                    mIMEComposingStr = newText.toString();
                                    dispatchCompositionStart(mWebViewInternalHandle,
                                            newText.toString());
                                    mComposingStatus = LweWebViewImpl.ImeComposingStatus.COMPOSING_START;
                                }
                                dispatchCompositionUpdate(mWebViewInternalHandle,
                                        newText.toString());
                            }
                        }
                    });
                }
            }

            return super.setComposingText(text, newCursorPosition);
        }

        @Override
        public boolean finishComposingText() {
            synchronized (sWebViewThreadLocker) {
                if (sWebViewHandler != null && mIMEComposingStr != null) {
                    sWebViewHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            if (mWebViewInternalHandle != 0) {
                                if (mComposingStatus ==
                                        LweWebViewImpl.ImeComposingStatus.COMPOSING_START) {
                                    dispatchCompositionEnd(mWebViewInternalHandle,
                                            mIMEComposingStr);
                                    mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
                                    mIMEComposingStr = null;
                                }
                            }
                        }
                    });
                }
            }
            return super.finishComposingText();
        }

    }

    public String getDefaultUA(){
        return mDefaultUserAgent;
    }

    public String getUA(){
        return mUserAgentString;
    }

    public int getCacheModeValue(){
        return mCacheMode;
    }

    public int getDefaultFontSize() { return mDefaultFontSize; }

    class StateChangeListener implements View.OnAttachStateChangeListener {
        @Override
        public void onViewAttachedToWindow(View v) {
        }

        @Override
        public void onViewDetachedFromWindow(View v) {
            synchronized (sWebViewThreadLocker) {
                if (sWebViewHandler != null) {
                    sWebViewHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            if (mWebViewInternalHandle != 0) {
                                destroy(mWebViewInternalHandle);
                            }
                            mWebViewInternalHandle = 0;
                            mWebViewClient = null;
                            destroyGLContext();
                        }
                    });
                }
            }
        }
    }

    public void initWebView(final View appView){
        mLWEView = (SemWebView)appView;
        Context appContext = mLWEView.getContext();

        mLWEView.setFocusable(true);
        mLWEView.setFocusableInTouchMode(true);

        mIMM = (InputMethodManager) appContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        sDpr = appContext.getResources().getDisplayMetrics().xdpi / 150;
        sLocalStoragePath = appContext.getDataDir().getAbsolutePath() + "/StarFish-localStorage";
        sCookiePath = appContext.getDataDir().getAbsolutePath() + "/StarFish-cookie";
        sCachePath = appContext.getCacheDir().getAbsolutePath() + "/StarFish-cache";

        final Object initLock = new Object();

        synchronized (sWebViewThreadLocker) {
            if (sWebViewThread == null) {
                sWebViewThread = new Thread() {

                    @Override
                    public void run() {
                        super.run();
                        Looper.prepare();

                        sWebViewHandler = new Handler() {
                            @Override
                            public void handleMessage(Message msg) {
                                super.handleMessage(msg);
                                // process incoming messages here
                            }
                        };

                        synchronized (sWebViewThreadLocker) {
                            sWebViewThreadLooper = Looper.myLooper();
                        }
                        synchronized(initLock) {
                            initLock.notifyAll();
                        }
                        Looper.loop();
                        sWebViewHandler = null;
                    }
                };
                sWebViewThread.setPriority(Thread.MAX_PRIORITY);
                sWebViewThread.start();
            }
        }
        if(sWebViewThread==null){
            synchronized(initLock) {
                try {
                    initLock.wait();
                } catch (Exception e) {}
            }
        }


        mLWEView.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            @Override
            public void onFocusChange(View v, boolean hasFocus) {
                final boolean hasfocus = hasFocus;
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        synchronized (sWebViewThreadLocker) {
                            if (mWebViewInternalHandle != 0) {
                                if (hasfocus) {
                                    focus(mWebViewInternalHandle);
                                } else {
                                    blur(mWebViewInternalHandle);
                                }
                            }
                        }
                    }
                });
            }
        });

        mLWEView.setSurfaceTextureListener(new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
                initGLContext();
                init();
                sWebViewHandler.post(new Runnable(){
                    @Override
                    public void run() {
                        synchronized (sWebViewThreadLocker) {
                            if (mLWEView.getWidth() == 0 || sWebViewHandler == null) {
                                sWebViewHandler.post(this);
                                return;
                            }
                        }
                        mWindowWidth = mLWEView.getWidth();
                        mWindowHeight = mLWEView.getHeight();
                        if (mWebViewInternalHandle == 0) {
                            mWebViewInternalHandle =
                                    create(mWindowWidth, mWindowHeight, sDpr,
                                            mUserAgentString, sLocale, sTimezone,
                                            sLocalStoragePath, sCookiePath, sCachePath);
                        }
                    }
                });
            }

            @Override
            public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
                synchronized (sWebViewThreadLocker) {
                    mWindowWidth = width;
                    mWindowHeight = height;
                    if (sWebViewHandler != null) {
                        sWebViewHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mWebViewInternalHandle != 0) {
                                    updateBuffer(mWebViewInternalHandle, null, mWindowWidth, mWindowHeight, mWindowWidth * 4);
                                    resume(mWebViewInternalHandle);
                                }
                            }
                        });
                    }
                }
            }

            @Override
            public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
                synchronized (sWebViewThreadLocker) {
                    if (sWebViewHandler != null) {
                        sWebViewHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mWebViewInternalHandle != 0) {
                                    pause(mWebViewInternalHandle);
                                }
                            }
                        });
                    }
                }
                return true;
            }

            @Override
            public void onSurfaceTextureUpdated(SurfaceTexture surface) {
            }
        });

        synchronized (sWebViewThreadLocker) {
            mDefaultUserAgent = getDefaultUserAgent();
            if (mUserAgentString == null) {
                mUserAgentString = mDefaultUserAgent;
            }
        }

        mLWEView.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                char keyValue = (char)event.getUnicodeChar();
                switch (keyCode) {
                    case KeyEvent.KEYCODE_ENTER:
                        keyValue = 13; // ascii - CR
                        break;
                    case KeyEvent.KEYCODE_DEL:
                        keyValue = 8; // ascii - BS
                        break;
                    case KeyEvent.KEYCODE_POWER:
                    case KeyEvent.KEYCODE_BACK:
                    case KeyEvent.KEYCODE_SEARCH:
                    case KeyEvent.KEYCODE_CAMERA:
                    case KeyEvent.KEYCODE_VOLUME_UP:
                    case KeyEvent.KEYCODE_VOLUME_DOWN:
                    case KeyEvent.KEYCODE_VOLUME_MUTE:
                    case KeyEvent.KEYCODE_VOICE_ASSIST:
                        return false;
                }
                final char key = keyValue;
                final int eventAction = event.getAction();
                synchronized (sWebViewThreadLocker) {
                    if (sWebViewHandler != null && key != 0) {
                        sWebViewHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mWebViewInternalHandle != 0) {
                                    if (eventAction == KeyEvent.ACTION_DOWN) {
                                        dispatchKeyDown(mWebViewInternalHandle, key, 0);
                                    } else if (eventAction == KeyEvent.ACTION_UP) {
                                        dispatchKeyUp(mWebViewInternalHandle, key, 0);
                                    }
                                }
                            }
                        });
                    }
                }
                return true;
            }
        });

        mLWEView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, final MotionEvent motionEvent) {
                int[] location = new int[2];
                view.getLocationOnScreen(location);
                float screenX = motionEvent.getRawX();
                float screenY = motionEvent.getRawY();
                final float viewX = screenX - location[0];
                final float viewY = screenY - location[1];

                if (mLWEView.hasFocus() == false) {
                    mLWEView.requestFocus();
                }

                synchronized (sWebViewThreadLocker) {
                    if (sWebViewHandler != null) {
                        sWebViewHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mWebViewInternalHandle != 0) {
                                    if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
                                        dispatchMouseDown(mWebViewInternalHandle, viewX, viewY);
                                    } else if (motionEvent.getAction() == MotionEvent.ACTION_UP) {
                                        dispatchMouseUp(mWebViewInternalHandle, viewX, viewY);
                                    } else if (motionEvent.getAction() ==
                                            MotionEvent.ACTION_CANCEL) {
                                        dispatchMouseUp(mWebViewInternalHandle, viewX, viewY);
                                    } else if (motionEvent.getAction() == MotionEvent.ACTION_MOVE) {
                                        dispatchMouseMove(mWebViewInternalHandle, viewX, viewY,
                                                true, false);
                                    }
                                }
                            }
                        });
                    }
                }
                return true;
            }
        });
        mLWEView.addOnAttachStateChangeListener(new StateChangeListener());
    }


    static private int registerTimerMap(final HashMap<String, TimerData> map, int ms, int fn,
                                        int data) {
        final int mms = ms;
        final int uid = sTimerUid.addAndGet(1);
        Runnable r = new Runnable() {
            @Override
            public void run() {
                if (map.containsKey(uid + "")) {
                    TimerData td = map.get(uid + "");
                    // Log.e(sTag, String.format("java serviceQueueTimer %d %d %d called", uid,
                    // (int)td.fn, (int)td.data));
                    boolean ret = serviceQueueTimer(uid, td.fn, td.data);
                    // Log.e(sTag, String.format("java serviceQueueTimer %d %d %d call ended", uid,
                    // (int)td.fn, (int)td.data));
                    if (ret) {
                        sWebViewHandler.postDelayed(this, mms);
                    } else {
                        if (map.containsKey(uid + "")) {
                            map.remove(uid + "");
                        }
                    }

                }
            }
        };
        TimerData td = new TimerData();
        td.runnable = r;
        td.fn = fn;
        td.data = data;

        synchronized (map) {
            final long current = SystemClock.uptimeMillis();
            map.put(uid + "", td);
            sWebViewHandler.postDelayed(r, ms);
        }
        return uid;
    }

    static private int startIdler(int ms, int fn, int data) {
        return registerTimerMap(sIdlerMap, ms, fn, data);
    }

    static private int startTimer(int ms, int fn, int data) {
        return registerTimerMap(sTimerMap, ms, fn, data);
    }

    static private void cancelTimer(int uid) {
        if (sTimerMap.containsKey(uid + "")) {
            TimerData r = sTimerMap.get(uid + "");
            sWebViewHandler.removeCallbacks(r.runnable);
            sTimerMap.remove(uid + "");
        }
    }

    static private void cancelIdler(int uid) {
        if (sIdlerMap.containsKey(uid + "")) {
            TimerData r = sIdlerMap.get(uid + "");
            sWebViewHandler.removeCallbacks(r.runnable);
            sIdlerMap.remove(uid + "");
        }
    }

    static private void runAllRemainingIdler() {
        while (!sIdlerMap.isEmpty()) {
            String key = sIdlerMap.keySet().iterator().next();
            TimerData r = sIdlerMap.get(key);
            sWebViewHandler.removeCallbacks(r.runnable);
            r.runnable.run();
            sIdlerMap.remove(key);
        }
    }

    private void flushRendering(int updatedX, int updatedY, int updatedWidth, int updatedHeight) {
    }

    private void showDropdownMenu(String[] list, int checkedPosition) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mLWEView.getContext());
        builder.setSingleChoiceItems(list, checkedPosition,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        onDropdownMenuItemSelected(which);
                    }
                });

        builder.show();
    }

    private void onDropdownMenuItemSelected(int position) {
        final int positionIdx = position;
        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        onDropdownMenuItemSelected(mWebViewInternalHandle, positionIdx);
                    }
                }
            });
        }
    }

    private void showAlert(String title, String message) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mLWEView.getContext());
        builder.setTitle(title);
        builder.setMessage(message);
        builder.setPositiveButton("OK", null);

        AlertDialog dialog = builder.create();
        dialog.show();
    }


    // Listener called from native code
    private void onLoadResource(String url) {
        if (mWebViewClient != null) {
            mWebViewClient.onLoadResource(mLWEView, url);
        }
    }

    private void onReceivedError(int errorCode, boolean canGoBack, boolean canGoForward) {
        mCanGoBack = canGoBack;
        mCanGoForward = canGoForward;
        if (mWebViewClient != null) {
            //TODO
            mWebViewClient.onReceivedError(mLWEView, new SemWebResourceError(errorCode, "NotSupported"));
        }
    }

    private void onPageFinished(String url, boolean canGoBack, boolean canGoForward) {
        mCurrentURL = url;
        mCanGoBack = canGoBack;
        mCanGoForward = canGoForward;
        if (mWebViewClient != null) {
            mWebViewClient.onPageFinished(mLWEView, url);
        }
    }

    private void onPageStarted(String url, boolean canGoBack, boolean canGoForward) {
        mCurrentURL = url;
        mCanGoBack = canGoBack;
        mCanGoForward = canGoForward;
        if (mWebViewClient != null) {
            mWebViewClient.onPageStarted(mLWEView, url);
        }
    }

    private boolean shouldOverrideUrlLoading(String request) {
        if (mWebViewClient != null) {
            return mWebViewClient.shouldOverrideUrlLoading(mLWEView, request);
        }
        return false;
    }

    private void onProgressChanged(int newProgres) {
        if (mWebLweClient != null) {
            mWebLweClient.onProgressChanged(mLWEView, newProgres);
        }
    }

    private void onDownloadStart(String url, String userAgent, String contentDisposition,
                                 String mimetype, long contentLength) {
        if (mDownloadListener != null) {
            mDownloadListener.onDownloadStart(url, userAgent, contentDisposition,
                    mimetype, contentLength);
        }
    }

    // public boolean onShowFileChooser(WebView webView, ValueCallback<Uri[]> Uris)

    private void showSoftKeyboard() {
        mLWEView.post(new Runnable() {
            @Override
            public void run() {
                if (mLWEView != null) {
                    if (mIMM == null) {
                        mIMM = (InputMethodManager) mLWEView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    }

                    if (mLWEView.hasFocus() == false) {
                        if (mLWEView.requestFocus() == false) {
                            Log.w(sTag,"Failed to request focus");
                        }
                    }

                    mIMM.showSoftInput(mLWEView, InputMethodManager.SHOW_IMPLICIT);
                    mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
                }
            }
        });
    }

    private void hideSoftKeyboard() {
        mLWEView.post(new Runnable() {
            @Override
            public void run() {
                if (mLWEView != null && mIMM != null) {

                    if (mIMM == null) {
                        mIMM = (InputMethodManager) mLWEView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                    }
                    mIMM.hideSoftInputFromWindow(mLWEView.getWindowToken(), 0);
                    mComposingStatus = LweWebViewImpl.ImeComposingStatus.NORMAL;
                }
            }
        });
    }


    public void loadUrl(final String url) {
        if (url == null) {
            return;
        }
        if(!isReadyWebViewInstance(new Runnable() {
            @Override
            public void run() {
                loadUrl(url);
            }
        })){
            return;
        }

        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        loadUrl(mWebViewInternalHandle, url);
                    }
                });
            }
        }
    }

    public String getUrl() {
        return mCurrentURL;
    }

    private boolean isReadyWebViewInstance(Runnable r) {
        if(sWebViewHandler != null){
            if(mWebViewInternalHandle != 0){
                return true;
            }
            synchronized (sWebViewThreadLocker) {
                sWebViewHandler.post(r);
            }
        }
        return false;
    }

    public void loadData(String data) {
        if (data == null) {
            return;
        }

        final String htmlData = data;
        if(!isReadyWebViewInstance(new Runnable() {
            @Override
            public void run() {
                loadData(htmlData);
            }
        })){
            return;
        }

        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        loadData(mWebViewInternalHandle, htmlData);
                    }
                });
            }
        }
    }

    public void reload() {
        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            reload(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public void stopLoading() {
        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            stopLoading(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public void goBack() {
        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            goBack(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public void goForward() {
        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            goForward(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public boolean canGoBack() {
        return mCanGoBack;
    }
    public boolean canGoForward() {
        return mCanGoForward;
    }


    public void addJavascriptInterface(final Object object, final String name) {
        if ((object == null) || (name == null)) {
            return;
        }

        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        Method[] methods = object.getClass().getMethods();
                        for (Method m : methods) {
                            if (m.isAnnotationPresent(JavascriptInterface.class)) {
                                if (m.getReturnType().toString().equals("class java.lang.String") &&
                                        m.getParameterTypes()[0].toString().equals(
                                                "class java.lang.String")) {
                                    addJavascriptInterface(mWebViewInternalHandle, name,
                                            m.getName(), object);
                                } else {
                                    Log.d(sTag, "ERROR: addJavascriptInterface : invalid signature");
                                }
                            }
                        }
                    }
                }
            });
        }
    }

    public void removeJavascriptInterface(final String name) {
        if (name == null) {
            return;
        }

        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        removeJavascriptInterface(mWebViewInternalHandle, name);
                    }
                }
            });
        }
    }

    public void clearCache() {
        synchronized (sWebViewThreadLocker) {
            if (sWebViewHandler != null) {
                sWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            clearCache(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public void evaluateJavascript(final String script,
                                   final ValueCallback<String> resultCallback) {
        if ((script == null) || (resultCallback == null)) {
            return;
        }

        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        String resultStr = evaluateJavaScript(mWebViewInternalHandle, script);
                        if (resultCallback != null) {
                            resultCallback.onReceiveValue(resultStr);
                        }
                    }
                }
            });
        }
    }

    public void clearHistory() {
        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        clearHistory(mWebViewInternalHandle);
                    }
                }
            });
        }
    }

    public void setSettings(SemWebSettings settings) {
        if (settings == null) {
            return;
        }

        final String UA = mUserAgentString = settings.getUserAgentString();
        final int cacheMode = mCacheMode = settings.getCacheMode();
        final int defaultFontSize = mDefaultFontSize = settings.getDefaultFontSize();
        if (sWebViewHandler != null) {
            sWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        setUserAgentString(mWebViewInternalHandle, UA);
                        setCacheMode(mWebViewInternalHandle, cacheMode);
                        setDefaultFontSize(mWebViewInternalHandle, defaultFontSize);
                    }
                }
            });
        }
    }

    private void glMakeCurrent() {
        if (mEgl != null) {
            mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext);
        }
    }

    private void glSwapBuffers() {
        if (mEgl != null) {
            mEgl.eglSwapBuffers(mEglDisplay, mEglSurface);
        }
    }

    public void setWebViewClient(SemWebViewClient client) {
        mWebViewClient = client;
    }
    public void setWebLweClient(SemWebLweClient client) {
        mWebLweClient = client;
    }
    public void setDownloadListener(SemDownloadListener listener) {
        mDownloadListener = listener;
    }

    // Following methods are internal use only
    native private void loadUrl(long starFish, String url);
    native private void loadData(long starFish, String data);
    native private long create(int initialNaturalWidth, int initialNaturalHeight,
                               float devicePixelRatio, String userAgentString, String locale,
                               String timezoneID, String localstoragePath, String cookiePath,
                               String cachePath);
    native private void destroy(long starFish);
    native private void goBack(long starFish);
    native private void goForward(long starFish);
    native private void reload(long starFish);
    native private void stopLoading(long starFish);
    native private void clearHistory(long starFish);
    native private void clearCache(long starFish);
    native private void pause(long starFish);
    native private void resume(long starFish);
    native private void focus(long starFish);
    native private void blur(long starFish);

    native private void addJavascriptInterface(long starFish, String objectName,
                                               String functionName, Object instance);
    native private void removeJavascriptInterface(long starFish, String objectName);
    native private String evaluateJavaScript(long starFish, String data);
    native private String getDefaultUserAgent();
    native private void setUserAgentString(long starFish, String userAgent);
    native private void setCacheMode(long starFish, int mode);
    native private void setDefaultFontSize(long starFish, int size);
    static native private void init();
    static native private void updateBuffer(long starFish, Bitmap b, int w, int h, int stride);
    static native private boolean serviceQueueTimer(int uid, int fn, int data);
    static native private void dispatchMouseDown(long starFish, float x, float y);
    static native private void dispatchMouseMove(long starFish, float x, float y,
                                                 boolean isLButtonPressed,
                                                 boolean isRButtonPressed);
    static native private void dispatchMouseUp(long starFish, float x, float y);
    static native private void dispatchKeyDown(long starFish, int keyValue, int modifier);
    static native private void dispatchKeyUp(long starFish, int keyValue, int modifier);
    static native private void dispatchKeyPress(long starFish, int keyValue, int modifier);
    static native private void dispatchCompositionStart(long starFish, String Value);
    static native private void dispatchCompositionUpdate(long starFish, String Value);
    static native private void dispatchCompositionEnd(long starFish, String Value);
    native private void onDropdownMenuItemSelected(long starFish, int position);


}