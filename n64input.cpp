#include "ControllerImpl.h"
#include "config.h"

n64_controller controller[4];

void ControllerCommand ( int Control, BYTE * Command)
{}

void DllAbout ( HWND hParent )
{
	CAboutDlg dlg;
	dlg.DoModal();
}

void DllConfig ( HWND hParent )
{
	CMainDlg dlg;
	dlg.DoModal();
}

void DllTest ( HWND hParent )
{
	MessageBox(NULL,"This is a test :3\nThis is only a test :3\nThis is a test :3\nThis is a test of the emergency broadcasting system :3","hellu!",MB_ICONINFORMATION);
}

void GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
	PluginInfo->Version = 0x0100;
	PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
	wsprintf(PluginInfo->Name,"%s",PLUGIN_VERSION);

}

void GetKeys(int Control, BUTTONS * Keys )
{}

void InitiateControllers (HWND hMainWindow, CONTROL Controls[4]){
	for( int i = 0; i < 4; i++)	
	{
		InitController(&controller[i],i);
		Controls[i].Present = controller[i].plugged_in;
	    Controls[i].Plugin = controller[i].pak_type;
		Controls[i].RawData	= true;
	}	
	return;
}

void ReadController ( int Control, BYTE * Command )
{
	if( Control == -1 )return;
	if( !controller[Control].plugged_in ){
		Command[1] |= RD_ERROR;
		return;
	}
	switch( Command[2] )
	{
	case RD_RESETCONTROLLER:
	case RD_GETSTATUS:
		Command[3] = RD_GAMEPAD | RD_ABSOLUTE;
		Command[4] = RD_NOEEPROM;
		if( controller[Control].plugged_in && controller[Control].pak_data )
		{
			Command[5] = RD_PLUGIN;
		}
    break;
	case RD_READKEYS:
		if( controller[Control].plugged_in)
		{
			GetButtons(&controller[Control]);
			memcpy((LPDWORD)&Command[3],&controller[Control].Keys,sizeof(BUTTONS));
		}
	break;
	case RD_READPAK:
		ReadFromPak( Control, &Command[3] );
		break;
	case RD_WRITEPAK:
		WriteToPak( Control, &Command[3] );
		break;
	default:
		Command[1] = Command[1] | RD_ERROR;
	}
	return;
}

void RomClosed (void)
{
	for( int i = 0; i > 4 ; i++)	
	{
		if( controller[i].plugged_in)
		{
		SaveControllerPak(&controller[i],i);
		CloseController(&controller[i],i);
		}
	}	
}

void RomOpen (void)
{
}

void WM_KeyDown( WPARAM wParam, LPARAM lParam )
{
}

void WM_KeyUp( WPARAM wParam, LPARAM lParam )
{
}

void CloseDLL ()
{
	
	return;
}

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved )  // reserved
{
	// Perform actions based on the reason for calling.
	switch( fdwReason ) 
	{ 
	case DLL_PROCESS_ATTACH:
		srand(GetTickCount());
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup.
		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
	