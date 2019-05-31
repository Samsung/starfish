/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

package com.samsung.android.lwe;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.graphics.Bitmap;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.webkit.DownloadListener;
import android.webkit.JavascriptInterface;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.AbsoluteLayout;
import android.net.Uri;

import java.lang.reflect.Constructor;
import dalvik.system.PathClassLoader;

/**
 * This class is a view that displays Web pages.
 *
 * <h3>Usage Information</h3>
 *
 * <p>SemWebView is a lightweight WebView that is designed to be memory-efficient than
 * Android WebView. Here are some guidelines on using SemWebView.</p>
 *
 * <ol>
 * <li>Decide when to use SemWebView instead of stock WebView. SemWebView does not
 *    aim to replace the stock WebView, but aims to reduce runtime memory usage when
 *    using the supported APIs. Once developers decide to use SemWebView, or
 *    update existing code already written in WebView, they may need to write
 *    an separate implementation path, as not all APIs supported by WebView are
 *    supported by SemWebView.</li>
 *
 * <li> SemWebView is an optional component, and hence, some devices could come
 *    without having SemWebView installed. In the case where SemWebView does not
 *    come with a device, all operations made on SemWebView will be forwarded to
 *    the stock WebView, and works as if Android WebView were used.</li>
 *
 * <li> Supported Web pages are controlled by a whitelist. To add URLs to the whitelist,
 *    please contact <code>duddlf.choi@samsung.com</code>.</li>
 * </ol>
 *
 * @since SEP 10.2
 */
public class SemWebView extends SurfaceView {
    private static final boolean isPlatformCode = false;

    private static PathClassLoader pcl = null;
    /**
     * @hide
     */
    public static final String PACKAGE_NAME = "com.samsung.android.lwe";
    private static final String LweWebViewImpl = "com.samsung.android.lwe.LweWebViewImpl";
    private static boolean USE_LWE = true;

