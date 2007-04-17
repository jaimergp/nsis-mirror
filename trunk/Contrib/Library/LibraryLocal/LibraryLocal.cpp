/*

  LibraryLocal - used by the Library.nsh macros
  Get the version of local DLL and TLB files
  Written by Joost Verburg

*/

#include "../../../Source/Platform.h"

#include <stdio.h>
#include <iostream>
#include <fstream>

#include "../../../Source/ResourceEditor.h"
#include "../../../Source/ResourceVersionInfo.h"
#include "../../../Source/winchar.h"

using namespace std;

int g_noconfig=0;
int g_display_errors=1;
FILE *g_output=stdout;

int GetDLLVersion(string& filepath, DWORD& high, DWORD & low)
{
  int found = 0;

  FILE *fdll = fopen(filepath.c_str(), "rb");
  if (!fdll)
    return 0;

  fseek(fdll, 0, SEEK_END);
  unsigned int len = ftell(fdll);
  fseek(fdll, 0, SEEK_SET);

  LPBYTE dll = (LPBYTE) malloc(len);

  if (!dll)
  {
    fclose(fdll);
    return 0;
  }

  if (fread(dll, 1, len, fdll) != len)
  {
    fclose(fdll);
    free(dll);
    return 0;
  }

  try
  {
    CResourceEditor *dllre = new CResourceEditor(dll, len);
    LPBYTE ver = dllre->GetResourceA(VS_FILE_INFO, MAKEINTRESOURCE(VS_VERSION_INFO), 0);
    int versize = dllre->GetResourceSizeA(VS_FILE_INFO, MAKEINTRESOURCE(VS_VERSION_INFO), 0);

    if (ver)
    {
      if ((size_t) versize > sizeof(WORD) * 3)
      {
        // get VS_FIXEDFILEINFO from VS_VERSIONINFO
        WCHAR *szKey = (WCHAR *)(ver + sizeof(WORD) * 3);
        int len = (winchar_strlen(szKey) + 1) * sizeof(WCHAR) + sizeof(WORD) * 3;
        len = (len + 3) & ~3; // align on DWORD boundry
        VS_FIXEDFILEINFO *verinfo = (VS_FIXEDFILEINFO *)(ver + len);
        if (versize > len && verinfo->dwSignature == VS_FFI_SIGNATURE)
        {
          low = verinfo->dwFileVersionLS;
          high = verinfo->dwFileVersionMS;
          found = 1;
        }
      }
      dllre->FreeResource(ver);
    }

    delete dllre;
  }
  catch (exception&)
  {
    return 0;
  }

  return found;
}

int GetTLBVersion(string& filepath, DWORD& high, DWORD & low)
{
#ifdef _WIN32

  int found = 0;

  char fullpath[1024];
  char *p;
  if (!GetFullPathName(filepath.c_str(), sizeof(fullpath), fullpath, &p))
    return 0;

  wchar_t ole_filename[1024];
  MultiByteToWideChar(CP_ACP, 0, fullpath, lstrlen(fullpath) + 1, ole_filename, 1024);
  
  ITypeLib* typeLib;
  HRESULT hr;
  
  hr = LoadTypeLib(ole_filename, &typeLib);
  
  if (SUCCEEDED(hr)) {

    TLIBATTR* typelibAttr;
    
    hr = typeLib->GetLibAttr(&typelibAttr);

    if (SUCCEEDED(hr)) {
      
      high = typelibAttr->wMajorVerNum;
      low = typelibAttr->wMinorVerNum;
      
      found = 1;

    }

    typeLib->Release();

  }

  return found;

#else

  return 0;

#endif
}

int main(int argc, char* argv[])
{

  // Parse the command line

  string cmdline;

  string mode;
  string filename;
  string filepath;

  int filefound = 0;

  if (argc != 4)
    return 1;

  // Get the full path of the local file

  mode = argv[1];
  filename = argv[2];

  // Validate filename

  ifstream fs(filename.c_str());
  
  if (fs.is_open())
  {
    filefound = 1;
    fs.close();
  }

  // Work
  
  int versionfound = 0;
  DWORD low = 0, high = 0;

  if (filefound)
  {

    // Get version
    
    // DLL
    
    if (mode.compare("D") == 0)
    {
      
      versionfound = GetDLLVersion(filename, high, low);

    }

    // TLB
    
    if (mode.compare("T") == 0)
    {
      
      versionfound = GetTLBVersion(filename, high, low);

    }

  }

  // Write the version to an NSIS header file

  ofstream header(argv[3], ofstream::out);
  
  if (header)
  {

    if (!filefound)
    {
      header << "!define LIBRARY_VERSION_FILENOTFOUND" << endl;
    }
    else if (!versionfound)
    {
      header << "!define LIBRARY_VERSION_NONE" << endl;
    }
    else
    {
      header << "!define LIBRARY_VERSION_HIGH " << high << endl;
      header << "!define LIBRARY_VERSION_LOW " << low << endl;
    }
    
    header.close();

  }

  return 0;

}