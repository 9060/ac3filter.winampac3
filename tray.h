#ifndef WINAMPAC3_TRAY_H
#define WINAMPAC3_TRAY_H

#include <windows.h>
#include "guids.h"

class PropThread;

class WinampAC3Tray
{
protected:
  IWinampAC3 *filter;

  PropThread *dialog;
  bool visible;

  HWND  hwnd;
  HICON hicon;
  HMENU hmenu;
  NOTIFYICONDATA nid;

  HMENU create_menu() const;

  static LRESULT CALLBACK TrayProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
  WinampAC3Tray(IWinampAC3 *filter);
  ~WinampAC3Tray();

  // tray icon control
  void show();
  void hide();

  // control actions
  void config();
  void popup_menu();
  void preset(const char *preset);
};

#endif