    private final Signature googlePlatformKey =
        new Signature("308204a830820390a003020102020900b3998086d056cffa300d06092a864886f70d0101040500308194310b3009060355040613025553311330110603550408130a43616c69666f726e6961311630140603550407130d4d6f756e7461696e20566965773110300e060355040a1307416e64726f69643110300e060355040b1307416e64726f69643110300e06035504031307416e64726f69643122302006092a864886f70d0109011613616e64726f696440616e64726f69642e636f6d301e170d3038303431353232343035305a170d3335303930313232343035305a308194310b3009060355040613025553311330110603550408130a43616c69666f726e6961311630140603550407130d4d6f756e7461696e20566965773110300e060355040a1307416e64726f69643110300e060355040b1307416e64726f69643110300e06035504031307416e64726f69643122302006092a864886f70d0109011613616e64726f696440616e64726f69642e636f6d30820120300d06092a864886f70d01010105000382010d003082010802820101009c780592ac0d5d381cdeaa65ecc8a6006e36480c6d7207b12011be50863aabe2b55d009adf7146d6f2202280c7cd4d7bdb26243b8a806c26b34b137523a49268224904dc01493e7c0acf1a05c874f69b037b60309d9074d24280e16bad2a8734361951eaf72a482d09b204b1875e12ac98c1aa773d6800b9eafde56d58bed8e8da16f9a360099c37a834a6dfedb7b6b44a049e07a269fccf2c5496f2cf36d64df90a3b8d8f34a3baab4cf53371ab27719b3ba58754ad0c53fc14e1db45d51e234fbbe93c9ba4edf9ce54261350ec535607bf69a2ff4aa07db5f7ea200d09a6c1b49e21402f89ed1190893aab5a9180f152e82f85a45753cf5fc19071c5eec827020103a381fc3081f9301d0603551d0e041604144fe4a0b3dd9cba29f71d7287c4e7c38f2086c2993081c90603551d230481c13081be80144fe4a0b3dd9cba29f71d7287c4e7c38f2086c299a1819aa48197308194310b3009060355040613025553311330110603550408130a43616c69666f726e6961311630140603550407130d4d6f756e7461696e20566965773110300e060355040a1307416e64726f69643110300e060355040b1307416e64726f69643110300e06035504031307416e64726f69643122302006092a864886f70d0109011613616e64726f696440616e64726f69642e636f6d820900b3998086d056cffa300c0603551d13040530030101ff300d06092a864886f70d01010405000382010100572551b8d93a1f73de0f6d469f86dad6701400293c88a0cd7cd778b73dafcc197fab76e6212e56c1c761cfc42fd733de52c50ae08814cefc0a3b5a1a4346054d829f1d82b42b2048bf88b5d14929ef85f60edd12d72d55657e22e3e85d04c831d613d19938bb8982247fa321256ba12d1d6a8f92ea1db1c373317ba0c037f0d1aff645aef224979fba6e7a14bc025c71b98138cef3ddfc059617cf24845cf7b40d6382f7275ed738495ab6e5931b9421765c491b72fb68e080dbdb58c2029d347c8b328ce43ef6a8b15533edfbe989bd6a48dd4b202eda94c6ab8dd5b8399203daae2ed446232e4fe9bd961394c6300e5138e3cfd285e6e4e483538cb8b1b357");
    private final Signature googleTestKey =
        new Signature("308204a830820390a003020102020900936eacbe07f201df300d06092a864886f70d0101050500308194310b3009060355040613025553311330110603550408130a43616c69666f726e6961311630140603550407130d4d6f756e7461696e20566965773110300e060355040a1307416e64726f69643110300e060355040b1307416e64726f69643110300e06035504031307416e64726f69643122302006092a864886f70d0109011613616e64726f696440616e64726f69642e636f6d301e170d3038303232393031333334365a170d3335303731373031333334365a308194310b3009060355040613025553311330110603550408130a43616c69666f726e6961311630140603550407130d4d6f756e7461696e20566965773110300e060355040a1307416e64726f69643110300e060355040b1307416e64726f69643110300e06035504031307416e64726f69643122302006092a864886f70d0109011613616e64726f696440616e64726f69642e636f6d30820120300d06092a864886f70d01010105000382010d00308201080282010100d6931904dec60b24b1edc762e0d9d8253e3ecd6ceb1de2ff068ca8e8bca8cd6bd3786ea70aa76ce60ebb0f993559ffd93e77a943e7e83d4b64b8e4fea2d3e656f1e267a81bbfb230b578c20443be4c7218b846f5211586f038a14e89c2be387f8ebecf8fcac3da1ee330c9ea93d0a7c3dc4af350220d50080732e0809717ee6a053359e6a694ec2cb3f284a0a466c87a94d83b31093a67372e2f6412c06e6d42f15818dffe0381cc0cd444da6cddc3b82458194801b32564134fbfde98c9287748dbf5676a540d8154c8bbca07b9e247553311c46b9af76fdeeccc8e69e7c8a2d08e782620943f99727d3c04fe72991d99df9bae38a0b2177fa31d5b6afee91f020103a381fc3081f9301d0603551d0e04160414485900563d272c46ae118605a47419ac09ca8c113081c90603551d230481c13081be8014485900563d272c46ae118605a47419ac09ca8c11a1819aa48197308194310b3009060355040613025553311330110603550408130a43616c69666f726e6961311630140603550407130d4d6f756e7461696e20566965773110300e060355040a1307416e64726f69643110300e060355040b1307416e64726f69643110300e06035504031307416e64726f69643122302006092a864886f70d0109011613616e64726f696440616e64726f69642e636f6d820900936eacbe07f201df300c0603551d13040530030101ff300d06092a864886f70d010105050003820101007aaf968ceb50c441055118d0daabaf015b8a765a27a715a2c2b44f221415ffdace03095abfa42df70708726c2069e5c36eddae0400be29452c084bc27eb6a17eac9dbe182c204eb15311f455d824b656dbe4dc2240912d7586fe88951d01a8feb5ae5a4260535df83431052422468c36e22c2a5ef994d61dd7306ae4c9f6951ba3c12f1d1914ddc61f1a62da2df827f603fea5603b2c540dbd7c019c36bab29a4271c117df523cdbc5f3817a49e0efa60cbd7f74177e7a4f193d43f4220772666e4c4d83e1bd5a86087cf34f2dec21e245ca6c2bb016e683638050d2c430eea7c26a1c49d3760a58ab7f1a82cc938b4831384324bd0401fa12163a50570e684d");

