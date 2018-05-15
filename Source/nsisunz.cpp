// Unicode NSIS support by Gringoloco023, http://portableapps.com/node/21879, Februari 6 2010
// Improvements by past-due, https://github.com/past-due/, 2018+
/*
UnZip Plug-in for NSIS
Written by Saivert

Credits:
  - Based on code in NSIS Zip2Exe
    portions Copyright � 1999-2001 Miguel Garrido (mgarrido01@hotmail.com)
  - Uses ZLIB - Copyright � Mark Adler
  - ZIP format routines - Copyright (C) 1998 Gilles Vollant

  Even though this project is an NSIS Plug-in, it also
  exports a function that can be used by any applications.
  The exported function is "appextract"
*/
//#include "AggressiveOptimize.h"
#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include "nsis/pluginapi.h"

extern "C" {
#include "minizip/unzip.h"
};

//Strings used all over the place
TCHAR szSuccess[]        = {_T('s'),_T('u'),_T('c'),_T('c'),_T('e'),_T('s'),_T('s'),_T('\0')};
TCHAR szFile[]           = {_T('/'),_T('f'),_T('i'),_T('l'),_T('e'),_T('\0')};
TCHAR szNoextractpath[]  = {_T('/'),_T('n'),_T('o'),_T('e'),_T('x'),_T('t'),_T('r'),_T('a'),_T('c'),_T('t'),_T('p'),_T('a'),_T('t'),_T('h'),_T('\0')};
TCHAR szDefExtractText[] = {_T('E'),_T('x'),_T('t'),_T('r'),_T('a'),_T('c'),_T('t'),_T(':'),_T(' '),_T('%'),_T('f'),_T('\0')};

TCHAR g_extract_text[1024];

int g_extracting;
HWND g_hwndParent;
TCHAR tempzip_path[1024];

void internal_unzip(int);

CRITICAL_SECTION cs;

//Log stuff
HWND g_hwndList;
HWND g_hwndStatus;
void LogMessage(HWND, const TCHAR *, int = 0);

#ifdef UNICODE
	char * _T2A(wchar_t *str);
	wchar_t * _A2T(char *str);
	#define _tfopen_s _wfopen_s
#else
	#define _T2A(x) (x)
	#define _A2T(x) (x)
	#define _tfopen_s fopen_s
#endif

extern "C" BOOL WINAPI DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	//case DLL_THREAD_ATTACH:
	case DLL_PROCESS_ATTACH:
		InitializeCriticalSection(&cs);
		break;
	//case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		DeleteCriticalSection(&cs);
		break;
	}
	return TRUE;
}

void doMKDir(TCHAR *directory)
{
	TCHAR *p, *p2;
	TCHAR buf[MAX_PATH];
	if (!*directory) return;
	lstrcpy(buf,directory);
	p=buf; while (*p) p++;
	while (p >= buf && *p != _T('\\')) p--;
	p2 = buf;
	if (p2[1] == _T(':')) p2+=4;
	else if (p2[0] == _T('\\') && p2[1] == _T('\\'))
	{
		p2+=2;
		while (*p2 && *p2 != _T('\\')) p2++;
		if (*p2) p2++;
		while (*p2 && *p2 != _T('\\')) p2++;
		if (*p2) p2++;
	}
	if (p >= p2)
	{
		*p=0;
		doMKDir(buf);
	}
	CreateDirectory(directory,NULL);
}

//Based on inttosizestr from NSIS exehead project.
//Creates a string representing the size of a file,
//choosing the unit that's best suited.
static TCHAR * FormatDiskSize(TCHAR *str, unsigned long b)
{
	TCHAR scale=_T('k');
	TCHAR sh=30;
	TCHAR s=0;

	if      (b <= (1024*1024)     ) sh=10, scale=_T('k');
	else if (b <= (1024*1024*1024)) sh=20, scale=_T('M');
	else if (GetVersion()&0x80000000) s=_T('+'); //only display '+' on GB shown on Win95

	if (b >= 1000) //represent everything above 1000 bytes as 0,xx kB
	{
		wsprintf(str, _T("%d.%d%d %cB%c"),
			b>>sh, ((b*10)>>sh)%10, ((b*20)>>sh)%10, scale, s);
		if (b < 1024)
			wsprintf(str+_tcsclen(str), _T(" (%u byte%c)"), b, b == 1?0:_T('s'));
	}
	else
		wsprintf(str, _T("%u byte%c"), b, b == 1?0:_T('s'));

	return str;
}

/* A simple format parser
 * Handles %f, %c and %u
 * size of out must be twice as large as size of str
 */
