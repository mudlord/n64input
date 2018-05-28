#include <windows.h>
#include "ControllerImpl.h"
#include <XInput.h>
#include <Commctrl.h>
#include "Shlwapi.h"
#pragma comment(lib, "XInput.lib")
	
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

void CloseController(pn64_controller controller, int controller_number)
{
	if( controller && controller->pak_data ){
		switch (*(BYTE*)controller[controller_number].pak_data)
		{
		case _n64_controller::PAK_NONE:
			break;
		case _n64_controller::PAK_CONTROLLER:
			{
				MEMPAK *mPak =  (MEMPAK*)controller->pak_data;
				if( !mPak->fReadonly ){
					UnmapViewOfFile( mPak->aMemPakData );
					if ( mPak->hMemPakHandle != NULL )
						CloseHandle( mPak->hMemPakHandle );
				}
				else{
					free( mPak->aMemPakData );
					mPak->aMemPakData = NULL;
				}
			}
			break;
		case _n64_controller::PAK_RUMBLE:
			free( controller->pak_data );
			controller->pak_data = NULL;
			break;
		}
	}
}

void SaveControllerPak(pn64_controller controller, int controller_number)
{
	if( controller && controller->pak_data )
	{
		switch (*(BYTE*)controller[controller_number].pak_data)
		{
		case _n64_controller::PAK_CONTROLLER:
			{
				MEMPAK *mPak =  (MEMPAK*)controller->pak_data;
				if( !mPak->fReadonly )
				FlushViewOfFile( mPak->aMemPakData, PAK_MEM_SIZE );	// we've already written the stuff, just flush the cache
			}
		break;
		}
	}
}

bool CheckFileExists(const TCHAR *fileName)
{
	DWORD       fileAttr;
	fileAttr = GetFileAttributes(fileName);
	if (0xFFFFFFFF == fileAttr)return false;
	return true;
}