    private final Signature samsungPlatformKey =
        new Signature("308204d4308203bca003020102020900d20995a79c0daad6300d06092a864886f70d01010505003081a2310b3009060355040613024b52311430120603550408130b536f757468204b6f726561311330110603550407130a5375776f6e2043697479311c301a060355040a131353616d73756e6720436f72706f726174696f6e310c300a060355040b1303444d43311530130603550403130c53616d73756e6720436572743125302306092a864886f70d0109011616616e64726f69642e6f734073616d73756e672e636f6d301e170d3131303632323132323531325a170d3338313130373132323531325a3081a2310b3009060355040613024b52311430120603550408130b536f757468204b6f726561311330110603550407130a5375776f6e2043697479311c301a060355040a131353616d73756e6720436f72706f726174696f6e310c300a060355040b1303444d43311530130603550403130c53616d73756e6720436572743125302306092a864886f70d0109011616616e64726f69642e6f734073616d73756e672e636f6d30820120300d06092a864886f70d01010105000382010d00308201080282010100c986384a3e1f2fb206670e78ef232215c0d26f45a22728db99a44da11c35ac33a71fe071c4a2d6825a9b4c88b333ed96f3c5e6c666d60f3ee94c490885abcf8dc660f707aabc77ead3e2d0d8aee8108c15cd260f2e85042c28d2f292daa3c6da0c7bf2391db7841aade8fdf0c9d0defcf77124e6d2de0a9e0d2da746c3670e4ffcdc85b701bb4744861b96ff7311da3603c5a10336e55ffa34b4353eedc85f51015e1518c67e309e39f87639ff178107f109cd18411a6077f26964b6e63f8a70b9619db04306a323c1a1d23af867e19f14f570ffe573d0e3a0c2b30632aaec3173380994be1e341e3a90bd2e4b615481f46db39ea83816448ec35feb1735c1f3020103a382010b30820107301d0603551d0e04160414932c3af70b627a0c7610b5a0e7427d6cfaea3f1e3081d70603551d230481cf3081cc8014932c3af70b627a0c7610b5a0e7427d6cfaea3f1ea181a8a481a53081a2310b3009060355040613024b52311430120603550408130b536f757468204b6f726561311330110603550407130a5375776f6e2043697479311c301a060355040a131353616d73756e6720436f72706f726174696f6e310c300a060355040b1303444d43311530130603550403130c53616d73756e6720436572743125302306092a864886f70d0109011616616e64726f69642e6f734073616d73756e672e636f6d820900d20995a79c0daad6300c0603551d13040530030101ff300d06092a864886f70d01010505000382010100329601fe40e036a4a86cc5d49dd8c1b5415998e72637538b0d430369ac51530f63aace8c019a1a66616a2f1bb2c5fabd6f313261f380e3471623f053d9e3c53f5fd6d1965d7b000e4dc244c1b27e2fe9a323ff077f52c4675e86247aa801187137e30c9bbf01c567a4299db4bf0b25b7d7107a7b81ee102f72ff47950164e26752e114c42f8b9d2a42e7308897ec640ea1924ed13abbe9d120912b62f4926493a86db94c0b46f44c6161d58c2f648164890c512dfb28d42c855bf470dbee2dab6960cad04e81f71525ded46cdd0f359f99c460db9f007d96ce83b4b218ac2d82c48f12608d469733f05a3375594669ccbf8a495544d6c5701e9369c08c810158");
    private final Signature samsungReleaseKey =
        new Signature("308204d4308203bca003020102020900e5eff0a8f66d92b3300d06092a864886f70d01010505003081a2310b3009060355040613024b52311430120603550408130b536f757468204b6f726561311330110603550407130a5375776f6e2043697479311c301a060355040a131353616d73756e6720436f72706f726174696f6e310c300a060355040b1303444d43311530130603550403130c53616d73756e6720436572743125302306092a864886f70d0109011616616e64726f69642e6f734073616d73756e672e636f6d301e170d3131303632323132323531335a170d3338313130373132323531335a3081a2310b3009060355040613024b52311430120603550408130b536f757468204b6f726561311330110603550407130a5375776f6e2043697479311c301a060355040a131353616d73756e6720436f72706f726174696f6e310c300a060355040b1303444d43311530130603550403130c53616d73756e6720436572743125302306092a864886f70d0109011616616e64726f69642e6f734073616d73756e672e636f6d30820120300d06092a864886f70d01010105000382010d00308201080282010100e9f1edb42423201dce62e68f2159ed8ea766b43a43d348754841b72e9678ce6b03d06d31532d88f2ef2d5ba39a028de0857983cd321f5b7786c2d3699df4c0b40c8d856f147c5dc54b9d1d671d1a51b5c5364da36fc5b0fe825afb513ec7a2db862c48a6046c43c3b71a1e275155f6c30aed2a68326ac327f60160d427cf55b617230907a84edbff21cc256c628a16f15d55d49138cdf2606504e1591196ed0bdc25b7cc4f67b33fb29ec4dbb13dbe6f3467a0871a49e620067755e6f095c3bd84f8b7d1e66a8c6d1e5150f7fa9d95475dc7061a321aaf9c686b09be23ccc59b35011c6823ffd5874d8fa2a1e5d276ee5aa381187e26112c7d5562703b36210b020103a382010b30820107301d0603551d0e041604145b115b23db35655f9f77f78756961006eebe3a9e3081d70603551d230481cf3081cc80145b115b23db35655f9f77f78756961006eebe3a9ea181a8a481a53081a2310b3009060355040613024b52311430120603550408130b536f757468204b6f726561311330110603550407130a5375776f6e2043697479311c301a060355040a131353616d73756e6720436f72706f726174696f6e310c300a060355040b1303444d43311530130603550403130c53616d73756e6720436572743125302306092a864886f70d0109011616616e64726f69642e6f734073616d73756e672e636f6d820900e5eff0a8f66d92b3300c0603551d13040530030101ff300d06092a864886f70d0101050500038201010039c91877eb09c2c84445443673c77a1219c5c02e6552fa2fbad0d736bc5ab6ebaf0375e520fe9799403ecb71659b23afda1475a34ef4b2e1ffcba8d7ff385c21cb6482540bce3837e6234fd4f7dd576d7fcfe9cfa925509f772c494e1569fe44e6fcd4122e483c2caa2c639566dbcfe85ed7818d5431e73154ad453289fb56b607643919cf534fbeefbdc2009c7fcb5f9b1fa97490462363fa4bedc5e0b9d157e448e6d0e7cfa31f1a2faa9378d03c8d1163d3803bc69bf24ec77ce7d559abcaf8d345494abf0e3276f0ebd2aa08e4f4f6f5aaea4bc523d8cc8e2c9200ba551dd3d4e15d5921303ca9333f42f992ddb70c2958e776c12d7e3b7bd74222eb5c7a");