static TCHAR * parse(const TCHAR *str, TCHAR *out, int outlen,
					TCHAR *filename,
					unsigned long compressed,
					unsigned long uncompressed,
					unsigned int nkb)
{
	TCHAR s[32];
	const TCHAR *p;
	UINT i=0;
	p=str-1;
	memset(out, 0, outlen);

	while (p++ && (*p != 0))
	{
		if (*p == _T('%'))
		{
			switch (*(p+1))
			{
			case _T('c'):
			case _T('C'):
			{
				FormatDiskSize(s, compressed);
				lstrcat(out, s);
				i += lstrlen(s);
				break;
			}
			case _T('u'):
			case _T('U'):
			{
				FormatDiskSize(s, uncompressed);
				lstrcat(out, s);
				i += lstrlen(s);
				break;
			}
			case _T('f'):
			case _T('F'):
			{
				lstrcat(out, filename);
				i += lstrlen(filename);
				break;
			}
			case _T('p'):
			case _T('P'):
			{
				unsigned int j;
				j = uncompressed;
				if (!j) j = 1;
				wsprintf(s, _T("%d%%"), nkb*100/j);
				lstrcat(out, s);
				i += lstrlen(s);
				break;
			}
			case _T('b'):
			case _T('B'):
			{
				FormatDiskSize(s, nkb);
				lstrcat(out, s);
				i += lstrlen(s);
				break;
			}
			default: out[i++] = *p;
			}
			++p;
		} else {
			out[i++] = *p;
		}
	}
	return out;
}

extern "C" __declspec(dllexport)
void Unzip(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop)
{
	EXDLL_INIT();
	g_hwndParent = hwndParent;
	internal_unzip(0);
}

extern "C" __declspec(dllexport)
void UnzipToLog(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop)
{
	EXDLL_INIT();
	g_hwndParent = hwndParent;
	internal_unzip(1);
}

extern "C" __declspec(dllexport)
void UnzipToStack(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop)
{
	EXDLL_INIT();
	g_hwndParent = hwndParent;
	internal_unzip(2);
}

// This function is to be used by rundll32.exe. Call it like this:
//
//   rundll32.exe "c:\a path\nsisunz.dll",extract_RunDLL c:\path\zipfile.zip c:\outdir
//
// or to extract a single file, use this (concatenate the next two lines):
//
//   rundll32.exe c:\path\nsisunz.dll,extract_RunDLL
//     /file readme.txt "c:\a path\zipfile.zip" c:\outdir
//
// "/noextractpath" is implicitly used if "/file" is used.
// It mimics NSIS itself by setting up a stack and pushing the
// parameters from the function on the stack. I made it like this
// so I could directly reuse the code I already had written without modifications.
extern "C" __declspec(dllexport)
void extract_RunDLL(HINSTANCE hAppInstance, LPVOID unused, TCHAR *params)
{
	TCHAR res[1024];
	TCHAR temp[256];
	TCHAR zipfile[MAX_PATH]={0,};
	TCHAR destdir[MAX_PATH]={0,};
	TCHAR file[MAX_PATH]={0,};
	TCHAR *p = params;
	TCHAR *s;
	TCHAR q=0;
	int next=0;
#define NEXT_FILE 1
#define NEXT_ZIP  2
#define NEXT_DEST 3

	stack_t ownstacktop;
	stack_t *pownstacktop = &ownstacktop;

	ownstacktop.next = NULL;
	ownstacktop.text[0] = NULL;

	g_stringsize=1024;
	g_stacktop=&pownstacktop;
	g_variables=NULL;

	//Parse the parameters
	//While we do the parsing we use next as a clue to
	//what comes next. This way I don't have to write a
	//new function.
	while (*p)
	{
		if (*p==_T('"')) q=_T('"'), p++; else q=_T(' '); //Do we search for a quote or a space?
		s = temp;
		while (*p && *p != q) *s++ = *p++; //Copy chars from params to zipfile
		*s = 0; //Terminate string
		if (next==NEXT_FILE) lstrcpy(file, temp), next=NEXT_ZIP;
		else if (next==NEXT_ZIP) lstrcpy(zipfile, temp), next=NEXT_DEST;
		else if (next==NEXT_DEST) lstrcpy(destdir, temp), next=0;
		else if (!lstrcmpi(temp, szFile)) next=NEXT_FILE;
		else lstrcpy(zipfile, temp), next=NEXT_DEST;

		while (*p && *++p == _T(' '));
	}

	pushstring(destdir);
	pushstring(zipfile);

	if (file[0])
	{
		pushstring(file);
		pushstring(szFile);
		pushstring(szNoextractpath);
	}
	
	internal_unzip(0);
	popstring(res);
	if (lstrcmp(res, szSuccess))
		MessageBox(0, res, NULL, MB_ICONERROR);
}

