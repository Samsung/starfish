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

#ifndef __DaliStarFishBinder__
#define __DaliStarFishBinder__

#include "LWEWebView.h"

struct DaliStarFishBinder {
    void* lweInstance;
    std::list<size_t> asyncHandlePool;
    std::string url;
    uint8_t* outputBuffer;
    size_t outputWidth;
    size_t outputHeight;
    size_t outputStride;
    bool canGoBack, canGoForward;
    bool isRunning;
    std::function<void(LWE::WebContainer*,
                       const LWE::WebContainer::RenderResult&)>
        onRenderedHandler;
    std::function<void(LWE::WebContainer*, LWE::ResourceError)> onReceivedError;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onPageFinishedHandler;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onPageStartedHandler;
    std::function<void(LWE::WebContainer*, const std::string&)>
        onLoadResourceHandler;
    DaliStarFishBinder()
        : lweInstance(nullptr)
        , outputBuffer(nullptr)
        , outputWidth(0)
        , outputHeight(0)
        , outputStride(0)
        , canGoBack(false)
        , canGoForward(false)
        , isRunning(false)
    {
    }
};

LWE::KeyValue eventKeyToKeyboardData(const char* DALIKeyString,
                                     bool isShiftPressed);

extern "C" void startMainThreadIfNeeds(pthread_t& t);
extern "C" void createInstance(DaliStarFishBinder* binder);
extern "C" void loadURL(DaliStarFishBinder* binder, const std::string& url);
extern "C" void setSize(DaliStarFishBinder* binder);
extern "C" void loadData(DaliStarFishBinder* binder, const std::string& d);
extern "C" void reload(DaliStarFishBinder* binder);
extern "C" void stopLoading(DaliStarFishBinder* binder);
extern "C" void goBack(DaliStarFishBinder* binder);
extern "C" void goForward(DaliStarFishBinder* binder);
extern "C" void addJavaScriptInterface(
    DaliStarFishBinder* binder, const std::string& exposedObjectName,
    const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb);
extern "C" void evaluateJavaScript(DaliStarFishBinder* binder,
                                   const std::string& script);
extern "C" void clearHistory(DaliStarFishBinder* binder);
extern "C" void destroy(DaliStarFishBinder* binder);
extern "C" void removeJavascriptInterface(DaliStarFishBinder* binder,
                                          const std::string& exposedObjectName,
                                          const std::string& jsFunctionName);
extern "C" void clearCache(DaliStarFishBinder* binder);
extern "C" void stopLoop(DaliStarFishBinder* binder);

extern "C" void registerOnRenderedHandler(
    DaliStarFishBinder* binder,
    const std::function<
        void(LWE::WebContainer* c,
             const LWE::WebContainer::RenderResult& renderResult)>& callback);
extern "C" void registerOnPageStartedHandler(
    DaliStarFishBinder* binder,
    const std::function<void(LWE::WebContainer*, const std::string&)>&
        callback);
extern "C" void registerOnReceivedErrorHandler(
    DaliStarFishBinder* binder,
    const std::function<void(LWE::WebContainer*, LWE::ResourceError)>&
        callback);
extern "C" void registerOnPageFinishedHandler(
    DaliStarFishBinder* binder,
    const std::function<void(LWE::WebContainer*, const std::string&)>&
        callback);

extern "C" void dispatchMouseDownEvent(DaliStarFishBinder* binder, float x,
                                       float y);
extern "C" void dispatchMouseUpEvent(DaliStarFishBinder* binder, float x,
                                     float y);
extern "C" void dispatchMouseMoveEvent(DaliStarFishBinder* binder, float x,
                                       float y, bool isLButtonPressed,
                                       bool isRButtonPressed);
extern "C" void dispatchKeyDownEvent(DaliStarFishBinder* binder,
                                     LWE::KeyValue keyCode);
extern "C" void dispatchKeyPressEvent(DaliStarFishBinder* binder,
                                      LWE::KeyValue keyCode);
extern "C" void dispatchKeyUpEvent(DaliStarFishBinder* binder,
                                   LWE::KeyValue keyCode);

#endif
