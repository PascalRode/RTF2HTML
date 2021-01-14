// RTF2HTML.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "RTF2HTML.h"
#include "RTF2HTMLTree.h"
#include "RTF2HTMLConverter.h"




#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

void ProcessFile(const CString& strInFile, const CString& strOutFile)
{
	CStdioFile fileIn, fileOut;
	CFileException* pException = new CFileException;

	if (!fileIn.Open(strInFile, CFile::modeRead | CFile::shareDenyWrite | CFile::typeText, pException)) throw pException;
	if (!fileOut.Open(strOutFile, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive | CFile::typeText, pException)) throw pException;
	CString strIn, strOut;

	CString strLine;
	while (fileIn.ReadString(strLine)) {
		if (strIn != "") strIn += "\r\n";

		strLine.Replace("\\\\", "GUIXT_REPLACEBACKSLASH");
		strIn += strLine;
	}
	CRTF_HTMLConverter convert(CRTF_HTMLConverter::c_modRTF2HTML);
	convert.SetTitle("RTF2HTML Converter converted document");
	strIn >> convert;
	strOut << convert;

	strOut.Replace("GUIXT_REPLACEBACKSLASH", "\\");
	fileOut.WriteString(strOut);

	fileIn.Close();
	fileOut.Flush();
	fileOut.Close();
	if (pException) pException->Delete();
}

void ProcessClipboard()
{

	CString myText;
	UINT format = RegisterClipboardFormat(CF_RTF);

	if (OpenClipboard(NULL))
	{
		HANDLE hClipboardData = GetClipboardData(format);
		if (hClipboardData)
		{
			TCHAR* pchData = (TCHAR*)GlobalLock(hClipboardData);
			if (pchData)
			{
				myText = pchData;
				GlobalUnlock(hClipboardData);
			}
		}


		myText.Replace("\\\\", "GUIXT_REPLACEBACKSLASH");

		CRTF_HTMLConverter convert(CRTF_HTMLConverter::c_modRTF2HTML);
		convert.SetTitle("RTF2HTML Converter converted document");
		myText >> convert;
		myText << convert;

		myText.Replace("GUIXT_REPLACEBACKSLASH", "\\");

		EmptyClipboard();
		HGLOBAL clipbuffer;
		char* buffer;
		EmptyClipboard();

		clipbuffer = GlobalAlloc(GMEM_DDESHARE, myText.GetLength() + 1);
		buffer = (char*)GlobalLock(clipbuffer);
		strcpy(buffer, LPCSTR(myText));
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_TEXT, clipbuffer);
	

		CloseClipboard();
	}

}



int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) {
		// TODO: change error code to suit your needs
		fprintf(stdout, "Fatal Error: MFC initialization failed\n");

		nRetCode = 1;
	}
	else {
		// TODO: code your application's behavior here.		


		if (argc != 3) {
			// fprintf(stdout, "Usage: RTF2HTML <source - rtf - file> <target - html - file>\n");

			ProcessClipboard();

		}
		else {

			fprintf(stdout, "Converting from RTF to HTML ... \n");
			ProcessFile(argv[1], argv[2]);
			fprintf(stdout, "Done.  \n");
		}
	}

	return nRetCode;
}

