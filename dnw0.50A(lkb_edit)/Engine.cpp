// =============================================
// Program Name: engine
// =============================================

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#pragma hdrstop
#include <string.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h> //for _vsntprintf()
#include <shellapi.h> //ShellExecute

#include "resource.h"

#include "engine.h"
#include "dnw.h"
#include "fileopen.h"
#include "font.h"
#include "d_box.h"

#include <locale.h> //lkb
//#include "usbtxrx.h"

/////////////////////
// lkb add
////////////////////
#define _BUF_SIZE_LIMITED_  1 //lkb add

//end add

#pragma warning (disable : 4100)
#pragma warning (disable : 4068)

void myprintf(const char *format, ...);

TCHAR szAppName[] = APPNAME;

HINSTANCE _hInstance;
HWND _hwnd;
static HWND _hwndEdit;

//lkb mark
#define EDITID   1
#define MAX_EDIT_BUF_SIZE (0x7FFE) 
#define EDIT_BUF_SIZE (0x6000)   
#define EDIT_BUF_DEC_SIZE (0x2000)

//1) Why program doesn't work when EDIT_BUF_SIZE=50000? 
//   If the data size of the edit box is over about 30000, 
//   EM_REPLACESEL message needs very long time.
//   So,EDIT_BUF_SIZE is limited to 30000. 
//   If the size is bigger than 30000, the size will be decreased by 5000. 
//2) WM_CLEAR to the edit box doesn't work. only, EM_REPLACESEL message works.

// ==============================================
// INITIALIZATION
// ==============================================

//////////////////////////////////////
// The WinMain function is the program entry point.
//////////////////////////////////////
#pragma argsused
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance,
				   LPSTR  lpszCmdParam, int nCmdShow)
{
	MSG  Msg;
	_hInstance = hInst;

	//OutputDebugString("");
	//setlocale(LC_ALL," ");

	if (!Register(hInst))
		return FALSE;

	if (!Create(hInst, nCmdShow))
		return FALSE;

	while (GetMessage(&Msg, NULL, 0, 0))
	{
		if (Msg.message == WM_CONTEXTMENU)
		{
			OutputDebugString("WM_CONTEXTMENU--\n");
		}
		if( _hDlgDownloadProgress ==0 || !IsDialogMessage(_hDlgDownloadProgress,&Msg) ) 
			//To throw the message to dialog box procedure
		{
			TranslateMessage(&Msg);

			//To intercept key-board input instead of edit control.
			if(Msg.message==WM_CHAR) 
				SendMessage(_hwnd,WM_CHAR,Msg.wParam,Msg.lParam);
			else	//2000.1.26
				DispatchMessage(&Msg);
			//EB_Printf("."); //for debug
		}
	}
	return Msg.wParam;
}


//////////////////////////////////////
// Register Window
//////////////////////////////////////
BOOL Register(HINSTANCE hInst)
{
#if 0
	WNDCLASS WndClass;

	WndClass.style          = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc    = WndProc;
	WndClass.cbClsExtra     = 0;
	WndClass.cbWndExtra     = 0;
	WndClass.hInstance      = hInst;
	WndClass.hIcon          = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON1));
	WndClass.hCursor        = LoadCursor(NULL,IDC_ARROW);
	WndClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	WndClass.lpszMenuName   = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.lpszClassName  = szAppName;

	return (RegisterClass (&WndClass) != 0);
#else
	WNDCLASSEX WndClass;

	WndClass.cbSize	  = sizeof(WNDCLASSEX);
	WndClass.style          = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc    = WndProc;
	WndClass.cbClsExtra     = 0;
	WndClass.cbWndExtra     = 0;
	WndClass.hInstance      = hInst;
	WndClass.hIcon          = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON1));
	WndClass.hCursor        = LoadCursor(NULL,IDC_ARROW);
	WndClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	WndClass.lpszMenuName   = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.lpszClassName  = szAppName;
	WndClass.hIconSm	  = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON2));

	return (RegisterClassEx(&WndClass) != 0);

#endif
}

