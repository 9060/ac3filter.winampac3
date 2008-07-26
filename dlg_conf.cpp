#include <stdio.h>
#include <windows.h>
#include <commctrl.h>

#include "guids.h"
#include "resource_ids.h"

#include "dlg_conf.h"
#include "dlg/control_all.h"



#define SAFE_RELEASE(p) { if (p) p->Release(); p = 0; }

#define dlg_printf(dlg, ctrl, format, params)                     \
{                                                                 \
  char buf[255];                                                  \
  sprintf(buf, format, ##params);                                 \
  SendDlgItemMessage(dlg, ctrl, WM_SETTEXT, 0, (LONG)(LPSTR)buf); \
}


///////////////////////////////////////////////////////////////////////////////
// Specialized controllers
///////////////////////////////////////////////////////////////////////////////

Controller *ConfigDlg::ctrl_main(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels)
{
  ControlList *ctrl = new ControlList(hdlg);
  if (ctrl == 0) return 0;

  ctrl->add(new ControlSpk(hdlg, dec));
  ctrl->add(new ControlPreset(hdlg, dec, proc));
  ctrl->add(new ControlLevels(hdlg, filter, proc, invert_levels));
  ctrl->add(new ControlAGC(hdlg, proc));
  ctrl->add(new ControlInfo(hdlg, dec));
  ctrl->add(new ControlVer(hdlg));
  return ctrl;
}

Controller *ConfigDlg::ctrl_mixer(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels)
{
  ControlList *ctrl = new ControlList(hdlg);
  if (ctrl == 0) return 0;

  ctrl->add(new ControlSpk(hdlg, dec));
  ctrl->add(new ControlPreset(hdlg, dec, proc));
  ctrl->add(new ControlLevels(hdlg, filter, proc, invert_levels));
  ctrl->add(new ControlAGC(hdlg, proc));
  ctrl->add(new ControlMatrix(hdlg, proc));
  ctrl->add(new ControlBass(hdlg, proc));
  ctrl->add(new ControlVer(hdlg));
  return ctrl;
}

Controller *ConfigDlg::ctrl_gains(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels)
{
  ControlList *ctrl = new ControlList(hdlg);
  if (ctrl == 0) return 0;

  ctrl->add(new ControlPreset(hdlg, dec, proc));
  ctrl->add(new ControlLevels(hdlg, filter, proc, invert_levels));
  ctrl->add(new ControlAGC(hdlg, proc));
  ctrl->add(new ControlMatrix(hdlg, proc));
  ctrl->add(new ControlDelay(hdlg, proc));
  ctrl->add(new ControlIOGains(hdlg, proc));
  ctrl->add(new ControlVer(hdlg));
  return ctrl;
}

Controller *ConfigDlg::ctrl_eq(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels)
{
  ControlList *ctrl = new ControlList(hdlg);
  if (ctrl == 0) return 0;

  ctrl->add(new ControlPreset(hdlg, dec, proc));
  ctrl->add(new ControlEq(hdlg, proc));
  ctrl->add(new ControlSpectrum(hdlg, proc));
  ctrl->add(new ControlVer(hdlg));
  return ctrl;
}

Controller *ConfigDlg::ctrl_spdif(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels)
{
  ControlList *ctrl = new ControlList(hdlg);
  if (ctrl == 0) return 0;

  ctrl->add(new ControlSpk(hdlg, dec));
  ctrl->add(new ControlPreset(hdlg, dec, proc));
  ctrl->add(new ControlSPDIF(hdlg, dec));
  ctrl->add(new ControlSink(hdlg, filter));
  ctrl->add(new ControlSystem(hdlg, filter, dec));
  ctrl->add(new ControlInfo(hdlg, dec));
  ctrl->add(new ControlVer(hdlg));
  return ctrl;
}

Controller *ConfigDlg::ctrl_system(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels)
{
  ControlList *ctrl = new ControlList(hdlg);
  if (ctrl == 0) return 0;

  ctrl->add(new ControlSink(hdlg, filter));
  ctrl->add(new ControlSPDIF(hdlg, dec));
  ctrl->add(new ControlSystem(hdlg, filter, dec));
  ctrl->add(new ControlAGC(hdlg, proc));
  ctrl->add(new ControlVer(hdlg));
  ctrl->add(new ControlAbout(hdlg));
  return ctrl;
}

///////////////////////////////////////////////////////////////////////////////
// Dialog creation
///////////////////////////////////////////////////////////////////////////////

ConfigDlg *ConfigDlg::create_main(HMODULE hmodule, IWinampAC3 *winampac3)
{ return new ConfigDlg(hmodule, MAKEINTRESOURCE(IDD_MAIN), winampac3, &ConfigDlg::ctrl_main); }

ConfigDlg *ConfigDlg::create_mixer(HMODULE hmodule, IWinampAC3 *winampac3)
{ return new ConfigDlg(hmodule, MAKEINTRESOURCE(IDD_MIXER), winampac3, &ConfigDlg::ctrl_mixer); }

ConfigDlg *ConfigDlg::create_gains(HMODULE hmodule, IWinampAC3 *winampac3)
{ return new ConfigDlg(hmodule, MAKEINTRESOURCE(IDD_GAINS), winampac3, &ConfigDlg::ctrl_gains); }

ConfigDlg *ConfigDlg::create_eq(HMODULE hmodule, IWinampAC3 *winampac3)
{ return new ConfigDlg(hmodule, MAKEINTRESOURCE(IDD_EQ), winampac3, &ConfigDlg::ctrl_eq); }

ConfigDlg *ConfigDlg::create_spdif(HMODULE hmodule, IWinampAC3 *winampac3)
{ return new ConfigDlg(hmodule, MAKEINTRESOURCE(IDD_SPDIF), winampac3, &ConfigDlg::ctrl_spdif); }

ConfigDlg *ConfigDlg::create_system(HMODULE hmodule, IWinampAC3 *winampac3)
{ return new ConfigDlg(hmodule, MAKEINTRESOURCE(IDD_SYSTEM), winampac3, &ConfigDlg::ctrl_system); }

///////////////////////////////////////////////////////////////////////////////
// Dialog class
///////////////////////////////////////////////////////////////////////////////

ConfigDlg::ConfigDlg(HMODULE hmodule_, LPCSTR dlg_res_, IWinampAC3 *filter_, ctrl_maker maker_)
:TabSheet(hmodule_, dlg_res_)
{
  maker = maker_;
  filter = filter_;
  dec = filter->get_decoder();
  proc = filter->get_audio_processor();

  InitCommonControls();
}

///////////////////////////////////////////////////////////////////////////////
// TabSheet overrides

void
ConfigDlg::on_create()
{
  // Init controllers
  bool invert_levels = get_invert_levels();
  if (maker) ctrl = (*maker)(hwnd, filter, dec, proc, invert_levels);
  cpu = new ControlCPU(hwnd, filter, invert_levels);

  // Init and update controls
  init_controls();
  update_dynamic_controls();
  update_static_controls();

  // Update 'old' values
  dec->get_in_spk(&in_spk);
}

void
ConfigDlg::on_destroy()
{
  // Destroy controllers
  safe_delete(ctrl);
  safe_delete(cpu);
}

void
ConfigDlg::on_show()
{
  update_dynamic_controls();
  update_static_controls();
  SetTimer(hwnd, 1, get_refresh_time(), 0);  // for all dynamic controls
  SetTimer(hwnd, 2, 1000, 0); // for CPU usage (should be averaged)
}

void
ConfigDlg::on_hide()
{
  KillTimer(hwnd, 1);
  KillTimer(hwnd, 2);
}

BOOL 
ConfigDlg::on_message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  Speakers new_in_spk;
  switch (uMsg)
  {
    case WM_COMMAND:
      command(LOWORD(wParam), HIWORD(wParam));
      return TRUE;

    case WM_HSCROLL:
    case WM_VSCROLL:
      command(GetDlgCtrlID((HWND)lParam), LOWORD(wParam));
      return TRUE;

    case WM_TIMER:
      switch (wParam)
      {
        case 1:
          dec->get_in_spk(&new_in_spk);
          if (in_spk != new_in_spk)
          {
            update_static_controls();
            in_spk = new_in_spk;
          }
          update_dynamic_controls();
          break;

        case 2:
          cpu->update_dynamic();
          break;
      }
      return TRUE;

  }

  return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// Interface
///////////////////////////////////////////////////////////////////////////////

bool
ConfigDlg::get_invert_levels()
{
  bool result = false;
  RegistryKey reg(REG_KEY);
  reg.get_bool("invert_levels", result);
  return result;
}

void
ConfigDlg::set_invert_levels(bool _invert_levels)
{
  RegistryKey reg(REG_KEY);
  reg.set_bool("invert_levels", _invert_levels);
}

int
ConfigDlg::get_refresh_time()
{
  int result = 100;
  RegistryKey reg(REG_KEY);
  reg.get_int32("refresh_time", result);
  return result;
}

void
ConfigDlg::set_refresh_time(int _refresh_time)
{
  RegistryKey reg(REG_KEY);
  reg.set_int32("refresh_time", _refresh_time);
  SetTimer(hwnd, 1, _refresh_time, 0);
}

///////////////////////////////////////////////////////////////////////////////
// Controls initalization/update
///////////////////////////////////////////////////////////////////////////////

void 
ConfigDlg::init_controls()
{
  /////////////////////////////////////
  // Init controllers

  if (ctrl) ctrl->init();
  if (cpu) cpu->init();

  /////////////////////////////////////
  // Interface

  edt_refresh_time.link(hwnd, IDC_EDT_REFRESH_TIME);
}

void 
ConfigDlg::update_dynamic_controls()
{
  if (ctrl) ctrl->update_dynamic();
}

void 
ConfigDlg::update_static_controls()
{
  /////////////////////////////////////
  // Update controllers

  if (ctrl) ctrl->update();
  if (cpu) cpu->update();

  /////////////////////////////////////
  // Interface

  bool tray;
  bool invert_levels;
  int refresh_time;

  filter->get_tray(&tray);
  invert_levels = get_invert_levels();
  refresh_time = get_refresh_time();

  CheckDlgButton(hwnd, IDC_CHK_TRAY, tray? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_INVERT_LEVELS, invert_levels? BST_CHECKED: BST_UNCHECKED);
  edt_refresh_time.update_value(refresh_time);
}

///////////////////////////////////////////////////////////////////////////////
// Commands
///////////////////////////////////////////////////////////////////////////////

void 
ConfigDlg::command(int control, int message)
{
  /////////////////////////////////////
  // Dispatch message to controllers

  if (ctrl && ctrl->own_control(control))
  {
    Controller::cmd_result result = ctrl->command(control, message);
    if (result == Controller::cmd_init) { init_controls(); update_static_controls(); update_dynamic_controls(); }
    if (result == Controller::cmd_update) { update_static_controls(); update_dynamic_controls(); }
    return;
  }

  switch (control)
  {
    /////////////////////////////////////
    // Interface

    case IDC_CHK_TRAY:
    {
      bool tray = IsDlgButtonChecked(hwnd, IDC_CHK_TRAY) == BST_CHECKED;
      filter->set_tray(tray);
      break;
    }

    case IDC_CHK_INVERT_LEVELS:
    {
      bool invert_levels = IsDlgButtonChecked(hwnd, IDC_CHK_INVERT_LEVELS) == BST_CHECKED;
      set_invert_levels(invert_levels);
      break;
    }

    case IDC_EDT_REFRESH_TIME:
      if (message == CB_ENTER)
      {
        int refresh_time = int(edt_refresh_time.value);
        set_refresh_time(refresh_time);
      }
      break;

    /////////////////////////////////////
    // Donate

    case IDC_BTN_DONATE:
      if (message == BN_CLICKED)
        ShellExecute(0, 0, "http://order.kagi.com/?6CZJZ", 0, 0, SW_SHOWMAXIMIZED);
      break;
  }
}
