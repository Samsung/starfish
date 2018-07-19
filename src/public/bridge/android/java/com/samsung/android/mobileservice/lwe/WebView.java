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

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Rect;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.os.Trace;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.webkit.DownloadListener;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;
import android.webkit.WebResourceRequest;

import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicInteger;


public class WebView extends SurfaceView {
    static {
        System.loadLibrary("lightweightwebengine");
    }
    static class TimerData {
        Runnable runnable;
        int fn;
        int data;
    }
    static HashMap<String, TimerData> mTimerMap = new HashMap<>();
    static HashMap<String, TimerData> mIdlerMap = new HashMap<>();
    static AtomicInteger mTimerUID = new AtomicInteger(0);

    protected static String TAG = "StarFish";
    protected static float DPR = 1;

    protected static String LOCALE = "ko-KR";
    protected static String TIMEZONE = "Asia/Seoul";
    protected static String localStoragePath;
    protected static String cookiePath;
    protected static String cachePath;

    protected static Handler mWebViewHandler;
    protected static Integer mWebViewThreadLocker = new Integer(0);
    protected static Looper mWebViewThreadLooper;
    protected static Thread mWebViewThread;
//    protected Runnable mRendering = null;

    protected Bitmap mScreenBuffer;
    protected long mWebViewInternalHandle;

    protected int mWindowWidth;
    protected int mWindowHeight;

    private boolean mCanGoBack = false;
    private boolean mCanGoForward = false;
    private String mCurrentURL = null;
    private WebViewClient mWebViewClient = null;
    private DownloadListener mDownloadListener = null;

    private int mCacheMode = Settings.LOAD_DEFAULT;
    private String mDefaultUserAgent = null;
    private String mUserAgentString = null;

    protected void initWebView(){
        DPR = getContext().getResources().getDisplayMetrics().xdpi/150;
        localStoragePath = getContext().getDataDir().getAbsolutePath()+"/StarFish-localStorage";
        cookiePath = getContext().getDataDir().getAbsolutePath()+"/StarFish-cookie";
        cachePath = getContext().getCacheDir().getAbsolutePath()+"/StarFish-cache";
        init();
        synchronized (mWebViewThreadLocker) {
            mDefaultUserAgent = getDefaultUserAgent();
            if(mUserAgentString==null){
                mUserAgentString = mDefaultUserAgent;
            }
        }
        getHolder().setFormat(PixelFormat.RGBA_8888);
        getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder surfaceHolder) {}

