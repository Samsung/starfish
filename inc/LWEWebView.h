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


#ifndef __LWEWebView__
#define __LWEWebView__

#include "StarFishExport.h"
#include <string>

namespace LWE
{

class WebView;
class Settings 
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

class ResourceError 
{
public:
    ResourceError(int code,std::string description);
	int GetErrorCode();
	std::string GetDescription();
private:
	int m_errorCode;
	std::string m_description;
};

class WebViewClient
{
public:
    virtual std::string OnLoadResource(WebView* view,std::string url){
		return std::string();
	}
    virtual void OnReceivedError(WebView* view, ResourceError error){}
	virtual void OnPageFinished(WebView* view, std::string url){}
	virtual void OnPageStarted(WebView* view, std::string url){}
};


class WebView {
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
	void AddJavaScriptInterface(std::string exposedObjectName, std::string jsFunctionName, std::string (*cb)(std::string));
	std::string EvaluateJavaScript(std::string script);
	void ClearHistory();
	void Destroy();
	void SetSettings(Settings setttings);
	void RemoveJavascriptInterface(std::string exposedObjectName, std::string jsFunctionName);
	void SetWebViewClient(WebViewClient* client);
protected:
	WebView(void* starFish);

private:
	void* m_starfish;
	WebViewClient* m_webViewClient;
};

}
#endif
