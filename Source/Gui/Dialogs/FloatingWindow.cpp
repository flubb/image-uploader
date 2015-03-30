/*
    Image Uploader - program for uploading images/files to Internet
    Copyright (C) 2007-2015 ZendeN <zenden2k@gmail.com>

    HomePage:    http://zenden.ws/imageuploader

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// This file was generated by WTL subclass control wizard
// FloatingWindow.cpp : Implementation of FloatingWindow

#include "FloatingWindow.h"

#include "ResultsPanel.h"
#include "ScreenshotDlg.h"
#include "Func/Settings.h"
#include "LogWindow.h"
#include "Func/Base.h"
#include "Func/HistoryManager.h"
#include "Core/Utils/CoreTypes.h"
#include <Func/WebUtils.h>
#include <Func/WinUtils.h>
#include <Core/Upload/UrlShorteningTask.h>
#include <Func/IuCommonFunctions.h>
#include <Gui/GuiTools.h>
// FloatingWindow
CFloatingWindow::CFloatingWindow()
{
	m_bFromHotkey = false;
	m_ActiveWindow = 0;
	EnableClicks = true;
	m_FileQueueUploader = 0;
	hMutex = NULL;
	m_PrevActiveWindow = 0;
	m_bStopCapturingWindows = false;
	WM_TASKBARCREATED = RegisterWindowMessage(_T("TaskbarCreated"));
	m_bIsUploading = 0;
}

CFloatingWindow::~CFloatingWindow()
{
	CloseHandle(hMutex);
	DeleteObject(m_hIconSmall);
	m_hWnd = 0;
}

LRESULT CFloatingWindow::OnClose(void)
{
	return 0;
}

bool MyInsertMenu(HMENU hMenu, int pos, UINT id, const LPCTSTR szTitle,  HBITMAP bm = NULL)
{
	MENUITEMINFO MenuItem;

	MenuItem.cbSize = sizeof(MenuItem);
	if (szTitle)
		MenuItem.fType = MFT_STRING;
	else
		MenuItem.fType = MFT_SEPARATOR;
	MenuItem.fMask = MIIM_TYPE | MIIM_ID | MIIM_DATA;
	if (bm)
		MenuItem.fMask |= MIIM_CHECKMARKS;
	MenuItem.wID = id;
	MenuItem.hbmpChecked = bm;
	MenuItem.hbmpUnchecked = bm;
	MenuItem.dwTypeData = (LPWSTR)szTitle;
	return InsertMenuItem(hMenu, pos, TRUE, &MenuItem) != 0;
}

LRESULT CFloatingWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	/*int w = ::GetSystemMetrics(SM_CXSMICON);
	if ( w > 32 ) {
		w = 48;
	} else if ( w > 16 ) {
		w = 32;
	}
	int h = w;*/
	m_hIconSmall = GuiTools::LoadSmallIcon(IDR_MAINFRAME);
	SetIcon(m_hIconSmall, FALSE);

	RegisterHotkeys();
	InstallIcon(APPNAME, m_hIconSmall, /*TrayMenu*/ 0);
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = NOTIFYICONDATA_V2_SIZE;
	nid.hWnd = m_hWnd;
	nid.uVersion = NOTIFYICON_VERSION;
	Shell_NotifyIcon(NIM_SETVERSION, &nid);
	return 0;
}

LRESULT CFloatingWindow::OnExit(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pWizardDlg->CloseWizard();
	return 0;
}

