/*

    Image Uploader -  free application for uploading images/files to the Internet

    Copyright 2007-2015 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/

#include "WebBrowser.h"
//using namespace ScriptAPI;


#ifdef _WIN32
#include "WebBrowserPrivate_win.h"
#else
// Not implemented
#endif


using namespace ScriptAPI;
DECLARE_INSTANCE_TYPE(CWebBrowser);
DECLARE_INSTANCE_TYPE(HtmlDocument);

namespace ScriptAPI {


CWebBrowser::CWebBrowser()
{
	d_ = new WebBrowserPrivate(this);
}

CWebBrowser::~CWebBrowser()
{
	delete d_;
}

bool CWebBrowser::navigateToUrl(const std::string& url)
{
	return d_->navigateToUrl(url);
}

void CWebBrowser::setOnUrlChangedCallback(SquirrelObject callBack, SquirrelObject context)
{
	d_->setOnUrlChangedCallback(callBack, context);
}

void CWebBrowser::setOnNavigateErrorCallback(SquirrelObject callBack, SquirrelObject context)
{
	d_->setOnNavigateErrorCallback(callBack, context);
}

void CWebBrowser::setOnLoadFinishedCallback(SquirrelObject callBack, SquirrelObject context)
{
	d_->setOnLoadFinishedCallback(callBack, context);
}

void CWebBrowser::setOnTimerCallback(int timerInterval, SquirrelObject callBack, SquirrelObject context)
{
	d_->setOnTimerCallback(timerInterval,  callBack, context);
}

void CWebBrowser::setOnFileInputFilledCallback(SquirrelObject callBack, SquirrelObject context)
{
	d_->setOnFileInputFilledCallback(callBack, context);
}

const std::string CWebBrowser::getDocumentContents()
{
	return d_->getDocumentContents();
}

ScriptAPI::HtmlDocument CWebBrowser::document()
{
	return d_->document();
}	

bool CWebBrowser::setHtml(const std::string& html)
{
	return d_->setHtml(html);
}

const std::string CWebBrowser::runJavaScript(const std::string& code)
{
	return d_->runJavaScript(code);
}

const std::string CWebBrowser::callJavaScriptFunction(const std::string& funcName, SquirrelObject args)
{
	return d_->callJavaScriptFunction(funcName, args);
}

void CWebBrowser::setSilent(bool silent)
{
	d_->setSilent(silent);
}

void CWebBrowser::addTrustedSite(const std::string& domain)
{
	d_->addTrustedSite(domain);
}

int CWebBrowser::getMajorVersion()
{
	return d_->getMajorVersion();
}

bool CWebBrowser::showModal()
{
	return d_->showModal();
}

bool CWebBrowser::exec()
{
	return d_->exec();
}

void CWebBrowser::show()
{
	d_->show();
}

void CWebBrowser::hide()
{
	d_->hide();
}

void CWebBrowser::close()
{
	d_->close();
}

void CWebBrowser::setFocus()
{
	d_->setFocus();
}

void CWebBrowser::setTitle(const std::string& title)
{
	d_->setTitle(title);
}

const std::string CWebBrowser::url()
{
	return d_->url();
}

const std::string CWebBrowser::title()
{
	return d_->title();
}

void RegisterWebBrowserClass() {
	using namespace SqPlus;
	SQClassDef<CWebBrowser>("CWebBrowser")
		.func(&CWebBrowser::navigateToUrl, "navigateToUrl")
		.func(&CWebBrowser::showModal, "showModal")
		.func(&CWebBrowser::setTitle, "setTitle")
		.func(&CWebBrowser::exec, "exec")
		.func(&CWebBrowser::show, "show")
		.func(&CWebBrowser::hide, "hide")
		.func(&CWebBrowser::close, "close")
		.func(&CWebBrowser::url, "url")
		.func(&CWebBrowser::title, "title")
		.func(&CWebBrowser::getDocumentContents, "getDocumentContents")
		.func(&CWebBrowser::document, "document")
		.func(&CWebBrowser::setHtml, "setHtml")
		.func(&CWebBrowser::runJavaScript, "runJavaScript")
		.func(&CWebBrowser::setSilent, "setSilent")
		.func(&CWebBrowser::addTrustedSite, "addTrustedSite")
		.func(&CWebBrowser::getMajorVersion, "getMajorVersion")
		.func(&CWebBrowser::callJavaScriptFunction, "callJavaScriptFunction")
		.func(&CWebBrowser::setOnUrlChangedCallback, "setOnUrlChangedCallback")
		.func(&CWebBrowser::setOnLoadFinishedCallback, "setOnLoadFinishedCallback")
		.func(&CWebBrowser::setOnTimerCallback, "setOnTimerCallback")
		.func(&CWebBrowser::setOnFileInputFilledCallback, "setOnFileInputFilledCallback")
		.func(&CWebBrowser::setOnNavigateErrorCallback, "setOnNavigateErrorCallback");
}

}