void SetXInputControllerDeadZone(XINPUT_GAMEPAD *Gamepad, 
	int LeftThumbDZ = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, 
	int RightThumbDZ = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
{
	if( ( Gamepad->sThumbLX < LeftThumbDZ && Gamepad->sThumbLX > -LeftThumbDZ ) &&
		( Gamepad->sThumbLY < LeftThumbDZ && Gamepad->sThumbLY > -LeftThumbDZ ) )
	{
		Gamepad->sThumbLX = 0;
		Gamepad->sThumbLY = 0;
	}
	if( ( Gamepad->sThumbRX < RightThumbDZ && Gamepad->sThumbRX > -RightThumbDZ ) &&
		( Gamepad->sThumbRY < RightThumbDZ && Gamepad->sThumbRY > -RightThumbDZ ) )
	{
		Gamepad->sThumbRX = 0;
		Gamepad->sThumbRY = 0;
	}
}

bool InitController(pn64_controller controller, int controller_number)
{
	    bool bReturn = false;
	    ZeroMemory( controller, sizeof(_n64_controller) );
		DWORD dwResult;         
		XINPUT_STATE state;    
		ZeroMemory( &state, sizeof(XINPUT_STATE) ); 
		dwResult = XInputGetState( controller_number, &state );
		if( dwResult != ERROR_SUCCESS ){
			ZeroMemory( &state, sizeof(XINPUT_STATE) ); 
		}
		controller->plugged_in = (dwResult == ERROR_SUCCESS)?TRUE:FALSE;
	
	    void load_cfg(pn64_controller controller, int controller_number);
		load_cfg(controller,controller_number);

	//	controller->pak_type = _n64_controller::PAK_CONTROLLER;
		//controller->controller_number = controller_number;

		if( controller->plugged_in)
		{
			SaveControllerPak(controller, controller_number);
			CloseController(controller, controller_number);
		}

		switch(controller->pak_type)
		{
		case _n64_controller::PAK_CONTROLLER:
			{
				controller->pak_data = malloc(sizeof(MEMPAK));
				MEMPAK *mPak = (MEMPAK*)controller->pak_data;
				mPak->bPakType =  _n64_controller::PAK_CONTROLLER;
				mPak->fReadonly = false;
				mPak->hMemPakHandle = NULL;
				DWORD dwFilesize = PAK_MEM_SIZE;
				bool isNewfile = !CheckFileExists( controller->controllerpak_path );

				HANDLE hFileHandle = CreateFile( controller->controllerpak_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL );
				if( hFileHandle == INVALID_HANDLE_VALUE ){
					hFileHandle = CreateFile( controller->controllerpak_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL );
					if( hFileHandle != INVALID_HANDLE_VALUE ){
						mPak->fReadonly = true;
					}
					else{
						controller->pak_type = _n64_controller::PAK_NONE;// set so that CloseControllerPak doesn't try to close a file that isn't open
						break; // memory is freed at the end of this function
					}
				}

				DWORD dwCurrentSize = GetFileSize( hFileHandle, NULL );
				if ( mPak->fReadonly )
				{
					DWORD dwBytesRead = 0;
					SetFilePointer( hFileHandle, 0L, NULL, FILE_BEGIN );
					dwFilesize = min( dwFilesize, GetFileSize( hFileHandle, NULL ));
					if( !isNewfile ){
						mPak->aMemPakData = (LPBYTE)malloc( sizeof(BYTE) * PAK_MEM_SIZE );
						if( ReadFile( hFileHandle, mPak->aMemPakData, PAK_MEM_SIZE, &dwBytesRead, NULL )){
							if( dwBytesRead < dwFilesize )
								FillMemory( (LPBYTE)mPak->aMemPakData + dwBytesRead, PAK_MEM_SIZE - dwBytesRead, 0xFF );

							bReturn = true;
						}
						else
							free( mPak->aMemPakData );
					}
					else{
						FormatMemPak( mPak->aMemPakData );
						bReturn = true;
					}
					CloseHandle( hFileHandle );
				}
				else
				{
					mPak->hMemPakHandle = CreateFileMapping( hFileHandle, NULL, mPak->fReadonly ? PAGE_READONLY : PAGE_READWRITE, 0, dwFilesize, NULL);
					if (mPak->hMemPakHandle == NULL){
						CloseHandle(hFileHandle);
						break; 
					}
					CloseHandle(hFileHandle); // we can close the file handle now with no problems
					mPak->aMemPakData = (LPBYTE)MapViewOfFile( mPak->hMemPakHandle, FILE_MAP_ALL_ACCESS, 0, 0, PAK_MEM_SIZE );
					if( dwCurrentSize < dwFilesize )
						FillMemory( (LPBYTE)mPak->aMemPakData + (dwCurrentSize), dwFilesize - dwCurrentSize, 0xFF );
					if( isNewfile ){
						FormatMemPak( mPak->aMemPakData );
					}
					bReturn = true;
				}			
			}
			break;
		case _n64_controller::PAK_RUMBLE:
			{
				controller->pak_data = malloc(sizeof(RUMBLEPAK));
				RUMBLEPAK *rPak = (RUMBLEPAK*)controller->pak_data;
				rPak->bPakType = _n64_controller::PAK_RUMBLE;
				rPak->fLastData = true;	
			}
			break;
		}

		return bReturn;
}

void GetButtons(pn64_controller controller)
{
	DWORD dwResult; 
	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));
	// Get the state
	dwResult = XInputGetState(controller->controller_number, &state);
	if( dwResult != ERROR_SUCCESS ){
		ZeroMemory(&state, sizeof(XINPUT_STATE));
	}else{
		SetXInputControllerDeadZone(&state.Gamepad);
		// from the N64 func ref: The 3D Stick data is of type signed char and in
		// the range between 80 and -80. (32768 / 409 = ~80.1)
		short LY = state.Gamepad.sThumbLY/409;
		short LX = state.Gamepad.sThumbLX/409;
		controller->Keys.X_AXIS = LY;
		controller->Keys.Y_AXIS = LX;
		short RY = state.Gamepad.sThumbRY/409;
		short RX = state.Gamepad.sThumbRX/409;
		controller->Keys.R_CBUTTON = (RX >= 40)?1 : 0;
		controller->Keys.L_CBUTTON = (RX <= -40)?1 : 0;
		controller->Keys.U_CBUTTON = (RY >= 40)?1 : 0; 
		controller->Keys.D_CBUTTON = (RY <= -40)?1 : 0;

		controller->Keys.D_DPAD = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)?1:0;
		controller->Keys.U_DPAD = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)?1:0;
		controller->Keys.L_DPAD = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)?1:0;
		controller->Keys.R_DPAD = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)?1:0;
		controller->Keys.A_BUTTON = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A)? 1: 0;
		controller->Keys.B_BUTTON = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X)? 1: 0;
		controller->Keys.START_BUTTON = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START)? 1: 0;
		controller->Keys.Z_TRIG =  (state.Gamepad.bLeftTrigger)? 1: 0;
		controller->Keys.L_TRIG = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 1: 0;
		controller->Keys.R_TRIG = (state.Gamepad.bRightTrigger ||state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 1: 0;
	}
}