void internal_unzip(int uselog)
{
	//All char baby (or should I use int for the non-string stuff??)
	TCHAR first=0;
	TCHAR filefound=0;
	TCHAR usefile=0;
	TCHAR hastext=0;
	TCHAR noextractpath=0;
	TCHAR filetoextract[MAX_PATH+1];
	TCHAR buf[1024];
	TCHAR fn[MAX_PATH+1];

	popstring(buf);
	while (buf[0] == _T('/'))
	{
		if (!lstrcmpi(buf+1, _T("text"))) popstring(g_extract_text), hastext++;
		if (!lstrcmpi(buf+1, _T("noextractpath"))) noextractpath++;
		if (!lstrcmpi(buf+1, _T("file")))
		{
			TCHAR *p;
			popstring(filetoextract);
			// Ensure filename uses backslashes
			p = filetoextract;
			while (*p)
			{
				if (*p == _T('/')) *p=_T('\\');
				p++;
			}
			usefile++;
		}

		//if stack is empty, bail out
		if (popstring(buf))
			*buf = 0;
	}
	//check for first required param
    if (*buf)
      lstrcpyn(fn, buf, MAX_PATH);
    else
    {
		pushstring(_T("Error reading ZIP filename parameter"));
		return;
    }

	if (popstring(tempzip_path))
	{
		pushstring(_T("Error reading destination directory parameter"));
		return;
	}

	if (uselog==1)
	{
		if (!hastext) lstrcpy(g_extract_text, szDefExtractText);
	}

	unzFile f;
	f = unzOpen(_T2A(fn));
	if (!f || unzGoToFirstFile(f) != UNZ_OK)
	{
		if (f) unzClose(f);
		pushstring(_T("Error opening ZIP file"));
		return;
	}

	int nf=0, nb=0;
	unz_file_info fileinfo;
	g_extracting=1;
	do {
		char filenameA[MAX_PATH];
		TCHAR filename[MAX_PATH];
		unzGetCurrentFileInfo(f,&fileinfo,filenameA,sizeof(filename),NULL,0,NULL,0);
		lstrcpyn(filename, _A2T(filenameA), MAX_PATH);

		if (filename[0] && 
			filename[_tcsclen(filename)-1] != _T('\\') && 
			filename[_tcsclen(filename)-1] != _T('/'))
		{
			TCHAR *pfn=filename;
			//ensure path uses backslashes
			while (*pfn)
			{
				if (*pfn == _T('/')) *pfn=_T('\\');
				pfn++;
			}

			if (usefile)
			{
				if (lstrcmpi(filename, filetoextract) != 0) continue;
				else filefound++;
			}
			
			pfn=filename;
			if (pfn[1] == _T(':') && pfn[2] == _T('\\')) pfn+=3;
			while (*pfn == _T('\\')) pfn++;

			if (noextractpath)
			{
				TCHAR buf[MAX_PATH];
				lstrcpy(buf,filename);
				TCHAR *p=buf+_tcsclen(buf);
				while (p > buf && *p != _T('\\') && *p != _T('/')) p = CharPrev(buf, p);
				if (p > buf) p++;
				lstrcpy(filename, p);
			}

			TCHAR out_filename[1024];
			lstrcpy(out_filename,tempzip_path);
			lstrcat(out_filename,_T("\\"));
			lstrcat(out_filename,pfn);
			if (_tcsstr(pfn,_T("\\")))
			{
				TCHAR buf[1024];
				lstrcpy(buf,out_filename);
				TCHAR *p=buf+_tcsclen(buf);
				while (p > buf && *p != _T('\\')) p--;
				*p=0;
				if (buf[0]) doMKDir(buf);
			}

			if (unzOpenCurrentFile(f) == UNZ_OK)
			{
				FILE *fp;
				int l;
				fp = _tfopen(out_filename,_T("wb"));
				if (fp)
				{
					if (uselog==1) {
						TCHAR logtmp[256];
						parse(g_extract_text, logtmp, sizeof(logtmp),
							pfn, fileinfo.compressed_size, fileinfo.uncompressed_size, 0);
						LogMessage(g_hwndParent, logtmp);
					} else if (uselog == 2) {
						if (!first)
						{
							pushstring(_T("")); //push list terminator (empty string)
							first++;
						}
						pushstring(pfn);
					}
					nb=0;
					do
					{
						TCHAR buf[1024];
						l=unzReadCurrentFile(f,buf,sizeof(buf));
						if (l > 0) 
						{
							if (fwrite(buf,1,l,fp) != (unsigned int)l)
							{
								unzClose(f);
								fclose(fp);
								pushstring(_T("Error writing output file(s)"));
								g_extracting=0;
								return;
							}

							if (!g_extracting)
							{
								unzClose(f);
								fclose(fp);
								g_extracting=0;
								pushstring(_T("aborted"));
								return;
							}
						}

						if (uselog==1 && (nb % 1024))
						{
							TCHAR logtmp[256];
							parse(g_extract_text, logtmp, sizeof(logtmp),
								pfn, fileinfo.compressed_size,
								fileinfo.uncompressed_size, nb);
							LogMessage(g_hwndParent, logtmp, 1);
						}
						nb += l;
					} while (l > 0);

					fclose(fp);
				}
				else
				{
					unzClose(f);
					pushstring(_T("Error opening output file(s)"));
					g_extracting=0;
					return;
				}
				nf++;

				int quit=0;
				if (g_hwndParent)
				{
					MSG msg;				
					while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
					{
						if (msg.message == WM_DESTROY && msg.hwnd == g_hwndParent) 
						{
							quit++;
							break;
						}
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
				unzCloseCurrentFile(f);
				if (quit) break;
			}
			else
			{
				unzClose(f);
				pushstring(_T("Error extracting from ZIP file"));
				g_extracting=0;
				return;
			}
		}
	} while (unzGoToNextFile(f) == UNZ_OK);

	g_extracting=0;
	if (usefile && !filefound) {
		pushstring(_T("File not found in archive"));
	} else {
		pushstring(szSuccess);
	}
	unzClose(f);
	return;
}

// Tim Kosse's LogMessage
void LogMessage(HWND hwndParent, const TCHAR *pStr, int changelast) {
	static HWND hwndList=0;
	static HWND hwndStatus=0;
	LVITEM item={0};
	int nItemCount;
	if (!hwndParent) return;

	EnterCriticalSection(&cs);

	//Get ListView control on instfiles page (the log)
	if (!hwndList) hwndList = FindWindowEx(
		FindWindowEx(hwndParent, NULL, _T("#32770"), NULL),
		NULL, _T("SysListView32"), NULL);

	//Get status "STATIC" control above progressbar
	if (!hwndStatus) hwndStatus = GetDlgItem(
		FindWindowEx(hwndParent, NULL, _T("#32770"), NULL),
		1006);
	
	if (!hwndList || !hwndStatus) return;

	SendMessage(hwndStatus, WM_SETTEXT, 0, (LPARAM)pStr);//added by Saivert
	nItemCount=SendMessage(hwndList, LVM_GETITEMCOUNT, 0, 0);

	item.mask=LVIF_TEXT;
	item.pszText=(TCHAR *)pStr;
	item.cchTextMax=0;
	item.iItem=changelast?nItemCount-1:nItemCount;

	if (changelast) {
		ListView_SetItem(hwndList, &item);
	} else {
		ListView_InsertItem(hwndList, &item);
	}
	ListView_EnsureVisible(hwndList, item.iItem, 0);

	LeaveCriticalSection(&cs);
}

// EXPERIMENTAL THREAD BASED EXTRACTION
/*
extern "C" __declspec(dllexport)
UnzipToLogUsingThread(HWND hwndParent, int string_size, char *variables, stack_t **stacktop)
{
	MSG msg;
	DWORD dwTID;

	CreateThread(NULL, 0, UnzipThread, (LPVOID)1, 0, &dwTID);
}

DWORD WINAPI UnzipThread(LPVOID p)
{
	int uselog = 1;
	return 0;
}
*/
#ifdef UNICODE
unsigned char staticCnvBuffer[1024*sizeof(TCHAR)]; /* temp buffer, holds ASCII & UNICODE string after conversion */
char * _T2A(wchar_t *wideStr)
{
	WideCharToMultiByte(CP_ACP, 0, wideStr, -1, (LPSTR)staticCnvBuffer, sizeof(staticCnvBuffer), NULL, NULL);
	return (char *)staticCnvBuffer;
}
wchar_t * _A2T(char *ansiStr)
{
	MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, (TCHAR *)staticCnvBuffer, sizeof(staticCnvBuffer)/sizeof(TCHAR));
	return (wchar_t *)staticCnvBuffer;
}
#endif
