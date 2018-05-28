#include <atlbase.h>
#include <atlapp.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlwin.h>
#include <atlstr.h>
#include <atlctrlx.h>
#include "resource.h"
#include "configwriteread.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

void save_cfg(pn64_controller controller, int controller_number)
{
	TCHAR szBuffer[MAX_PATH+1];
	LPTSTR  strDLLPath1 = new TCHAR[_MAX_PATH];
	GetModuleFileName((HINSTANCE)&__ImageBase, strDLLPath1, _MAX_PATH);
	PathRemoveFileSpec( strDLLPath1);
	ZeroMemory(szBuffer,sizeof(szBuffer));
	wsprintf(szBuffer,"%s\\n64input.ini",strDLLPath1);
	TCHAR szSection[MAX_PATH];
	ZeroMemory(szSection,sizeof(szSection));
	TCHAR szKey[MAX_PATH];
	ZeroMemory(szKey,sizeof(szKey));
	CIniWriter iniWriter(szBuffer);
	ZeroMemory(szBuffer,sizeof(szBuffer));
	wsprintf(szSection,"Player%d",controller_number);
	iniWriter.WriteInteger(szSection,"PAK_TYPE",controller->pak_type); 
	iniWriter.WriteString(szSection,"Path",controller->controllerpak_path);  
	free(strDLLPath1);
}

void load_cfg(pn64_controller controller, int controller_number)
{
	TCHAR szBuffer[MAX_PATH+1];
	LPTSTR  strDLLPath1 = new TCHAR[_MAX_PATH];
	GetModuleFileName((HINSTANCE)&__ImageBase, strDLLPath1, _MAX_PATH);
	PathRemoveFileSpec( strDLLPath1);
	ZeroMemory(szBuffer,sizeof(szBuffer));
	wsprintf(szBuffer,"%s\\n64input.ini",strDLLPath1);
	TCHAR szSection[MAX_PATH];
	ZeroMemory(szSection,sizeof(szSection));
	TCHAR szKey[MAX_PATH];
	ZeroMemory(szKey,sizeof(szKey));
	CIniReader iniReader(szBuffer);
	ZeroMemory(szBuffer,sizeof(szBuffer));
	wsprintf(szSection,"Player%d",controller_number);
	int pak_type = iniReader.ReadInteger(szSection,"PAK_TYPE",1);
	controller->pak_type = (_n64_controller::PAKTYPE)pak_type;
	wsprintf(szBuffer,"%s\\n64input%d.mpk",strDLLPath1,controller_number);
	char* mempak_filename = iniReader.ReadString(szSection,"Path",szBuffer);
	strcpy(controller->controllerpak_path,mempak_filename);
	free(strDLLPath1);
}

void save_cfg();

class CAboutDlg : public CDialogImpl<CAboutDlg>
{
	CHyperLink website;
	CStatic version_number;
public:
	enum { IDD = IDD_ABOUT };
	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialogView1)
		COMMAND_ID_HANDLER(IDOK,OnButtonOK)
	END_MSG_MAP()
	LRESULT OnInitDialogView1(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		version_number = GetDlgItem(IDC_PLUGINNAME);
		version_number.SetWindowText(PLUGIN_VERSION);
		website.SubclassWindow(GetDlgItem(IDC_SITELINK));
		website.SetHyperLink(_T("http://vba-m.com/forum/Forum-n64input"));
		return TRUE;
	}
	LRESULT OnButtonOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};


char *player_name[]={
	"One",
	"Two",
	"Three",
	"Four"
};
char *pak_description[]={
	"no Pak.",
	"a Controller Pak.",
	"a Rumble Pak."
};

class CMainDlg : public CDialogImpl<CMainDlg>
{
	n64_controller controller_settings[4];
	CComboBox player1,player2,player3,player4, player_mempak;
	CEdit mempakpath;
public:
	enum { IDD = IDD_CONFIG };


	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDC_MEMPAKLOAD,OnMemPakLoad)
		COMMAND_ID_HANDLER(IDC_MEMPAKSAVE,OnMemPakSave)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		REFLECT_NOTIFICATIONS();
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// center the dialog on the screen
		CenterWindow();

		player1 = GetDlgItem(IDC_PAKTYPE1);
		player2 = GetDlgItem(IDC_PAKTYPE2);
		player3 = GetDlgItem(IDC_PAKTYPE3);
		player4 = GetDlgItem(IDC_PAKTYPE4);
		player_mempak = GetDlgItem(IDC_MEMPAKPLAYER);
		mempakpath = GetDlgItem(IDC_MEMPAKPATH);
	
		ZeroMemory( controller, sizeof(_n64_controller) );
		for( int i = 0; i < 4; i++)	
		{
			load_cfg(&controller_settings[i],i);
		}	
		for (int i = 0; i < arrayof(player_name);i++)
		{
			player_mempak.AddString(player_name[i]);
		}
		for (int i = 0; i < arrayof(pak_description);i++)
		{
			player1.AddString(pak_description[i]);
			player2.AddString(pak_description[i]);
			player3.AddString(pak_description[i]);
			player4.AddString(pak_description[i]);
		}

		player1.SetCurSel(controller_settings[0].pak_type-1);
		player2.SetCurSel(controller_settings[1].pak_type-1);
		player3.SetCurSel(controller_settings[2].pak_type-1);
		player4.SetCurSel(controller_settings[3].pak_type-1);

		return TRUE;
	}

	LRESULT OnMemPakLoad(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		TCHAR szFileName[MAX_PATH];
		ZeroMemory(szFileName,sizeof(szFileName));
		int sel = player_mempak.GetCurSel();
		if(sel != -1)
		{
			strcpy(szFileName,controller_settings[sel].controllerpak_path);
			mempakpath.SetWindowText(szFileName);
			CFileDialog dlg( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Controller pak files (*.mpk)\0*.mpk\0");
			if (dlg.DoModal() == IDOK){
			lstrcpy(szFileName,dlg.m_szFileName);
			mempakpath.SetWindowText(szFileName);		
			}
		}
		return 0;
	}

	LRESULT OnMemPakSave(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		TCHAR szFileName[MAX_PATH];
		ZeroMemory(szFileName,sizeof(szFileName));
		int sel = player_mempak.GetCurSel();
		if(sel != -1)
		{
			CFileDialog dlg( FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Controller pak files (*.mpk)\0*.mpk\0");
			if (dlg.DoModal() == IDOK)
			{
				lstrcpy(szFileName,dlg.m_szFileName);
				mempakpath.SetWindowText(szFileName);
				strcpy(controller_settings[sel].controllerpak_path,szFileName);
				MessageBox("Controller pak changed!","Notice",MB_OK);
				// do stuff
			}
		}
		return 0;
	}

	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		controller_settings[0].pak_type = (_n64_controller::PAKTYPE)(player1.GetCurSel() + 1);
		controller_settings[1].pak_type = (_n64_controller::PAKTYPE)(player2.GetCurSel() + 1);
		controller_settings[2].pak_type = (_n64_controller::PAKTYPE)(player3.GetCurSel() + 1);
		controller_settings[3].pak_type = (_n64_controller::PAKTYPE)(player4.GetCurSel() + 1);
		for (int i = 0; i < 4;i++)
		{
			save_cfg(&controller_settings[i], i);
		}
		MessageBox("All settings saved!","Yay!",MB_OK);

		// TODO: Add save code
		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};