    /**
     * @hide
     */
    protected static final String sTag = "SemWebView";

    private LweWebView mLWEWebView = null;
    private WebView mAndroidWebView = null;

    private boolean canUseLWE() {
        if (USE_LWE && mLWEWebView != null) {
            return true;
        } else if (mAndroidWebView == null) {
            throw new AssertionError("Both LWE and WebView are not available.");
        }
        return false;
    }

    private boolean checkLWEInstallation() {
        /*
        try{
            getContext().getPackageManager().getPackageInfo(PACKAGE_NAME, 0);
            USE_LWE = true;
        }catch (Exception e){}
        */
        return USE_LWE;
    }

    private boolean checkSignature(Context context) {
        try {
            PackageManager packageManager = context.getPackageManager();
            PackageInfo platformPackageInfo =
                packageManager.getPackageInfo("android", PackageManager.GET_SIGNATURES);
            PackageInfo lwePackageInfo =
                packageManager.getPackageInfo(PACKAGE_NAME, PackageManager.GET_SIGNATURES);

            if (platformPackageInfo == null || lwePackageInfo == null) {
                Log.e(sTag, "PackageInfo not found");
                return false;
            }
            if (platformPackageInfo.signatures == null || lwePackageInfo.signatures == null) {
                Log.e(sTag, "PackageInfo.signatures not found");
                return false;
            }

            boolean validationSucceed = false;
            if (platformPackageInfo.signatures[0].equals(samsungPlatformKey)) {
                if (lwePackageInfo.signatures[0].equals(samsungReleaseKey)) {
                    Log.d(sTag, "Signature validation succeed: samsungReleaseKey");
                    validationSucceed = true;
                }
            } else if (platformPackageInfo.signatures[0].equals(googlePlatformKey)) {
                Log.d(sTag, "Skip signature validation: googleTestKey");
                validationSucceed = true;
            } else {
                Log.e(sTag, "Unknown key used");
            }

            if (validationSucceed) {
                return true;
            }
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(sTag, "apk not found in the PackageManager");
            e.printStackTrace();
        }

        Log.e(sTag, "Signature validation failed");
        return false;
    }