void VibrateController( DWORD nController,  int LeftMotorVal = 65535, int RightMotorVal = 65535 )
{
	XINPUT_VIBRATION vibration;
	ZeroMemory( &vibration, sizeof( XINPUT_VIBRATION ) );
	vibration.wLeftMotorSpeed = LeftMotorVal;
	vibration.wLeftMotorSpeed = RightMotorVal;
	XInputSetState( nController, &vibration );
}

BYTE DataCRC( LPCBYTE Data, const int iLength )
{
	register BYTE Remainder = Data[0];
	int iByte = 1;
	BYTE bBit = 0;
	while( iByte <= iLength )
	{
		bool HighBit = ((Remainder & 0x80) != 0);
		Remainder = Remainder << 1;
		Remainder += ( iByte < iLength && Data[iByte] & (0x80 >> bBit )) ? 1 : 0;
		Remainder ^= (HighBit) ? 0x85 : 0;
		bBit++;
		iByte += bBit/8;
		bBit %= 8;
	}
	return Remainder;
}

BYTE ReadFromPak( const int iControl, LPBYTE Command )
{
	BYTE bReturn = RD_ERROR;
	if( !controller[iControl].pak_data )
		return RD_ERROR;
	LPBYTE Data = &Command[2];
	WORD dwAddress = (Command[0] << 8) + (Command[1] & 0xE0);
	switch( *(BYTE*)controller[iControl].pak_data )
	{
	case _n64_controller::PAK_CONTROLLER:
		{
			MEMPAK *mPak = (MEMPAK*)controller[iControl].pak_data;
			if( dwAddress < 0x8000 )
				CopyMemory( Data, &mPak->aMemPakData[dwAddress], 32 );
			else
				CopyMemory( Data, &mPak->aMemPakTemp[(dwAddress%0x100)], 32 );
			Data[32] = DataCRC( Data, 32 );
			bReturn = RD_OK;
		}
	break;
	case _n64_controller::PAK_RUMBLE:
		{
			//do rumble
			if(( dwAddress >= 0x8000 ) && ( dwAddress < 0x9000 ) ){
				RUMBLEPAK *rPak = (RUMBLEPAK*)controller[iControl].pak_data;
				if (rPak->fLastData)
					FillMemory( Data, 32, 0x80 );
				else
					ZeroMemory( Data, 32 );
				VibrateController( controller[iControl].controller_number, 0, 0);
			}
			else
				ZeroMemory( Data, 32 );
			Data[32] = DataCRC( Data, 32 );
			bReturn = RD_OK;
		}
	break;
	default:
		break;
	}
	return bReturn;
}