            @Override
            public void surfaceChanged(SurfaceHolder surfaceHolder, int i, final int w, final int h) {
                synchronized (mWebViewThreadLocker) {
                    mWindowWidth = w;
                    mWindowHeight = h;
                    mScreenBuffer = Bitmap.createBitmap(mWindowWidth, mWindowHeight, Bitmap.Config.ARGB_8888);
                    if (mWebViewHandler != null) {
                        mWebViewHandler.post(new Runnable() {
                            @Override
                            public void run()
                            {
                                if (mWebViewInternalHandle != 0 && mScreenBuffer!=null) {
                                    updateBuffer(mWebViewInternalHandle,mScreenBuffer, mWindowWidth, mWindowHeight,mWindowWidth*4);
                                    Resume(mWebViewInternalHandle);
                                }
                            }
                        });
                    }
                }
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
                synchronized (mWebViewThreadLocker) {
                    mScreenBuffer = null;
                    if (mWebViewHandler != null) {
                        mWebViewHandler.post(new Runnable() {
                            @Override
                            public void run()
                            {
                                if (mWebViewInternalHandle != 0) {
                                    Pause(mWebViewInternalHandle);
                                }
                            }
                        });
                    }
                }
            }
        });

        synchronized (mWebViewThreadLocker) {
            if (mWebViewThread == null) {
                mWebViewThread = new Thread() {

                    @Override
                    public void run() {
                        Looper.prepare();

                        mWebViewHandler = new Handler() {
                            public void handleMessage(Message msg) {
                                // process incoming messages here
                            }
                        };

                        synchronized (mWebViewThreadLocker) {
                            mWebViewThreadLooper = Looper.myLooper();
                        }

                        Looper.loop();
                        mWebViewHandler = null;
                    }
                };
                mWebViewThread.setPriority(Thread.MAX_PRIORITY);
                mWebViewThread.start();
            }
        }

        post(new Runnable() {
            @Override
            public void run() {
                synchronized (mWebViewThreadLocker) {
                    if (getWidth() == 0 || mWebViewHandler == null) {
                        post(this);
                        return;
                    }
                }
                mWindowWidth = getWidth();
                mWindowHeight = getHeight();

            }
        });

        setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, final MotionEvent motionEvent) {

                int[] location = new int[2];
                view.getLocationOnScreen(location);
                float screenX = motionEvent.getRawX();
                float screenY = motionEvent.getRawY();
                final float viewX = screenX - location[0];
                final float viewY = screenY - location[1];

                synchronized (mWebViewThreadLocker) {
                    if (mWebViewHandler != null) {
                        mWebViewHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mWebViewInternalHandle != 0) {
                                    if (motionEvent.getAction() == MotionEvent.ACTION_DOWN) {
                                        dispatchMouseDown(mWebViewInternalHandle, viewX, viewY);
                                    } else if (motionEvent.getAction() == MotionEvent.ACTION_UP) {
                                        dispatchMouseUp(mWebViewInternalHandle, viewX, viewY);
                                    } else if (motionEvent.getAction() == MotionEvent.ACTION_CANCEL) {
                                        dispatchMouseUp(mWebViewInternalHandle, viewX, viewY);
                                    } else if (motionEvent.getAction() == MotionEvent.ACTION_MOVE) {
                                        dispatchMouseMove(mWebViewInternalHandle, viewX, viewY,true,false);
                                    }
                                }
                            }
                        });
                    }
                }
                return true;
            }
        });
    }

    class StateChangeListener implements View.OnAttachStateChangeListener{
        @Override
        public void onViewAttachedToWindow(View v){}

        @Override
        public void onViewDetachedFromWindow(View v){

            synchronized (mWebViewThreadLocker) {
                if (mWebViewHandler != null) {
                    mWebViewHandler.post(new Runnable() {
                        @Override
                        public void run() {
                        if (mWebViewInternalHandle != 0) {
                            Destroy(mWebViewInternalHandle);
                        }
                        mWebViewInternalHandle = 0;
                        mWebViewClient = null;
                        }
                    });
                }
            }
        }
    }

    public WebView(Context context){
        super(context);
        initWebView();
        super.addOnAttachStateChangeListener(new StateChangeListener());
    }

    public WebView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initWebView();
        super.addOnAttachStateChangeListener(new StateChangeListener());
    }

    public WebView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs);
        initWebView();
        super.addOnAttachStateChangeListener(new StateChangeListener());
    }

    // Called by JNI

    private void onLoadResource(String url) {
        if(mWebViewClient!=null){
            mWebViewClient.onLoadResource(this,url);
        }
    }

    private void onReceivedError(int errorCode,boolean canGoBack, boolean canGoForward){
        mCanGoBack = canGoBack;
        mCanGoForward = canGoForward;
        if(mWebViewClient!=null){
            //TODO
            mWebViewClient.onReceivedError(this,new ResourceError(errorCode,"NotSupported"));
        }
    }

    private void onPageFinished(String url,boolean canGoBack, boolean canGoForward){
        mCurrentURL = url;
        mCanGoBack = canGoBack;
        mCanGoForward = canGoForward;
        if(mWebViewClient!=null){
            mWebViewClient.onPageFinished(this,url);
        }
    }

    private void onPageStarted(String url,boolean canGoBack, boolean canGoForward){
        mCurrentURL = url;
        mCanGoBack = canGoBack;
        mCanGoForward = canGoForward;
        if(mWebViewClient!=null){
            mWebViewClient.onPageStarted(this,url);
        }
    }

    private boolean shouldOverrideUrlLoading (String request) {
        if (mWebViewClient != null) {
            return mWebViewClient.shouldOverrideUrlLoading(this, request);
        }
        return false;
    }

    private void onProgressChanged(int newProgres){
        if(mWebViewClient!=null){
            mWebViewClient.onProgressChanged(this,newProgres);
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

    // Called by JNI
    static private int registerTimerMap(final HashMap<String, TimerData> map,int ms, int fn, int data) {
        final int mms = ms;
        final int uid = mTimerUID.addAndGet(1);
        Runnable r = new Runnable() {
            @Override
            public void run() {
                if (map.containsKey(uid+"")) {
                    TimerData td = map.get(uid+"");
//                     Log.e(TAG, String.format("java serviceQueueTimer %d %d %d call!!", uid, (int)td.fn, (int)td.data));
                    boolean ret = serviceQueueTimer(uid, td.fn, td.data);
//                     Log.e(TAG, String.format("java serviceQueueTimer %d %d %d callend!!", uid, (int)td.fn, (int)td.data));
                    if (ret) {
                        mWebViewHandler.postDelayed(this, mms);
                    } else {
                        if (map.containsKey(uid+"")) {
                            map.remove(uid+"");
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
            map.put(uid+"", td);
            mWebViewHandler.postDelayed(r, ms);
        }
        return uid;
    }

    static private int startIdler(int ms, int fn, int data) {
        return registerTimerMap(mIdlerMap,ms,fn,data);
    }

    static private int startTimer(int ms, int fn, int data) {
        return registerTimerMap(mTimerMap,ms,fn,data);
    }

    static private void cancelTimer(int uid)
    {
        if (mTimerMap.containsKey(uid+"")) {
            TimerData r = mTimerMap.get(uid+"");
            mWebViewHandler.removeCallbacks(r.runnable);
            mTimerMap.remove(uid+"");
        }
    }

    static private void cancelIdler(int uid)
    {
        if (mIdlerMap.containsKey(uid+"")) {
            TimerData r = mIdlerMap.get(uid+"");
            mWebViewHandler.removeCallbacks(r.runnable);
            mIdlerMap.remove(uid+"");
        }
    }

    static private void runAllRemainingIdler()
    {
        while(!mIdlerMap.isEmpty()){
            String key = mIdlerMap.keySet().iterator().next();
            TimerData r = mIdlerMap.get(key);
            mWebViewHandler.removeCallbacks(r.runnable);
            r.runnable.run();
            mIdlerMap.remove(key);
        }
    }

    private void flushRendering(int updatedX,int updatedY,int updatedWidth,int updatedHeight) {
        synchronized (mWebViewThreadLocker) {
            if (mScreenBuffer != null && mWebViewInternalHandle != 0) {
                Canvas canvas = getHolder().lockCanvas();
                if (canvas != null) {
                    Paint paint = new Paint();
                    paint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_OVER));
                    Rect updateRect = new Rect(updatedX,updatedY,updatedWidth,updatedHeight);
                    canvas.drawBitmap(mScreenBuffer, updateRect, updateRect, paint);
                    getHolder().unlockCanvasAndPost(canvas);
                }
            }
        }
    }

    public void loadUrl(final String URL){
        synchronized (mWebViewThreadLocker) {
            if (mWebViewHandler != null) {
                mWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle == 0) {
                            mWebViewInternalHandle = WebView.this.Create(mWindowWidth, mWindowHeight, DPR, mUserAgentString,LOCALE,TIMEZONE,localStoragePath,cookiePath,cachePath);
                        }
                        loadUrl(mWebViewInternalHandle, URL);
                    }
                });
            }
        }
    }

    public String getUrl(){
        return mCurrentURL;
    }

    public void loadData(String data){
        final String htmlData = data;
        synchronized (mWebViewThreadLocker) {
            if (mWebViewHandler != null) {
                mWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle == 0) {
                            mWebViewInternalHandle = Create(mWindowWidth, mWindowHeight, DPR, mUserAgentString,LOCALE,TIMEZONE,localStoragePath,cookiePath,cachePath);
                        }
                        loadData(mWebViewInternalHandle, htmlData);
                    }
                });
            }
        }
    }

    public void reload(){
        synchronized (mWebViewThreadLocker) {
            if (mWebViewHandler != null) {
                mWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            Reload(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public void stopLoading(){
        synchronized (mWebViewThreadLocker) {
            if (mWebViewHandler != null) {
                mWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            StopLoading(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public void goBack(){
        synchronized (mWebViewThreadLocker) {
            if (mWebViewHandler != null) {
                mWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            GoBack(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public void goForward(){
        synchronized (mWebViewThreadLocker) {
            if (mWebViewHandler != null) {
                mWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            GoForward(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }

    public boolean canGoBack(){
        return mCanGoBack;
    }

    public boolean canGoForward(){
        return mCanGoForward;
    }

    public void addJavascriptInterface(final Object nativeCB,final String globalObjName){
        if (mWebViewHandler != null) {
            mWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        Method[] methods = nativeCB.getClass().getMethods();
                        for (Method m : methods) {
                            if (m.isAnnotationPresent(JavascriptInterface.class)) {
                                if(m.getReturnType().toString().equals("class java.lang.String") &&
                                        m.getParameterTypes()[0].toString().equals("class java.lang.String")) {
                                    addJavascriptInterface(mWebViewInternalHandle, globalObjName, m.getName(), nativeCB);
                                }else{
                                    Log.d(TAG,"ERROR: addJavascriptInterface : invalid signature");
                                }
                            }
                        }
                    }
                }
            });
        }
    }

    public void removeJavascriptInterface(final String globalObjName){
        if (mWebViewHandler != null) {
            mWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        removeJavascriptInterface(mWebViewInternalHandle, globalObjName);
                    }
                }
            });
        }
    }

    public void clearCache(){
        synchronized (mWebViewThreadLocker) {
            if (mWebViewHandler != null) {
                mWebViewHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (mWebViewInternalHandle != 0) {
                            ClearCache(mWebViewInternalHandle);
                        }
                    }
                });
            }
        }
    }


    public void evaluateJavascript(final String scriptData, final ValueCallback<String> resultCB) {
        if (mWebViewHandler != null) {
            mWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        String resultStr = EvaluateJavaScript(mWebViewInternalHandle,scriptData);
                        if (resultCB != null) {
                            resultCB.onReceiveValue(resultStr);
                        }
                    }
                }
            });
        }
    }

    public void clearHistory(){
        if (mWebViewHandler != null) {
            mWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                if (mWebViewInternalHandle != 0) {
                    ClearHistory(mWebViewInternalHandle);
                }
                }
            });
        }
    }

    public Settings getSettings(){
        return new Settings(mDefaultUserAgent,mUserAgentString,mCacheMode);
    }

    public void setSettings(Settings setttings){
        final String UA = mUserAgentString = setttings.getUserAgentString();
        final int cacheMode = mCacheMode = setttings.getCacheMode();
        if (mWebViewHandler != null) {
            mWebViewHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mWebViewInternalHandle != 0) {
                        setUserAgentString(mWebViewInternalHandle, UA);
                        setCacheMode(mWebViewInternalHandle, cacheMode);
                    }
                }
            });
        }
    }

    public void setWebViewClient(WebViewClient client){
        mWebViewClient = client;
    }
    public void setDownloadListener(DownloadListener listener) { mDownloadListener = listener; }

    native public void loadUrl(long starFish, String url);
    native public void loadData(long starFish, String data);
    native public String EvaluateJavaScript(long starFish, String data);

    native public long Create(int initialNaturalWidth, int initialNaturalHeight, float devicePixelRatio, String userAgentString,String locale,String timezoneID,String localstoragePath,String cookiePath, String cachePath);
    native public void Destroy(long starFish);
    native public void GoBack(long starFish);
    native public void GoForward(long starFish);
    native public void Reload(long starFish);
    native public void StopLoading(long starFish);
    native public void ClearHistory(long starFish);
    native public void ClearCache(long starFish);
    native public void Pause(long starFish);
    native public void Resume(long starFish);

    native public void addJavascriptInterface(long starFish, String objectName, String functionName, Object instance);
    native public void removeJavascriptInterface(long starFish, String objectName);


    // For internal purpose
    native public String getDefaultUserAgent();
    native public void setUserAgentString(long starFish, String userAgent);
    native public void setCacheMode(long starFish, int mode);

    static native public void init();
    static native public void updateBuffer(long starFish, Bitmap b, int w, int h, int stride);
    static native public boolean serviceQueueTimer(int uid, int fn, int data);
    static native public void dispatchMouseDown(long starFish, float x, float y);
    static native public void dispatchMouseMove(long starFish, float x, float y,boolean isLButtonPressed, boolean isRButtonPressed);
    static native public void dispatchMouseUp(long starFish, float x, float y);
}