    private LweWebView getLWEWebViewInstance(Context context, AttributeSet attrs, int defStyle) {
        if (mLWEWebView != null) {
            return mLWEWebView;
        }

        if (isPlatformCode) {
            Log.d(sTag, "Running platform code...");
            try {
                if (pcl == null) {
                    if (!checkSignature(getContext())) {
                        Log.e(sTag, "apk signature failed");
                        return null;
                    }

                    PackageManager packageManager = getContext().getPackageManager();
                    String path = packageManager.getPackageInfo(PACKAGE_NAME, 0).applicationInfo.nativeLibraryDir;
                    if(!getContext().getApplicationInfo().nativeLibraryDir.endsWith("64")&&!path.endsWith("arm")){
                        path = path.substring(0,path.length()-2);
                    }
                    String dexpath = packageManager.getPackageInfo(PACKAGE_NAME, 0).applicationInfo.publicSourceDir;
                    pcl = new PathClassLoader(dexpath, path, getContext().getClassLoader());
                }

                if (pcl == null) {
                    Log.e(sTag, "PathClassLoader failed");
                    return null;
                }

                Class<?> cls = pcl.loadClass(LweWebViewImpl);
                Constructor<?> cons = cls.getConstructor();
                LweWebView lweWebView = (LweWebView)cons.newInstance();
                Log.d(sTag, "LweWebView creation succeed: platform lib");
                return lweWebView;
            } catch (PackageManager.NameNotFoundException e) {
                Log.e(sTag, "apk is not installed");
                e.printStackTrace();
            } catch (Exception e) {
                Log.e(sTag, "apk cannot be loaded");
                e.printStackTrace();
            }

            Log.e(sTag, "LweWebView creation failed");
        } else {
            Log.d(sTag, "Running downloadable code...");
            // We use reflection to create an instance of LweWebViewImpl(), as
            // LweWebViewImpl.java is not included in the platform, and
            // without the actual implementation, a build error occurs.
            try {
                LweWebView lweWebView =
                    (LweWebView)Class.forName(LweWebViewImpl).getConstructor(String.class).newInstance();
                Log.d(sTag, "LweWebView creation succeed: apk lib");
                return lweWebView;
            } catch (Exception e) {
                Log.e(sTag, "cannot create LweWebViewImpl");
            }
        }

        return null;
    }

