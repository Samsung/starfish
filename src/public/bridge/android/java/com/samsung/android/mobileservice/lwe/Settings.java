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

public class Settings {
    public static final int LOAD_DEFAULT = -1;
//    static final int LOAD_NORMAL = 0;
    public static final int LOAD_CACHE_ELSE_NETWORK = 1;
    public static final int LOAD_NO_CACHE = 2;
    public static final int LOAD_CACHE_ONLY = 3;


    private int mCacheMode = LOAD_DEFAULT;
    private String mDefaultUserAgent = null;
    private String mUserAgentString = null;

    Settings(String DUA, String UA, int cacheMode){
        mDefaultUserAgent = DUA;
        mUserAgentString = UA;
        setCacheMode(cacheMode);
    }

    public String getDefaultUserAgent(){
        return mDefaultUserAgent;
    }

    public String getUserAgentString(){
        return mUserAgentString;
    }

    public int getCacheMode(){
        return mCacheMode;
    }

    public void setUserAgentString(String ua){
        mUserAgentString = ua;
    }

    public void setCacheMode(int mode){
        if(mode==LOAD_DEFAULT || mode==LOAD_NO_CACHE){
            mCacheMode  = mode;
        }
    }

    /*
    void setAllowUniveralAceessFromFilesURLs(boolean allowUniveralAceessFromFilesURLs){
    }
    void setJavaScriptEnable(boolean javaScriptEnable){
    }
    */

}
