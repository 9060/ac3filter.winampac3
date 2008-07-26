#ifndef DLG_CONF_H
#define DLG_CONF_H

#include "controls.h"
#include "tab.h"
#include "guids.h"

          
class ConfigDlg : public TabSheet, public AudioProcessorState
{
public:
  // Custom controllers for different dialogs
  typedef Controller * (*ctrl_maker)(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels);
  static Controller *ctrl_main(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels);
  static Controller *ctrl_mixer(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels);
  static Controller *ctrl_gains(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels);
  static Controller *ctrl_eq(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels);
  static Controller *ctrl_spdif(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels);
  static Controller *ctrl_system(HWND hdlg, IWinampAC3 *filter, IDecoder *dec, IAudioProcessor *proc, bool invert_levels);

  // Dialog creation functions
  static ConfigDlg *create_main(HMODULE hmodule, IWinampAC3 *winampac3);
  static ConfigDlg *create_mixer(HMODULE hmodule, IWinampAC3 *winampac3);
  static ConfigDlg *create_gains(HMODULE hmodule, IWinampAC3 *winampac3);
  static ConfigDlg *create_eq(HMODULE hmodule, IWinampAC3 *winampac3);
  static ConfigDlg *create_spdif(HMODULE hmodule, IWinampAC3 *winampac3);
  static ConfigDlg *create_system(HMODULE hmodule, IWinampAC3 *winampac3);

private:
  IWinampAC3      *filter;
  IDecoder        *dec;
  IAudioProcessor *proc;

  // Dialog controllers
  ctrl_maker maker;
  Controller *ctrl;
  Controller *cpu;

  // Dialog state
  Speakers    in_spk;
  DoubleEdit  edt_refresh_time;
  LinkButton  lnk_donate;

  ConfigDlg(HMODULE hmodule, LPCSTR dlg_res, IWinampAC3 *filter, ctrl_maker maker);

  /////////////////////////////////////////////////////////////////////////////
  // TabSheet overrides

  virtual void on_create();
  virtual void on_destroy();
  virtual void on_show();
  virtual void on_hide();

  virtual BOOL on_message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  /////////////////////////////////////////////////////////////////////////////
  // Interface options

  bool get_tooltips();
  void set_tooltips(bool tooltips);

  bool get_invert_levels();
  void set_invert_levels(bool invert_levels);

  int get_refresh_time();
  void set_refresh_time(int refresh_time);

  /////////////////////////////////////////////////////////////////////////////
  // Interface update functions

  void init_controls();
  void update_dynamic_controls();
  void update_static_controls();

  /////////////////////////////////////////////////////////////////////////////
  // Handle control notifications

  void command(int control, int message);
};

#endif