LRESULT CFloatingWindow::OnTrayIcon(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (!EnableClicks )
		return 0;

	if (LOWORD(lParam) == WM_LBUTTONDOWN)
	{
		m_bStopCapturingWindows = true;
	}
	if (LOWORD(lParam) == WM_MOUSEMOVE)
	{
		if (!m_bStopCapturingWindows)
		{
			HWND wnd =  GetForegroundWindow();
			if (wnd != m_hWnd)
				m_PrevActiveWindow = GetForegroundWindow();
		}
	}
	if (LOWORD(lParam) == WM_RBUTTONUP)
	{
		if (m_bIsUploading && Settings.Hotkeys[Settings.TrayIconSettings.RightClickCommand].commandId != IDM_CONTEXTMENU)
			return 0;
		SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys[Settings.TrayIconSettings.RightClickCommand].commandId, 0));
	}
	else if (LOWORD(lParam) == WM_LBUTTONDBLCLK)
	{
		EnableClicks = false;
		KillTimer(1);
		SetTimer(2, GetDoubleClickTime());
		if (m_bIsUploading && Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId !=
		    IDM_CONTEXTMENU)
			return 0;
		SendMessage(WM_COMMAND,
		            MAKEWPARAM(Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId, 0));
	}
	else if (LOWORD(lParam) == WM_LBUTTONUP)
	{
		m_bStopCapturingWindows = false;
		if (m_bIsUploading && Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId !=
		    IDM_CONTEXTMENU)
			return 0;

		if (!Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId)
			SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys[Settings.TrayIconSettings.LeftClickCommand].commandId, 0));
		else
			SetTimer(1, (UINT) (1.2 * GetDoubleClickTime()));
	}
	else if (LOWORD(lParam) == WM_MBUTTONUP)
	{
		if (m_bIsUploading && Settings.Hotkeys[Settings.TrayIconSettings.MiddleClickCommand].commandId != IDM_CONTEXTMENU)
			return 0;

		SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys[Settings.TrayIconSettings.MiddleClickCommand].commandId, 0));
	}
	else if (LOWORD(lParam) == NIN_BALLOONUSERCLICK)
	{
		CAtlArray<CUrlListItem> items;
		CUrlListItem it;
		it.ImageUrl = Utf8ToWstring(lastUploadedItem_.fileListItem.imageUrl).c_str();
		it.ImageUrlShortened = lastUploadedItem_.imageUrlShortened;
		it.ThumbUrl =  Utf8ToWstring(lastUploadedItem_.fileListItem.thumbUrl).c_str();
		it.DownloadUrl = Utf8ToWstring(lastUploadedItem_.fileListItem.downloadUrl).c_str();
		it.DownloadUrlShortened = lastUploadedItem_.downloadUrlShortened;
		items.Add(it);
		if (it.ImageUrl.IsEmpty() && it.DownloadUrl.IsEmpty())
			return 0;
		CResultsWindow rp( pWizardDlg, items, false);
		rp.DoModal(m_hWnd);
	}
	return 0;
}

LRESULT CFloatingWindow::OnMenuSettings(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (!pWizardDlg->IsWindowEnabled())
	{
		HWND childModalDialog = pWizardDlg->GetLastActivePopup();
		if (childModalDialog && ::IsWindowVisible(childModalDialog))
			SetForegroundWindow(childModalDialog);
		return 0;
	}
	HWND hParent  = pWizardDlg->m_hWnd; // pWizardDlg->IsWindowEnabled()?  : 0;
	CSettingsDlg dlg(CSettingsDlg::spTrayIcon);
	dlg.DoModal(hParent);
	return 0;
}

LRESULT CFloatingWindow::OnCloseTray(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ShowWindow(SW_HIDE);
	pWizardDlg->ShowWindow(SW_SHOW);
	pWizardDlg->SetDlgItemText(IDCANCEL, TR("�����"));
	CloseHandle(hMutex);
	RemoveIcon();
	UnRegisterHotkeys();
	DestroyWindow();
	hMutex = NULL;
	m_hWnd = 0;
	return 0;
}

LRESULT CFloatingWindow::OnReloadSettings(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!lParam)
		UnRegisterHotkeys();

	if (!wParam)
		Settings.LoadSettings();

	if (!lParam)
		RegisterHotkeys();
	return 0;
}

LRESULT CFloatingWindow::OnImportvideo(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (pWizardDlg->executeFunc(_T("importvideo,1")))
		pWizardDlg->ShowWindow(SW_SHOW);
	return 0;
}

