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


#ifndef __LWEWebView__
#define __LWEWebView__

#include "StarFishExport.h"
#include <string>

namespace LWE
{

class WebView;
class STARFISH_EXPORT Settings 
{
public:
	Settings(std::string default_ua,std::string ua);
	std::string GetDefaultUserAgent();
	std::string GetUserAgentString();
	void SetUserAgentString(std::string ua);
private:
	std::string m_defaultUserAgent;
	std::string m_UserAgent;
};

class STARFISH_EXPORT ResourceError 
{
public:
    ResourceError(int code,std::string description);
	int GetErrorCode();
	std::string GetDescription();
private:
	int m_errorCode;
	std::string m_description;
};

class STARFISH_EXPORT WebViewClient
{
public:
	virtual void OnReceivedError(WebView* view, ResourceError error){}
	virtual void OnPageFinished(WebView* view, std::string url){}
	virtual void OnPageStarted(WebView* view, std::string url){}
	virtual void OnLoadResource(WebView* view,std::string url){}
};


class STARFISH_EXPORT WebView {
public:
	static WebView* Create();
	static WebView* Create(void* starFish);

	Settings GetSettings();
	void LoadURL(std::string url);
	std::string GetURL();
	void LoadData(std::string data);
	void Reload();
	void StopLoading();
	void GoBack();
	void GoForward();
	bool CanGoBack();
	bool CanGoForward();
	void AddJavaScriptInterface(std::string exposedObjectName, std::string jsFunctionName, std::function<std::string(std::string)> cb);
	std::string EvaluateJavaScript(std::string script);
	void ClearHistory();
	void Destroy();
	void SetSettings(Settings setttings);
	void RemoveJavascriptInterface(std::string exposedObjectName, std::string jsFunctionName);
	void SetWebViewClient(WebViewClient* client);

	// Internal API
	void* getInternalPtr();

protected:
	WebView(void* starFish);

private:
	void* m_starfish;
	WebViewClient* m_webViewClient;
};
}

#ifdef PORT_WINDOW_BACKEND_ANDROID
	void requestRender();
#endif

#endif