    /**
     * Creates a new InputConnection for an InputMethod to interact with the WebView.
     *
     * @param outAttrs Fill in with attribute information about the connection.
     * @return InputConnection
     * @since Lightweight Web Engine 1.0
     */
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (mLWEWebView != null) {
            return mLWEWebView.getInputConnectionInstance(this);
        }
        return null;
    }

    /**
     * Constructs a new SemWebView with an Activity Context object.
     *
     * @param context An Activity Context to access application assets
     * @since Lightweight Web Engine 1.0
     */
    public SemWebView(Context context) {
        this(context, null);
    }

    /**
     * Constructs a new SemWebView with layout parameters.
     *
     * @param context An Activity Context to access application assets
     * @param attrs An AttributeSet passed to our parent
     * @since Lightweight Web Engine 1.0
     */
    public SemWebView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    /**
     * Constructs a new SemWebView with layout parameters and a default style.
     *
     * @param context An Activity Context to access application assets
     * @param attrs An AttributeSet passed to our parent
     * @param defStyle An attribute in the current theme that contains a reference to a style
     *                 resource that supplies default values for the view. Can be 0 to not look
     *                 for defaults.
     * @since Lightweight Web Engine 1.0
     */
    public SemWebView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        if (checkLWEInstallation()) {
            mLWEWebView = getLWEWebViewInstance(context, attrs, defStyle);
            if (mLWEWebView != null) {
                mLWEWebView.initWebView(this);
            }
        } else {
            mAndroidWebView = new WebView(context, attrs, defStyle);
            mAndroidWebView.getSettings().setJavaScriptEnabled(true);
            mAndroidWebView.setWebViewClient(new WebViewClient() {
                                             @Override
                                             public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                                                 return false;
                                             }});
        }
    }

    /**
     * Loads the given URL.
     *
     * @param url The URL of the resource to load
     * @since Lightweight Web Engine 1.0
     */
    public void loadUrl(String url) {
        if (canUseLWE()) {
            mLWEWebView.loadUrl(url);
        } else {
            mAndroidWebView.loadUrl(url);
        }
    }

    /**
     * Gets the URL for the current page.
     *
     * @return The URL for the current page
     * @since Lightweight Web Engine 1.0
     */
    public String getUrl() {
        if (canUseLWE()) {
            return mLWEWebView.getUrl();
        } else {
            return mAndroidWebView.getUrl();
        }
    }

    /**
     * Loads the given data into this WebView using a 'data' scheme URL.
     *
     * @param data A String of data in the given encoding
     * @param mimeType The MIME type of the data, e.g., 'text/html'.
     *                 This value may be null.
     * @param encoding The encoding of the data
     *                 This value may be null.
     * @since Lightweight Web Engine 1.0
     */
    public void loadData(String data, String mimeType, String encoding) {

        if (canUseLWE()) {
            if (mimeType == null) {
                mimeType = "text/html";
            }
            if (encoding == null) {
                encoding = "UTF-8";
            }
            mLWEWebView.loadData(data, mimeType, encoding);
        } else {
            mAndroidWebView.loadData(data, mimeType, encoding);
        }
    }

    /**
     * Reloads the current URL.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void reload() {
        if (canUseLWE()) {
            mLWEWebView.reload();
        } else {
            mAndroidWebView.reload();
        }
    }

    /**
     * Stops the current load.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void stopLoading() {
        if (canUseLWE()) {
            mLWEWebView.stopLoading();
        } else {
            mAndroidWebView.stopLoading();
        }
    }

    /**
     * Goes back in the history of this WebView.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void goBack() {
        if (canUseLWE()) {
            mLWEWebView.goBack();
        } else {
            mAndroidWebView.goBack();
        }
    }

    /**
     * Goes forward in the history of this WebView.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void goForward() {
        if (canUseLWE()) {
            mLWEWebView.goForward();
        } else {
            mAndroidWebView.goForward();
        }
    }

    /**
     * Gets whether this WebView has a back history item.
     *
     * @return {@code true} if this WebView has a back history item, <br>
     *         {@code false} otherwise
     * @since Lightweight Web Engine 1.0
     */
    public boolean canGoBack() {
        if (canUseLWE()) {
            return mLWEWebView.canGoBack();
        } else {
            return mAndroidWebView.canGoBack();
        }
    }

    /**
     * Gets whether this WebView has a forward history item.
     *
     * @return {@code true} if this WebView has a forward history item,
     *         {@code false} otherwise
     * @since Lightweight Web Engine 1.0
     */
    public boolean canGoForward() {
        if (canUseLWE()) {
            return mLWEWebView.canGoForward();
        } else {
            return mAndroidWebView.canGoForward();
        }
    }

    /**
     * Injects the supplied Java object into this WebView.
     *
     * @param object The Java object to inject into this WebView's JavaScript context.
     *               null values are ignored.
     * @param name The name used to expose the object in JavaScript
     * @since Lightweight Web Engine 1.0
     */
    @SuppressLint("JavascriptInterface")
    public void addJavascriptInterface(Object object, String name) {
        if (canUseLWE()) {
            mLWEWebView.addJavascriptInterface(object, name);
        } else {
            mAndroidWebView.addJavascriptInterface(object, name);
        }
    }

    /**
     * Removes a previously injected Java object from this WebView.
     *
     * @param name The name used to expose the object in JavaScript. This value must never be null.
     * @since Lightweight Web Engine 1.0
     */
    public void removeJavascriptInterface(String name) {
        if (canUseLWE()) {
            mLWEWebView.removeJavascriptInterface(name);
        } else {
            mAndroidWebView.removeJavascriptInterface(name);
        }
    }

    /**
     * Clears the resource cache.
     *
     * @param includeDiskFiles if {@code false}, only the RAM cache is cleared.
     * @since Lightweight Web Engine 1.0
     */
    public void clearCache(boolean includeDiskFiles) {
        if (canUseLWE()) {
            mLWEWebView.clearCache(includeDiskFiles);
        } else {
            mAndroidWebView.clearCache(includeDiskFiles);
        }
    }

    /**
     * Asynchronously evaluates JavaScript in the context of the currently displayed page.
     *
     * @param script The JavaScript to execute.
     * @param resultCallback A callback to be invoked when the script execution completes with the
     *                       result of the execution (if any). May be null if no notification of
     *                       the result is required.
     * @since Lightweight Web Engine 1.0
     */
    public void evaluateJavascript(String script, ValueCallback<String> resultCallback) {
        if (canUseLWE()) {
            mLWEWebView.evaluateJavascript(script, resultCallback);
        } else {
            mAndroidWebView.evaluateJavascript(script, resultCallback);
        }
    }

    /**
     * Tells this WebView to clear its internal back/forward list.
     *
     * @since Lightweight Web Engine 1.0
     */
    public void clearHistory() {
        if (canUseLWE()) {
            mLWEWebView.clearHistory();
        } else {
            mAndroidWebView.clearHistory();
        }
    }

    /**
     * Gets the Settings object used to control the settings for this WebView.
     *
     * @return A Settings object that can be used to control this WebView's settings
     * @since Lightweight Web Engine 1.0
     */
    public SemWebSettings getSettings() {
        if (canUseLWE()) {
            return mLWEWebView.getSettings();
        } else {
            return new SemWebSettings(mAndroidWebView);
        }
    }

    /**
     * Sets the WebViewClient that will receive various notifications and requests.
     * This will replace the current handler.
     *
     * @param client An implementation of SemWebViewClient
     * @since Lightweight Web Engine 1.0
     */
    public void setWebViewClient(SemWebViewClient client) {

        if (canUseLWE()) {
            mLWEWebView.setWebViewClient(client);
        } else {

            class WebViewClientWrapper extends WebViewClient {
                private SemWebView mSemWebview;
                private SemWebViewClient mSemWebViewClient;

                WebViewClientWrapper(SemWebView webview, SemWebViewClient client){
                    mSemWebview = webview;
                    mSemWebViewClient = client;
                }

                @Override
                public void onPageStarted(WebView view, String url, Bitmap favicon) {
                    mSemWebViewClient.onPageStarted(mSemWebview, url, favicon);
                }

                @Override
                public void onLoadResource(WebView view, String url) {
                    mSemWebViewClient.onLoadResource(mSemWebview, url);
                }

                @Override
                public void onPageFinished(WebView view, String url) {
                    mSemWebViewClient.onPageFinished(mSemWebview, url);
                }

                @Override
                public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
                    class MyWebResourceRequestImpl implements SemWebResourceRequest {
                        String mUrl;

                        public MyWebResourceRequestImpl(String url) {
                            mUrl = url;
                        }

                        public Uri getUrl() {
                            return Uri.parse(mUrl);
                        }
                    }

                    mSemWebViewClient.onReceivedError(
                        mSemWebview, new MyWebResourceRequestImpl(request.getUrl().toString()),
                        new SemWebResourceError(error.getErrorCode(), error.getDescription()));
                }

                @Override
                public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
                    return false;
                }
            }
            mAndroidWebView.setWebViewClient(new WebViewClientWrapper(this, client));
        }
    }

    /**
     * Sets the lwe handler.
     *
     * @param client An implementation of SemWebLweClient
     * @since Lightweight Web Engine 1.0
     */
    public void setWebLweClient(SemWebLweClient client) {
        if (canUseLWE()) {
            mLWEWebView.setWebLweClient(client);
        } else {
            class WebChromeClientWrapper extends WebChromeClient {
                private SemWebView mSemWebview;
                private SemWebLweClient mSemWebLweClient;
                WebChromeClientWrapper(SemWebView webview, SemWebLweClient client){
                    mSemWebview = webview;
                    mSemWebLweClient = client;
                }
                @Override
                public void onProgressChanged(WebView view, int newProgress) {
                    mSemWebLweClient.onProgressChanged(mSemWebview, newProgress);
                }
            }
            mAndroidWebView.setWebChromeClient(new WebChromeClientWrapper(this, client));
        }
    }

    /**
     * Registers the interface to be used when content can not be handled by the rendering engine,
     * and should be downloaded instead. This will replace the current handler.
     *
     * @param listener An implementation of SemDownloadListener
     * @since Lightweight Web Engine 1.0
     */
    public void setDownloadListener(SemDownloadListener listener) {
        if (canUseLWE()) {
            mLWEWebView.setDownloadListener(listener);
        } else {
            class DownloadListenerWrapper implements DownloadListener {
                private SemDownloadListener mSemDownloadListener;
                DownloadListenerWrapper(SemDownloadListener client){
                    mSemDownloadListener = client;
                }
                public void onDownloadStart(String url, String userAgent, String contentDisposition, String mimetype,
                                     long contentLength){
                    mSemDownloadListener.onDownloadStart(url, userAgent, contentDisposition, mimetype, contentLength);
                }
            }
            mAndroidWebView.setDownloadListener(new DownloadListenerWrapper(listener));
        }
    }
}