LRESULT CFloatingWindow::OnUploadFiles(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (pWizardDlg->executeFunc(_T("addfiles,1")))
		pWizardDlg->ShowWindow(SW_SHOW);
	return 0;
}

LRESULT CFloatingWindow::OnReUploadImages(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (pWizardDlg->executeFunc(_T("reuploadimages,1"))) {
		pWizardDlg->ShowWindow(SW_SHOW);
	}
	return 0;
}


LRESULT CFloatingWindow::OnUploadImages(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (pWizardDlg->executeFunc(_T("addimages,1")))
		pWizardDlg->ShowWindow(SW_SHOW);
	return 0;
}

LRESULT CFloatingWindow::OnPasteFromWeb(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (pWizardDlg->executeFunc(_T("downloadimages,1")))
		pWizardDlg->ShowWindow(SW_SHOW);
	return 0;
}

LRESULT CFloatingWindow::OnScreenshotDlg(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pWizardDlg->executeFunc(_T("screenshotdlg,2"));
	return 0;
}

LRESULT CFloatingWindow::OnRegionScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pWizardDlg->executeFunc(_T("regionscreenshot_dontshow,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
	return 0;
}

LRESULT CFloatingWindow::OnFullScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pWizardDlg->executeFunc(_T("fullscreenshot,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
	return 0;
}

LRESULT CFloatingWindow::OnWindowHandleScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pWizardDlg->executeFunc(_T("windowhandlescreenshot,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
	return 0;
}

LRESULT CFloatingWindow::OnFreeformScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pWizardDlg->executeFunc(_T("freeformscreenshot,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));
	return 0;
}

LRESULT CFloatingWindow::OnShortenUrlClipboard(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
	if (  m_FileQueueUploader && m_FileQueueUploader->IsRunning() ) {
		return false;
	}
	delete m_FileQueueUploader;
	m_FileQueueUploader =  new CFileQueueUploader;
	m_FileQueueUploader->setCallback(this);

 	CString url;
	WinUtils::GetClipboardText(url);
	if ( !url.IsEmpty() && !WebUtils::DoesTextLookLikeUrl(url) ) {
		return false;
	}

	std_tr::shared_ptr<UrlShorteningTask> task(new UrlShorteningTask(WCstringToUtf8(url)));

	CUploadEngineData *ue = Settings.urlShorteningServer.uploadEngineData();
	CAbstractUploadEngine * e = _EngineList->getUploadEngine(ue, Settings.urlShorteningServer.serverSettings());
	if ( !e ) {
		return false;
	}
	e->setUploadData(ue);
	ServerSettingsStruct& settings = Settings.urlShorteningServer.serverSettings();
	e->setServerSettings(settings);

	
	e->setUploadData(ue);
	uploadType_ = utUrl;
	m_FileQueueUploader->AddUploadTask(task, 0, e);
	m_FileQueueUploader->start();

	CString msg;
	msg.Format(TR("C������� ������ \"%s\" � ������� %s"), (LPCTSTR)url,
		(LPCTSTR)Utf8ToWstring(ue->Name).c_str());
	ShowBaloonTip(msg, _T("Image Uploader"));
	return 0;
}

LRESULT CFloatingWindow::OnWindowScreenshot(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (m_PrevActiveWindow)
		SetForegroundWindow(m_PrevActiveWindow);
	pWizardDlg->executeFunc(_T("windowscreenshot_delayed,") + (m_bFromHotkey ? CString(_T("1")) : CString(_T("2"))));

	return 0;
}

LRESULT CFloatingWindow::OnAddFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (pWizardDlg->executeFunc(_T("addfolder")))
		pWizardDlg->ShowWindow(SW_SHOW);
	return 0;
}

LRESULT CFloatingWindow::OnShortenUrl(WORD wNotifyCode, WORD wID, HWND hWndCtl) {
	if (pWizardDlg->executeFunc(_T("shortenurl")))
		pWizardDlg->ShowWindow(SW_SHOW);
	return 0;
}


LRESULT CFloatingWindow::OnShowAppWindow(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (pWizardDlg->IsWindowEnabled())
	{
		pWizardDlg->ShowWindow(SW_SHOWNORMAL);
		if (pWizardDlg->IsWindowVisible())
		{
			// SetForegroundWindow(m_hWnd);
			pWizardDlg->SetActiveWindow();
			SetForegroundWindow(pWizardDlg->m_hWnd);
		}
	}
	else
	{
		// Activating some child modal dialog if exists one
		HWND childModalDialog = pWizardDlg->GetLastActivePopup();
		if (childModalDialog && ::IsWindowVisible(childModalDialog))
			SetForegroundWindow(childModalDialog);
	}

	return 0;
}

LRESULT CFloatingWindow::OnContextMenu(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (!IsWindowEnabled())
		return 0;

	CMenu TrayMenu;
	TrayMenu.CreatePopupMenu();

	if (!m_bIsUploading)
	{
		// Inserting menu items
		int i = 0;
		MyInsertMenu(TrayMenu, i++, IDM_UPLOADFILES, TR("��������� �����") + CString(_T("...")));
		MyInsertMenu(TrayMenu, i++, IDM_ADDFOLDERS, TR("��������� �����") + CString(_T("...")));
		MyInsertMenu(TrayMenu, i++, 0, 0);
		bool IsClipboard = false;

		if (OpenClipboard())
		{
			IsClipboard = IsClipboardFormatAvailable(CF_BITMAP) != 0;
			CloseClipboard();
		}
		if (IsClipboard)
		{
			MyInsertMenu(TrayMenu, i++, IDM_PASTEFROMCLIPBOARD, TR("�������� �� ������"));
			MyInsertMenu(TrayMenu, i++, 0, 0);
		}
		MyInsertMenu(TrayMenu, i++, IDM_IMPORTVIDEO, TR("������ �����"));
		MyInsertMenu(TrayMenu, i++, 0, 0);
		MyInsertMenu(TrayMenu, i++, IDM_SCREENSHOTDLG, TR("��������") + CString(_T("...")));
		MyInsertMenu(TrayMenu, i++, IDM_REGIONSCREENSHOT, TR("������ ���������� �������"));
		MyInsertMenu(TrayMenu, i++, IDM_FULLSCREENSHOT, TR("������ ����� ������"));
		MyInsertMenu(TrayMenu, i++, IDM_WINDOWSCREENSHOT, TR("������ ��������� ����"));
		MyInsertMenu(TrayMenu, i++, IDM_WINDOWHANDLESCREENSHOT, TR("������ ���������� ��������"));
		MyInsertMenu(TrayMenu, i++, IDM_FREEFORMSCREENSHOT, TR("������ ������������ �����"));

		CMenu SubMenu;
		SubMenu.CreatePopupMenu();
		SubMenu.InsertMenu(
			0, MFT_STRING | MFT_RADIOCHECK |
			(Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_OPENINEDITOR ? MFS_CHECKED : 0),
			IDM_SCREENTSHOTACTION_OPENINEDITOR, TR("������� � ���������"));
		SubMenu.InsertMenu(
		   0, MFT_STRING | MFT_RADIOCHECK |
		   (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_UPLOAD ? MFS_CHECKED : 0),
		   IDM_SCREENTSHOTACTION_UPLOAD, TR("��������� �� ������"));
		SubMenu.InsertMenu(
		   1, MFT_STRING | MFT_RADIOCHECK |
		   (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_CLIPBOARD ? MFS_CHECKED : 0),
		   IDM_SCREENTSHOTACTION_TOCLIPBOARD, TR("���������� � ����� ������"));
		SubMenu.InsertMenu(
		   2, MFT_STRING | MFT_RADIOCHECK |
		   (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_SHOWWIZARD ? MFS_CHECKED : 0),
		   IDM_SCREENTSHOTACTION_SHOWWIZARD, TR("������� � ���� �������"));
		SubMenu.InsertMenu(
		   3, MFT_STRING | MFT_RADIOCHECK |
		   (Settings.TrayIconSettings.TrayScreenshotAction == TRAY_SCREENSHOT_ADDTOWIZARD ? MFS_CHECKED : 0),
		   IDM_SCREENTSHOTACTION_ADDTOWIZARD, TR("�������� � ������"));

		MENUITEMINFO mi;
		mi.cbSize = sizeof(mi);
		mi.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU;
		mi.fType = MFT_STRING;
		mi.hSubMenu = SubMenu;
		mi.wID = 10000;
		mi.dwTypeData  = TR("�������� �� �������");
		TrayMenu.InsertMenuItem(i++, true, &mi);

		SubMenu.Detach();
		MyInsertMenu(TrayMenu, i++, 0, 0);
		MyInsertMenu(TrayMenu, i++, IDM_SHORTENURL, TR("��������� ������"));
		MyInsertMenu(TrayMenu, i++, 0, 0);
		MyInsertMenu(TrayMenu, i++, IDM_SHOWAPPWINDOW, TR("�������� ���� ���������"));
		MyInsertMenu(TrayMenu, i++, 0, 0);
		MyInsertMenu(TrayMenu, i++, IDM_SETTINGS, TR("���������") + CString(_T("...")));
		MyInsertMenu(TrayMenu, i++, 0, 0);
		MyInsertMenu(TrayMenu, i++, IDM_EXIT, TR("�����"));
		if (Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId)
		{
			SetMenuDefaultItem(TrayMenu, Settings.Hotkeys[Settings.TrayIconSettings.LeftDoubleClickCommand].commandId,
			                   FALSE);
		}
	}
	else
		MyInsertMenu(TrayMenu, 0, IDM_STOPUPLOAD, TR("�������� ��������"));
	m_hTrayIconMenu = TrayMenu;
	CMenuHandle oPopup(m_hTrayIconMenu);
	PrepareMenu(oPopup);
	CPoint pos;
	GetCursorPos(&pos);
	SetForegroundWindow(m_hWnd);
	oPopup.TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, m_hWnd);
	// BUGFIX: See "PRB: Menus for Notification Icons Don't Work Correctly"
	PostMessage(WM_NULL);
	return 0;
}

LRESULT CFloatingWindow::OnTimer(UINT id)
{
	if (id == 1)
	{
		KillTimer(1);
		SendMessage(WM_COMMAND, MAKEWPARAM(Settings.Hotkeys[Settings.TrayIconSettings.LeftClickCommand].commandId, 0));
	}
	if (id == 2)
		EnableClicks = true;

	KillTimer(id);
	return 0;
}

inline BOOL SetOneInstance(LPCTSTR szName)
{
	HANDLE hMutex = NULL;
	BOOL bFound = FALSE;
	hMutex = ::CreateMutex(NULL, TRUE, szName);
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		bFound = TRUE;
	if (hMutex)
		::ReleaseMutex(hMutex);
	return bFound;
}

CFloatingWindow floatWnd;

void CFloatingWindow::CreateTrayIcon()
{
	BOOL bFound = FALSE;
	hMutex = ::CreateMutex(NULL, TRUE, _T("ImageUploader_TrayWnd_Mutex"));
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		bFound = TRUE;
	if (hMutex)
		::ReleaseMutex(hMutex);

	if (!bFound)
	{
		CRect r(100, 100, 400, 400);
		floatWnd.Create(0, r, _T("ImageUploader_TrayWnd"), WS_OVERLAPPED | WS_POPUP | WS_CAPTION );
		floatWnd.ShowWindow(SW_HIDE);
	}
}

BOOL IsRunningFloatingWnd()
{
	HANDLE hMutex = NULL;
	BOOL bFound = FALSE;
	hMutex = ::CreateMutex(NULL, TRUE, _T("ImageUploader_TrayWnd_Mutex"));
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		bFound = TRUE;
	if (hMutex)
	{
		::ReleaseMutex(hMutex);
		CloseHandle(hMutex);
	}
	return bFound;
}

void CFloatingWindow::RegisterHotkeys()
{
	m_hotkeys = Settings.Hotkeys;

	for (size_t i = 0; i < m_hotkeys.GetCount(); i++)
	{
		if (m_hotkeys[i].globalKey.keyCode)
		{
			if (!RegisterHotKey(m_hWnd, i, m_hotkeys[i].globalKey.keyModifier, m_hotkeys[i].globalKey.keyCode))
			{
				CString msg;
				msg.Format(TR("���������� ���������������� ���������� ��������� ������\n%s.\n ��������, ��� ������ ������ ����������."),
				           (LPCTSTR)m_hotkeys[i].globalKey.toString());
				WriteLog(logWarning, _T("Hotkeys"), msg);
			}
		}
	}
}

LRESULT CFloatingWindow::OnHotKey(int HotKeyID, UINT flags, UINT vk)
{
	if (HotKeyID < 0 || HotKeyID > int(m_hotkeys.GetCount()) - 1)
		return 0;
	if (m_bIsUploading)
		return 0;

	if (m_hotkeys[HotKeyID].func == _T("windowscreenshot"))
	{
		pWizardDlg->executeFunc(_T("windowscreenshot,1"));
	}
	else
	{
		m_bFromHotkey = true;
		SetActiveWindow();
		SetForegroundWindow(m_hWnd);
		SendMessage(WM_COMMAND, MAKEWPARAM(m_hotkeys[HotKeyID].commandId, 0));
		m_bFromHotkey = false;
	}
	return 0;
}

void CFloatingWindow::UnRegisterHotkeys()
{
	for (size_t i = 0; i < m_hotkeys.GetCount(); i++)
	{
		if (m_hotkeys[i].globalKey.keyCode)
			UnregisterHotKey(m_hWnd, i);
	}
	m_hotkeys.RemoveAll();
}

LRESULT CFloatingWindow::OnPaste(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (pWizardDlg->executeFunc(_T("paste")))
		pWizardDlg->ShowWindow(SW_SHOW);
	return 0;
}

LRESULT CFloatingWindow::OnMediaInfo(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (pWizardDlg->executeFunc(_T("mediainfo")))
		pWizardDlg->ShowWindow(SW_SHOW);
	return 0;
}

LRESULT CFloatingWindow::OnTaskbarCreated(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	InstallIcon(APPNAME, m_hIconSmall, 0);
	return 0;
}

LRESULT CFloatingWindow::OnScreenshotActionChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	Settings.TrayIconSettings.TrayScreenshotAction = wID - IDM_SCREENTSHOTACTION_UPLOAD;
	Settings.SaveSettings();
	return 0;
}


void CFloatingWindow::ShowBaloonTip(const CString& text, const CString& title)
{
	NOTIFYICONDATA nid;
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = NOTIFYICONDATA_V2_SIZE;
	nid.hWnd = m_hWnd;
	nid.uTimeout = 5500;
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = NIIF_INFO;
	lstrcpyn(nid.szInfo, text, ARRAY_SIZE(nid.szInfo) - 1);
	lstrcpyn(nid.szInfoTitle, title, ARRAY_SIZE(nid.szInfoTitle) - 1);
	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void CFloatingWindow::UploadScreenshot(const CString& realName, const CString& displayName)
{
	delete m_FileQueueUploader;
	m_FileQueueUploader =  0;
	lastUploadedItem_.fileListItem = CFileQueueUploader::FileListItem();
	lastUploadedItem_.imageUrlShortened.Empty();
	lastUploadedItem_.downloadUrlShortened.Empty();

	m_FileQueueUploader = new CFileQueueUploader();
	m_FileQueueUploader->setCallback(this);
	ServerProfile &serverProfile = Settings.quickScreenshotServer;
	CUploadEngineData* engineData = serverProfile.uploadEngineData();
	if (!engineData)
		engineData = _EngineList->byIndex(_EngineList->getRandomImageServer());
	if (!engineData)
		return;

	CImageConverter imageConverter;
	Thumbnail thumb;

	if (!thumb.LoadFromFile(WCstringToUtf8(IuCommonFunctions::GetDataFolder() + _T("\\Thumbnails\\") + Settings.quickScreenshotServer.getImageUploadParams().getThumb().TemplateName +
	                                       _T(".xml"))))
	{
		WriteLog(logError, _T("CThumbSettingsPage"), TR("�� ���� ��������� ���� ���������!"));
		return;
	}
	imageConverter.setEnableProcessing(Settings.quickScreenshotServer.getImageUploadParams().ProcessImages);
	imageConverter.setImageConvertingParams(Settings.ConvertProfiles[Settings.quickScreenshotServer.getImageUploadParams().ImageProfileName]);
	imageConverter.setThumbCreatingParams(Settings.quickScreenshotServer.getImageUploadParams().getThumb());
	bool GenThumbs = Settings.quickScreenshotServer.getImageUploadParams().CreateThumbs &&
	   ((!Settings.quickScreenshotServer.getImageUploadParams().UseServerThumbs) || (!engineData->SupportThumbnails));
	imageConverter.setThumbnail(&thumb);
	imageConverter.setGenerateThumb(GenThumbs);
	imageConverter.Convert(realName);

	CAbstractUploadEngine* engine = _EngineList->getUploadEngine(engineData, serverProfile.serverSettings());
	if (!engine)
		return;

	m_FileQueueUploader->setUploadSettings(engine);

	source_file_name_ = WCstringToUtf8(realName);
	server_name_ = engineData->Name;
	std::string utf8FileName = WCstringToUtf8(imageConverter.getImageFileName());
	lastUploadedItem_.fileListItem.fileSize = IuCoreUtils::getFileSize(utf8FileName);
	m_FileQueueUploader->AddFile(utf8FileName, WCstringToUtf8(displayName), 0, engine);
	
	CString thumbFileName = imageConverter.getThumbFileName();
	if (!thumbFileName.IsEmpty())
		m_FileQueueUploader->AddFile(WCstringToUtf8(thumbFileName), WCstringToUtf8(thumbFileName),
		                             reinterpret_cast<void*>(1), engine);

	m_bIsUploading = true;
	uploadType_ = utImage;
	m_FileQueueUploader->start();
	CString msg;
	msg.Format(TR("���� �������� \"%s\" �� ������ %s"), (LPCTSTR) GetOnlyFileName(displayName),
	           (LPCTSTR)Utf8ToWstring(engineData->Name).c_str());
	ShowBaloonTip(msg, TR("�������� ������"));
}

bool CFloatingWindow::OnQueueFinished(CFileQueueUploader*) {
	m_bIsUploading = false;
	bool usedDirectLink = true;

	if ( uploadType_ == utImage ) {
		CString url;
		if ((Settings.UseDirectLinks || lastUploadedItem_.fileListItem.downloadUrl.empty()) && !lastUploadedItem_.fileListItem.imageUrl.empty() )
			url = Utf8ToWstring(lastUploadedItem_.fileListItem.imageUrl).c_str();
		else if ((!Settings.UseDirectLinks || lastUploadedItem_.fileListItem.imageUrl.empty()) && !lastUploadedItem_.fileListItem.downloadUrl.empty() ) {
			url = Utf8ToWstring(lastUploadedItem_.fileListItem.downloadUrl).c_str();
			usedDirectLink = false;
		}


		if (url.IsEmpty())
		{
			ShowBaloonTip(TR("�� ������� ��������� ������ :("), _T("Image Uploader"));
			return true;
		}

		CHistoryManager* mgr = ZBase::get()->historyManager();
		std_tr::shared_ptr<CHistorySession> session = mgr->newSession();
		HistoryItem hi;
		hi.localFilePath = source_file_name_;
		hi.serverName = server_name_;
		hi.directUrl =  (lastUploadedItem_.fileListItem.imageUrl);
		hi.thumbUrl = (lastUploadedItem_.fileListItem.thumbUrl);
		hi.viewUrl = (lastUploadedItem_.fileListItem.downloadUrl);
		hi.uploadFileSize = lastUploadedItem_.fileListItem.fileSize; // IuCoreUtils::getFileSize(WCstringToUtf8(ImageFileName));
		session->AddItem(hi);

		if ( Settings.TrayIconSettings.ShortenLinks ) {
			std_tr::shared_ptr<UrlShorteningTask> task(new UrlShorteningTask(WCstringToUtf8(url)));

			CUploadEngineData *ue = Settings.urlShorteningServer.uploadEngineData();
			if ( !ue ) {
				ShowImageUploadedMessage(url);
				return false;

			}
			CAbstractUploadEngine * e = _EngineList->getUploadEngine(ue,Settings.urlShorteningServer.serverSettings());
			if ( !e ) {
				ShowImageUploadedMessage(url);
				return false;
			}
			e->setUploadData(ue);
			ServerSettingsStruct& settings = Settings.urlShorteningServer.serverSettings();
			e->setServerSettings(settings);
			e->setUploadData(ue);
			uploadType_ = utShorteningImageUrl;
			UploadTaskUserData* uploadTaskUserData = new UploadTaskUserData;
			uploadTaskUserData->linkTypeToShorten = usedDirectLink ? _T("ImageUrl") : _T("DownloadUrl");
			m_FileQueueUploader->AddUploadTask(task, reinterpret_cast<UploadTaskUserData*>(uploadTaskUserData), e);
			m_FileQueueUploader->start();
		} else {
			ShowImageUploadedMessage(url);
		}
	}
	return true;
}

bool CFloatingWindow::OnFileFinished(bool ok, CFileQueueUploader::FileListItem& result)
{
	if ( uploadType_ == utUrl ) {
		if ( ok ) {
			CString url = Utf8ToWCstring(result.imageUrl);
			IU_CopyTextToClipboard(url);
			ShowBaloonTip( TrimString(url, 70) + CString("\r\n")
				+ TR("(����� ��� ������������� ������� � ����� ������)"), TR("�������� ������"));
		} else {
			ShowBaloonTip( TR("��� ������������ �������� ���."), TR("�� ������� ��������� ������...") );
		}
	} else if ( uploadType_ == utShorteningImageUrl) {
		if ( ok ) { 
			CString url = Utf8ToWCstring(result.imageUrl);
			UploadTaskUserData *uploadTaskUserData = reinterpret_cast<UploadTaskUserData*>(result.uploadTask->userData);

			if ( uploadTaskUserData->linkTypeToShorten == "ImageUrl" ) {
				lastUploadedItem_.imageUrlShortened = url;
			} else if ( uploadTaskUserData->linkTypeToShorten == "DownloadUrl" ) {
				lastUploadedItem_.downloadUrlShortened = url;
			}
			ShowImageUploadedMessage(url);
		} else {
			UrlShorteningTask *urlShorteningTask = (UrlShorteningTask*) result.uploadTask;
			CString url = Utf8ToWCstring(urlShorteningTask->getUrl());
			ShowImageUploadedMessage(url);
		}
	} else {
		if (ok)
		{
			if (result.uploadTask->userData == 0)
				lastUploadedItem_.fileListItem = result;
			else if (int(result.uploadTask->userData) == 1)
				lastUploadedItem_.fileListItem.thumbUrl = result.imageUrl;	
		}
	}
	return true;
}

bool CFloatingWindow::OnConfigureNetworkClient(CFileQueueUploader *uploader, NetworkClient* nm)
{
	IU_ConfigureProxy(*nm);
	return true;
}

LRESULT CFloatingWindow::OnStopUpload(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (m_FileQueueUploader)
		m_FileQueueUploader->stop();
	return 0;
}

void CFloatingWindow::ShowImageUploadedMessage(const CString& url) {
	IU_CopyTextToClipboard(url);
	ShowBaloonTip(TrimString(url, 70) + CString("\r\n") 
		+ TR("(����� ��� ������������� ������� � ����� ������)")+ + CString("\r\n") + TR("������� �� ��� ��������� ��� �������� ���� � �����...") , TR("������ ������� ��������"));
}