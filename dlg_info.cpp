#include <windows.h>

#include "guids.h"
#include "resource_ids.h"
#include "dlg_info.h"

#include "parsers\file_parser.h"
#include "parsers\ac3\ac3_header.h"
#include "parsers\dts\dts_header.h"
#include "parsers\mpa\mpa_header.h"
#include "parsers\spdif\spdif_header.h"
#include "parsers\multi_header.h"


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
  const size_t buf_size = 1024;
  char buf[buf_size];

  FileParser file;
  const HeaderParser *parser_list[] = { &spdif_header, &ac3_header, &dts_header, &mpa_header };
  MultiHeader multi_parser(parser_list, array_size(parser_list));

  if (file.open(filename, &multi_parser, 1000000))
  {
    if (file.stats())
    {
      file.load_frame();
      size_t used = file.file_info(buf, buf_size);
      used += sprintf(buf + used, "\n");
      used += file.stream_info(buf + used, buf_size - used);
      cr2crlf(buf, buf_size);
    }
    else
      sprintf(buf, "Error: Cannot detect file format\n");
  }
  else
    sprintf(buf, "Error: Cannot open file '%s'\n", filename);
    

  SendDlgItemMessage(hwnd, IDC_EDT_FILENAME, WM_SETTEXT, 0, (LPARAM)filename);
  SendDlgItemMessage(hwnd, IDC_EDT_FILEINFO, WM_SETTEXT, 0, (LPARAM)buf);
}
