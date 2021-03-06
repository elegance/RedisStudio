#include "stdafx.h"
#include "ConnInfoUI.h"
#include "Base/CharacterSet.h"
#include "UserMessage.h"
#include "Redis/RedisClient.h"
#include "ConnInfoSubWhd.h"
#include "rapidjson/document.h"		// rapidjson's DOM-style API
#include "rapidjson/prettywriter.h"	// for stringify JSON
#include "rapidjson/filestream.h"	// wrapper of C stream for prettywriter as output
using namespace rapidjson;

DUI_BEGIN_MESSAGE_MAP(ConnInfoUI, CNotifyPump)
DUI_ON_MSGTYPE(DUI_MSGTYPE_CLICK,OnClick)
DUI_ON_MSGTYPE(DUI_MSGTYPE_SELECTCHANGED,OnSelectChanged)
DUI_ON_MSGTYPE(DUI_MSGTYPE_ITEMCLICK,OnItemClick)
DUI_ON_MSGTYPE(DUI_MSGTYPE_ITEMACTIVATE,OnItemActive)
DUI_END_MESSAGE_MAP()

static const char* const kConfigFilePath = "Config.json" ;
bool ConnInfoUI::m_isConecting = false;

ConnInfoUI::ConnInfoUI(const CDuiString& strXML,CPaintManagerUI* pm):AbstraceUI(pm)
{
	CDialogBuilder builder;
	CControlUI* pContainer = builder.Create(strXML.GetData(), NULL, NULL, GetPaintMgr(), NULL); // 这里必须传入m_PaintManager，不然子XML不能使用默认滚动条等信息。
	if( pContainer ) {
		this->Add(pContainer);
	}
	else {
		this->RemoveAll();
		return;
	}
}

void ConnInfoUI::Initialize()
{
	m_pRfhBtn = static_cast<CButtonUI*>(GetPaintMgr()->FindControl(_T("btn_conn_rfh")));
	m_pAddBtn = static_cast<CButtonUI*>(GetPaintMgr()->FindControl(_T("btn_conn_add")));
	m_pDelBtn = static_cast<CButtonUI*>(GetPaintMgr()->FindControl(_T("btn_conn_del")));
	m_pAltBtn = static_cast<CButtonUI*>(GetPaintMgr()->FindControl(_T("btn_conn_alt")));
	m_pListUI = static_cast<CListUI*> (GetPaintMgr()->FindControl(_T("serverlist")));
}

int ConnInfoUI::GetIndex()
{
	return 0;
}

DuiLib::CDuiString ConnInfoUI::GetVirtualwndName()
{
	return _T("ConnInfo");
}

void ConnInfoUI::RefreshWnd()
{
	LoadConfig(kConfigFilePath);
}


bool ConnInfoUI::LoadConfig( const std::string& path )
{	
	m_pListUI->RemoveAll();
	/// 设置利用回调填充数据
	m_pListUI->SetTextCallback(this);//[1]
	m_dicServerInfo.clear();
	
	FILE* fp = fopen(kConfigFilePath, "r");
	if (!fp) return false;

	FileStream is(fp);
	Document document;	// Default template parameter uses UTF8 and MemoryPoolAllocator.
	if (document.ParseStream<0>(is).HasParseError())
	{
		UserMessageBox(GetHWND(), 10014, Base::CharacterSet::ANSIToUnicode(document.GetParseError()).c_str(), MB_ICONERROR);
		fclose(fp);
		return false;
	}

	const Value& serverConfig = document["Server"];
	int idx = 0;
	for (Value::ConstValueIterator itr = serverConfig.Begin(); itr != serverConfig.End(); ++itr)
	{
		const Value& subConfig = *itr;
		CDuiString name = Base::CharacterSet::UTF8ToUnicode(subConfig["name"].GetString()).c_str();
		m_dicServerInfo[kServerNameIndex].push_back(Base::CharacterSet::UnicodeToANSI(name.GetData()));
		m_dicServerInfo[kServerIpIndex].push_back(subConfig["ip"].GetString());
		std::ostringstream os;
		os<<subConfig["port"].GetInt();
		m_dicServerInfo[kServerPortIndex].push_back(os.str());
		m_dicServerInfo[kServerAuthIndex].push_back(subConfig["auth"].GetString());

		CListTextElementUI* pListElement = new CListTextElementUI;
		pListElement->SetTag(idx++);
		/// 设置工具提示
		pListElement->SetToolTip(m_pListUI->GetToolTip());
		m_pListUI->Add(pListElement);
	}
	fclose(fp);

	return true;
}


bool ConnInfoUI::SaveConfig( const std::string& path )
{
	std::size_t theSize = m_dicServerInfo[kServerNameIndex].size();
	//Json::Value serverConfig;
	//Json::Value rootConfig;

	FILE* fp = fopen(kConfigFilePath, "w+");
	if (!fp) return false;
	FileStream os(fp);
	PrettyWriter<FileStream> writer(os);
	writer.StartObject();
	writer.String("Server");
	writer.StartArray();
	for (std::size_t idx=0; idx<theSize; ++idx)
	{
		writer.StartObject();
		writer.String("name");
		CDuiString name = Base::CharacterSet::ANSIToUnicode(m_dicServerInfo[kServerNameIndex][idx]).c_str();

		writer.String(Base::CharacterSet::UnicodeToUTF8(name.GetData()).c_str());
		writer.String("ip");
		writer.String(m_dicServerInfo[kServerIpIndex][idx].c_str(), m_dicServerInfo[kServerIpIndex][idx].length());
		writer.String("port");
		writer.Int(atoi(m_dicServerInfo[kServerPortIndex][idx].c_str()));
		writer.String("auth");
		writer.String(m_dicServerInfo[kServerAuthIndex][idx].c_str());
		writer.EndObject();
	}
	writer.EndArray();
	writer.EndObject();
	fclose(fp);
	return true;
}