//////////////////////////////////////
// Create the window and show it.
//////////////////////////////////////
BOOL Create(HINSTANCE hInst, int nCmdShow)
{
	//lkb add
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	//end lkb add
	HWND hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPEDWINDOW /*&(~(WS_SIZEBOX|WS_MAXIMIZEBOX|WS_MINIMIZEBOX))*/,
		// CW_USEDEFAULT, CW_USEDEFAULT,
		screenWidth/3, screenHeight/4,
		WINDOW_XSIZE,WINDOW_YSIZE,
		NULL, NULL, hInst, NULL);

	if (hwnd == NULL)
		return FALSE;

	//_hwnd=hwnd; 

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	return TRUE;
}

//==============================================
// IMPLEMENTATION
//==============================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message,  //菜单栏命令处理
						 WPARAM wParam, LPARAM lParam)
{
	//static HINSTANCE hInst;
	HMENU hmenu;
	int xPos,yPos;
	int iTextLen=0;
	char *pCharBuf = NULL;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwWritten;

	switch(Message)
	{
	case WM_CREATE:
		//PlaySound(TEXT("d:\\windows\\media\\chimes.wav"),NULL,SND_FILENAME|SND_ASYNC);
		_hwnd=hwnd;

		PopFileInitialize(hwnd);

		SetTimer(hwnd,1,1000,NULL);


		// Create the edit control child window
		_hwndEdit = CreateWindow (TEXT("edit"), NULL,
			WS_CHILD | WS_VISIBLE | /*WS_HSCROLL |*/ WS_VSCROLL |// WS_POPUP|
			WS_BORDER | ES_LEFT | ES_MULTILINE | ES_READONLY |
			/*ES_NOHIDESEL|*/ ES_AUTOVSCROLL /*|ES_AUTOHSCROLL*/ , 
			0, 0,SCREEN_X,SCREEN_Y,//SCREEN_Y,
			hwnd, (HMENU)EDITID, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

		SetFont(_hwndEdit);
#if _BUF_SIZE_LIMITED_
		SendMessage(_hwndEdit, EM_SETLIMITTEXT, MAX_EDIT_BUF_SIZE, 0L);
#endif

		ReadUserConfig(); //To get serial settings

		return 0; 
	case WM_COMMAND:
		if(lParam==0) //is menu command?
		{
			switch(LOWORD(wParam))
			{
			case CM_CONNECT:
			case ID_CONNECT_M:
				MenuConnect(hwnd);
				break;
			case CM_DISCONNECT: //lkb add
			case ID_DISCONNECT_M:
				MenuDisconnect();
				break;
			case CM_TRANSMIT:
				MenuTransmit(hwnd);
				break;
			case CM_OPTIONS:
				MenuOptions(hwnd);
				break;

			case  ID_CLEARSCREEN: //lkb add
				OutputDebugString("ID_CLEARSCREEN\n");
				SetWindowText(_hwndEdit, "");
				//EB_Printf("您好\n");
				break;

			case  ID_EDITOR: //lkb add, use notepad.exe to open it
				OutputDebugString("ID_EDITOR\n");
				iTextLen = SendMessage(_hwndEdit,WM_GETTEXTLENGTH,0x0,0x0);
				////myprintf("============ txt len %d\n", iTextLen);
				pCharBuf = new char[iTextLen];
				GetWindowText(_hwndEdit, pCharBuf, iTextLen);
				////myprintf("+++ %s\n", pCharBuf);
				hFile = CreateFile(TEXT("temp_to_editor.txt"), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
				if(hFile != INVALID_HANDLE_VALUE)
				{					
					if (WriteFile(hFile, pCharBuf, iTextLen, &dwWritten, NULL))
					{
						//ShellExecute(NULL, "open", "temp_to_editor.txt", NULL, NULL, SW_SHOWNORMAL); //use default program to open it
						WinExec( "notepad.exe temp_to_editor.txt", SW_SHOW ); //use notepad.exe to open it
					}
					else
					{
						EB_Printf("WriteFile txt file failed !!\n");
					}
				}
				else
				{
					EB_Printf("Create editor file Failed!!\n");
				}
				CloseHandle(hFile);
				delete[]pCharBuf;
				break;

				//case CM_USBTRANSMIT:
				//MenuUsbTransmit(hwnd);
				//break;
				// case CM_USBRECEIVE:
				//MenuUsbReceive(hwnd);
				//break;      
				//case CM_USBSTATUS:
				//MenuUsbStatus(hwnd);
				//break;
			case CM_ABOUT:
				MenuAbout(hwnd);
				break;
			}
			return 0;
		}
		else
			return 0;
		break;
	case WM_CONTEXTMENU: //lkb??? add //message notifies a window that the user clicked the right mouse button (right-clicked) in the window. //窗口选项为WS_POPUP时才有此消息？
		OutputDebugString("WM_CONTEXTMENU\n");
		hmenu = LoadMenu(_hInstance, MAKEINTRESOURCE(IDR_RIGHT_MENU));
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);
		TrackPopupMenu(hmenu,TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, xPos,yPos,0,_hwndEdit, NULL );

		break;

	case WM_LBUTTONDOWN:
		OutputDebugString("WM_LBUTTONDOWN\n");
		break;


		// WM_PAINT should be processed on DefWindowProc(). 
		// If there is WM_PAINT, there should be UpdateWindow(_hwndEdit). 
		/*
		case WM_PAINT:
		UpdateWindow(_hwndEdit); 
		return 0;
		*/
	case WM_DESTROY:
		KillTimer(hwnd,1);
		Quit(hwnd);
		ReleaseFont();
		PostQuitMessage(0);
		return 0;

	case WM_TIMER:
		switch(wParam)
		{
		case 1:
			UpdateWindowTitle();
			break;
		default:
			break;
		}
		return 0;

	case WM_SIZE : 
		MoveWindow (_hwndEdit, 0, 0, LOWORD (lParam), HIWORD (lParam), TRUE);

		return 0 ;
		//If the size of the parent window is not adjustable,
		//WM_SIZE message is not received.
		//If the size of the parent window is adjustable,
		//WM_SIZE message is received although window size is not changed.
		//MoveWindow(,,,,,TRUE) will make WM_PAINT.

		//If DefWindowProc() doesn't process WM_PAINT and WM_PAINT is processed by user, 
		//UpdateWindow(_hwndEdit) is should be executed inide WM_PAINT case,
		//which update the child window.

	case WM_CHAR:
		WriteCommBlock((char)wParam);    //UNICODE problem!!!
		return 0;

		/*	
		case WM_CTLCOLOR: //for changing FONT & COLOR
		if ( HIWORD(lParam) == CTLCOLOR_EDIT ) {
		// Set the text background color.
		SetBkColor(wParam, RGB(128,128,128));
		// Set the text foreground color.
		SetTextColor(wParam, RGB(255, 255, 255) );
		// Return the control background brush.
		return GetStockObject(LTGRAY_BRUSH);
		}
		else
		return GetStockObject(WHITE_BRUSH);
		*/
	}
	return DefWindowProc(hwnd, Message, wParam, lParam); 
}

//8: \b
//13: \r

// 1)about EM_SETSEL message arguments
//   start_position,end_position
//   The selected character is from start_position to end_position-1;
//   So, We should do +1 to the position of last selected character.
// 2)The char index start from 0.
// 3)WM_CLEAR,WM_CUT doesn't operate on EM_SETSEL chars.

#define STRING_LEN  4096  
volatile static int isUsed=0;

void EB_Printf(TCHAR *fmt,...) 
{
	va_list ap;
	int i,slen,lineIdx;
	int txtRepStart,txtRepEnd,txtSelEnd;
	static int wasCr=0; //should be static type.
	static TCHAR string[STRING_LEN]; //margin for '\b'
	static TCHAR string2[STRING_LEN]; //margin for '\n'->'\r\n'
	static int prevBSCnt=0;
	int str2Pt=0;


	while(isUsed==1); //EB_Printf can be called multiplely  //KIW

	txtRepStart=SendMessage(_hwndEdit,WM_GETTEXTLENGTH,0x0,0x0);
	txtRepEnd=txtRepStart-1;

	va_start(ap,fmt);
	_vsntprintf(string2,STRING_LEN-1,fmt,ap);
	va_end(ap);

	string2[STRING_LEN-1]='\0';

	//for better look of BS(backspace) char.,
	//the BS in the end of the string will be processed next time.
	for(i=0;i<prevBSCnt;i++) //process the previous BS char.
		string[i]='\b';
	string[prevBSCnt]='\0';
	lstrcat(string,string2);
	string2[0]='\0';

	slen=lstrlen(string);
	for(i=0;i<slen;i++)
		if(string[slen-i-1]!='\b')break;

	prevBSCnt=i; // These BSs will be processed next time.
	slen=slen-prevBSCnt;

	if(slen==0)
	{
		isUsed=0;
		return;
	}

	for(i=0;i<slen;i++)
	{
		if( (string[i]=='\n'))
		{
			string2[str2Pt++]='\r';txtRepEnd++;
			string2[str2Pt++]='\n';txtRepEnd++;
			wasCr=0;
			continue;
		}
		if( (string[i]!='\n') && (wasCr==1) )
		{
			string2[str2Pt++]='\r';txtRepEnd++;
			string2[str2Pt++]='\n';txtRepEnd++;
			wasCr=0;
		}
		if(string[i]=='\r')
		{
			wasCr=1;
			continue;
		}

		if(string[i]=='\b')
		{
			//flush string2
			if(str2Pt>0)
			{
				string2[--str2Pt]='\0';
				txtRepEnd--;
				continue;
			}
			//str2Pt==0;	    
			if(txtRepStart>0)
			{
				txtRepStart--;		
			}
			continue;
		}
		string2[str2Pt++]=string[i];
		txtRepEnd++;
		// if(str2Pt>256-3)break; //why needed? 2001.1.26
	}

	string2[str2Pt]='\0';    
	if(str2Pt>0)
	{
		SendMessage(_hwndEdit,EM_SETSEL,txtRepStart,txtRepEnd+1);
		SendMessage(_hwndEdit,EM_REPLACESEL,0,(LPARAM)string2);     
	}
	else
	{
		if(txtRepStart<=txtRepEnd)
		{
			SendMessage(_hwndEdit,EM_SETSEL,txtRepStart,txtRepEnd+1);
			SendMessage(_hwndEdit,EM_REPLACESEL,0,(LPARAM)"");         
		}
	}

#if _BUF_SIZE_LIMITED_
	//If edit buffer is over EDIT_BUF_SIZE,
	//the size of buffer must be decreased by EDIT_BUF_DEC_SIZE.
	if(txtRepEnd>EDIT_BUF_SIZE)
	{
		lineIdx=SendMessage(_hwndEdit,EM_LINEFROMCHAR,EDIT_BUF_DEC_SIZE,0x0);
		//lineIdx=SendMessage(_hwndEdit,EM_LINEFROMCHAR,txtRepEnd-txtRepStart+1,0x0); //for debug
		txtSelEnd=SendMessage(_hwndEdit,EM_LINEINDEX,lineIdx,0x0)-1;
		SendMessage(_hwndEdit,EM_SETSEL,0,txtSelEnd+1);

		SendMessage(_hwndEdit,EM_REPLACESEL,0,(LPARAM)"");
		//SendMessage(_hwndEdit,WM_CLEAR,0,0); //WM_CLEAR doesn't work? Why?
		//SendMessage(_hwndEdit,WM_CUT,0,0); //WM_CUT doesn't work? Why?

		//make the end of the text shown.
		txtRepEnd=SendMessage(_hwndEdit,WM_GETTEXTLENGTH,0x0,0x0)-1;
		SendMessage(_hwndEdit,EM_SETSEL,txtRepEnd+1,txtRepEnd+2); 
		SendMessage(_hwndEdit,EM_REPLACESEL,0,(LPARAM)" ");
		SendMessage(_hwndEdit,EM_SETSEL,txtRepEnd+1,txtRepEnd+2); 
		SendMessage(_hwndEdit,EM_REPLACESEL,0,(LPARAM)"");
	}
#endif

	isUsed=0;
}

#define DBG_STRING_BUFFER_SIZE 4096
VOID myprintf(LPCTSTR lpOutputString,...)
{

	TCHAR szData[DBG_STRING_BUFFER_SIZE]={0};
	va_list args;
	va_start(args, lpOutputString);
	_vsntprintf_s(szData,_countof(szData),_countof(szData)-1, lpOutputString, args);
	va_end(args);
	::OutputDebugString(szData);
}