BYTE WriteToPak( const int iControl, LPBYTE Command )
{
	BYTE bReturn = RD_ERROR;
	BYTE *Data = &Command[2];

	if( !controller[iControl].pak_data )
		return RD_ERROR;

	WORD dwAddress = (Command[0] << 8) + (Command[1] & 0xE0);
	switch( *(BYTE*)controller[iControl].pak_data )
	{
	case _n64_controller::PAK_CONTROLLER:
		{
			// Switched to memory-mapped file
			// That way, if the computer dies due to power loss or something mid-play, the savegame is still there.
			MEMPAK *mPak = (MEMPAK*)controller[iControl].pak_data;
			if( dwAddress < 0x8000 )
			{
				CopyMemory( &mPak->aMemPakData[dwAddress], Data, 32 );
				if (!mPak->fReadonly )
				{
					if ( mPak && mPak->bPakType ==  _n64_controller::PAK_CONTROLLER && !mPak->fReadonly && mPak->hMemPakHandle != NULL )
					FlushViewOfFile( mPak->aMemPakData, PAK_MEM_SIZE );
				}	
			}
			else
				CopyMemory( &mPak->aMemPakTemp[(dwAddress%0x100)], Data, 32 );
			Data[32] = DataCRC( Data, 32 );
			bReturn = RD_OK;
		}
		break;
	case _n64_controller::PAK_RUMBLE:
		{
			if( dwAddress == 0xC000 )
			{
				if( *Data )
					VibrateController( controller[iControl].controller_number );
				else
					VibrateController(controller[iControl].controller_number, 0, 0 );
			}
			else if (dwAddress >= 0x8000 && dwAddress < 0x9000)
			{
				RUMBLEPAK *rPak = (RUMBLEPAK*)controller[iControl].pak_data;
				rPak->fLastData = (*Data) ? true : false;
			}
			Data[32] = DataCRC( Data, 32 );
			bReturn = RD_OK;

		}
		break;
	default:
		break;
	}
	return bReturn;
}


void FormatMemPak( LPBYTE aMemPak )
{
	FillMemory( aMemPak, 0x100, 0xFF );
	aMemPak[0] = 0x81;
	// generate a valid code( i hope i can calculate it one day)
	BYTE aValidCodes[] = {	0x12, 0xC5, 0x8F, 0x6F, 0xA4, 0x28, 0x5B, 0xCA };
	BYTE aCode[8];
	int iRand = (( (ULONG)aMemPak / 4 + (ULONG)GetForegroundWindow() % 4 ) % (sizeof(aValidCodes)/8) );
	for( int n = 0; n <8; n++ )
		aCode[n] = aValidCodes[n+iRand];
	//----------
	aMemPak[0x20+0] = aMemPak[0x60+0] = aMemPak[0x80+0] = aMemPak[0xC0+0] = 0xFF;
	aMemPak[0x20+1] = aMemPak[0x60+1] = aMemPak[0x80+1] = aMemPak[0xC0+1] = 0xFF;
	aMemPak[0x20+2] = aMemPak[0x60+2] = aMemPak[0x80+2] = aMemPak[0xC0+2] = 0xFF;
	aMemPak[0x20+3] = aMemPak[0x60+3] = aMemPak[0x80+3] = aMemPak[0xC0+3] = 0xFF;
	aMemPak[0x20+4] = aMemPak[0x60+4] = aMemPak[0x80+4] = aMemPak[0xC0+4] = aCode[0];
	aMemPak[0x20+5] = aMemPak[0x60+5] = aMemPak[0x80+5] = aMemPak[0xC0+5] = aCode[1];
	aMemPak[0x20+6] = aMemPak[0x60+6] = aMemPak[0x80+6] = aMemPak[0xC0+6] = aCode[2];
	aMemPak[0x20+7] = aMemPak[0x60+7] = aMemPak[0x80+7] = aMemPak[0xC0+7] = aCode[3];
	//aMemPak[0x30+9] = aMemPak[0x70+9] = aMemPak[0x90+9] = aMemPak[0xD0+9] = 0x01; // not sure
	aMemPak[0x30+10] = aMemPak[0x70+10] = aMemPak[0x90+10] = aMemPak[0xD0+10] = 0x01;
	aMemPak[0x30+12] = aMemPak[0x70+12] = aMemPak[0x90+12] = aMemPak[0xD0+12] = aCode[4];
	aMemPak[0x30+13] = aMemPak[0x70+13] = aMemPak[0x90+13] = aMemPak[0xD0+13] = aCode[5];
	aMemPak[0x30+14] = aMemPak[0x70+14] = aMemPak[0x90+14] = aMemPak[0xD0+14] = aCode[6];
	aMemPak[0x30+15] = aMemPak[0x70+15] = aMemPak[0x90+15] = aMemPak[0xD0+15] = aCode[7];
	// Index
	ZeroMemory( &aMemPak[0x100], 0x400 );
	aMemPak[0x100+1] = aMemPak[0x200+1] = 0x71;
	for( int i = 0x00b; i < 0x100; i += 2 )
		aMemPak[0x100+i] = aMemPak[0x200+i] = 03;
	FillMemory( &aMemPak[0x500], 0x7B00, 0xFF );
}