LPCTSTR ConnInfoUI::GetItemText( CControlUI* pControl, int iIndex, int iSubItem )
{
	const std::string& str = m_dicServerInfo[iSubItem][iIndex];
	pControl->SetUserData(Base::CharacterSet::ANSIToUnicode(str).c_str());
	return pControl->GetUserData();
}

void ConnInfoUI::DoEvent(TEventUI& event)
{
	if (event.pSender->GetName() == _T("btn_refresh") &&
		event.Type == UIEVENT_BUTTONUP)
	{
		LoadConfig(kConfigFilePath);
	}
}


void ConnInfoUI::OnClick(TNotifyUI& msg)
{
	if (msg.pSender == m_pRfhBtn)
	{
		OnRfhConnInfo(msg);
	}
	else if (msg.pSender == m_pAddBtn)
	{
		OnAddConnInfo(msg);
	}
	else if (msg.pSender == m_pDelBtn)
	{
		OnDelConnInfo(msg);
	}
	else if (msg.pSender == m_pAltBtn)
	{
		OnAltConnInfo(msg);
	}
}

void ConnInfoUI::OnSelectChanged( TNotifyUI &msg )
{

}

void ConnInfoUI::OnItemClick( TNotifyUI &msg )
{

}

void ConnInfoUI::OnItemActive( TNotifyUI &msg )
{
	if (m_isConecting)
	{
		UserMessageBox(GetHWND(), 10013, NULL, MB_ICONINFORMATION);
		return ;
	}
	int idx = m_pListUI->GetCurSel();
	CDuiString* s = new CDuiString(Base::CharacterSet::ANSIToUnicode(m_dicServerInfo[kServerNameIndex][idx]).c_str());
	std::string ip = m_dicServerInfo[kServerIpIndex][idx];
	int         port = atoi(m_dicServerInfo[kServerPortIndex][idx].c_str());
	std::string auth = m_dicServerInfo[kServerAuthIndex][idx];
	RedisClient::GetInstance().SetServerInfo(*s, ip, port, auth);
	DoConnect();
}

void ConnInfoUI::OnAddConnInfo(TNotifyUI& msg)
{
	ConnInfoSubWhd* pWhd = new ConnInfoSubWhd(&m_dicServerInfo, ConnInfoSubWhd::Type_Add);
	if( pWhd == NULL ) { return; }
	pWhd->Create(GetHWND(), NULL, UI_CLASSSTYLE_DIALOG, 0, 0, 0, 0, 0, NULL);
	pWhd->CenterWindow();
	pWhd->ShowModal();
	SaveConfig(kConfigFilePath);
	LoadConfig(kConfigFilePath);
}

void ConnInfoUI::OnDelConnInfo(TNotifyUI& msg)
{
	if (m_pListUI->GetCurSel() < 0)
	{
		UserMessageBox(GetHWND(), 20002, NULL, MB_ICONINFORMATION);
		return;
	}
	int idx = m_pListUI->GetCurSel();
	m_dicServerInfo[kServerNameIndex].erase(m_dicServerInfo[kServerNameIndex].begin()+idx);
	m_dicServerInfo[kServerIpIndex].erase(m_dicServerInfo[kServerIpIndex].begin()+idx);
	m_dicServerInfo[kServerPortIndex].erase(m_dicServerInfo[kServerPortIndex].begin()+idx);
	m_dicServerInfo[kServerAuthIndex].erase(m_dicServerInfo[kServerAuthIndex].begin()+idx);
	SaveConfig(kConfigFilePath);
	LoadConfig(kConfigFilePath);
}

void ConnInfoUI::OnAltConnInfo(TNotifyUI& msg)
{
	if (m_pListUI->GetCurSel() < 0)
	{
		UserMessageBox(GetHWND(), 20002, NULL, MB_ICONINFORMATION);
		return;
	}
	ConnInfoSubWhd* pWhd = new ConnInfoSubWhd(&m_dicServerInfo, ConnInfoSubWhd::Type_Alt,m_pListUI->GetCurSel());
	if( pWhd == NULL ) { return; }
	pWhd->Create(GetHWND(), NULL, UI_CLASSSTYLE_DIALOG, 0, 0, 0, 0, 0, NULL);
	pWhd->CenterWindow();
	pWhd->ShowModal();
	SaveConfig(kConfigFilePath);
	LoadConfig(kConfigFilePath);
}

void ConnInfoUI::OnRfhConnInfo(TNotifyUI& msg)
{
	LoadConfig(kConfigFilePath);
}

void ConnInfoUI::DoConnect()
{
	m_isConecting = true;
	DWORD dwThreadID = 0;
	HANDLE hThread = CreateThread(NULL, 0, &ConnInfoUI::BackgroundWork, GetHWND(),  0, &dwThreadID);
}


DWORD WINAPI ConnInfoUI::BackgroundWork( LPVOID lpParameter )
{	
	HWND hwnd = (HWND) lpParameter;
	RedisClient& obj = RedisClient::GetInstance();
	CDuiString* s = new CDuiString(obj.GetName());
	::PostMessage(hwnd, WM_USER_CONNECTING, (WPARAM)s, NULL);
	s = new CDuiString(obj.GetName());
	obj.Connect();
	
	if (obj.IsConnected())
	{
		::PostMessage(hwnd, WM_USER_CONNECTED, (WPARAM)s, NULL);
	}
	else 
	{
		::PostMessage(hwnd, WM_USER_UNCONNECT, (WPARAM)s, NULL);
	}
	m_isConecting = false;
	return 0;
}
