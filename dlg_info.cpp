#include <windows.h>

#include "guids.h"
#include "resource.h"
#include "dlg_info.h"

#include "parsers\file_parser.h"
#include "parsers\ac3\ac3_parser.h"
#include "parsers\dts\dts_parser.h"


///////////////////////////////////////////////////////////////////////////////
// Initialization
///////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK
InfoDlg::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  InfoDlg *dlg;
  if (uMsg == WM_INITDIALOG)
  {
    SetWindowLong(hwnd, DWL_USER, lParam);

    dlg = (InfoDlg *)lParam;
    if (!dlg) return TRUE;
    dlg->hwnd = hwnd;
  }

  dlg = (InfoDlg *)GetWindowLong(hwnd, DWL_USER);
  if (!dlg)
    return FALSE;

  return dlg->message(hwnd, uMsg, wParam, lParam);
}

InfoDlg::InfoDlg(HINSTANCE _hinstance, HWND _parent, const char *_filename) 
{
  hinstance = _hinstance;
  parent    = _parent;
  filename  =  strdup(_filename);
}

InfoDlg::~InfoDlg()
{
  if (filename)
    delete filename;
}


int
InfoDlg::run()
{
  return DialogBoxParam(hinstance, MAKEINTRESOURCE(IDD_FILEINFO), parent, DialogProc, (LPARAM)this);
}

///////////////////////////////////////////////////////////////////////////////
// Handle messages
///////////////////////////////////////////////////////////////////////////////

BOOL 
InfoDlg::message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{        
  switch (uMsg)
  {
    case WM_ACTIVATE:
      init_controls();
      return TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
        EndDialog(hwnd, LOWORD(wParam));
      return TRUE;
  }
  return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// Controls initalization/update
///////////////////////////////////////////////////////////////////////////////

void 
InfoDlg::init_controls()
{
  char buf[1024];
  FileParser file;
  AC3Parser ac3;
  DTSParser dts;

  SendDlgItemMessage(hwnd, IDC_EDT_FILENAME, WM_SETTEXT, 0, (LPARAM)filename);

  if (file.open(&ac3, filename) && file.probe())
  {
    file.stats();
    file.get_info(buf, sizeof(buf));
    cr2crlf(buf, sizeof(buf));
    SendDlgItemMessage(hwnd, IDC_EDT_INFO, WM_SETTEXT, 0, (LPARAM)buf);
  }
  else if (file.open(&dts, filename) && file.probe())
  {
    file.stats();
    file.get_info(buf, sizeof(buf));
    cr2crlf(buf, sizeof(buf));
    SendDlgItemMessage(hwnd, IDC_EDT_INFO, WM_SETTEXT, 0, (LPARAM)buf);
  }
  else
  {
    SendDlgItemMessage(hwnd, IDC_EDT_INFO, WM_SETTEXT, 0, (LPARAM)"Cannot determine file format");
  }
}
