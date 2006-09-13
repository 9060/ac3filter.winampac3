#ifndef DLG_INFO_H
#define DLG_INFO_H

#include <windows.h>
#include "auto_file.h"

class InfoDlg
{
private:
  static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
  HWND       hwnd;
  HWND       parent;
  HINSTANCE  hinstance;
  char      *filename;

  BOOL message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void init_controls();

public:
  InfoDlg(HINSTANCE hinstance, HWND parent, const char *filename);
  ~InfoDlg();

  int run();
};

#endif

