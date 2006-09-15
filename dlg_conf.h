#ifndef DLG_CONF_H
#define DLG_CONF_H

#include "tab.h"
#include "controls.h"
#include "guids.h"

          
class ConfigDlg : public TabSheet, public AudioProcessorState
{
public:
  static ConfigDlg *create_main   (HMODULE hmodule, IWinampAC3 *filter);
  static ConfigDlg *create_mixer  (HMODULE hmodule, IWinampAC3 *filter);
  static ConfigDlg *create_gains  (HMODULE hmodule, IWinampAC3 *filter);
  static ConfigDlg *create_system (HMODULE hmodule, IWinampAC3 *filter);
  static ConfigDlg *create_about  (HMODULE hmodule, IWinampAC3 *filter);

  ConfigDlg(HMODULE hmodule, LPCSTR dlg_res, IWinampAC3 *filter);

  void reload_state();

private:
  IWinampAC3      *winampac3;
  IDecoder        *dec;
  IAudioProcessor *proc;

  bool     visible;
  bool     refresh;
  int      refresh_time;

  Speakers in_spk;
  Speakers out_spk;
  Speakers user_spk;

  int      formats;
  bool     query_sink;
  int      reinit;
  int      sink;

  // spdif
  bool     use_spdif;
  int      spdif_pt;
  bool     spdif_as_pcm;
  bool     spdif_encode;
  bool     spdif_stereo_pt;

  bool     spdif_check_sr;
  bool     spdif_allow_48;
  bool     spdif_allow_44;
  bool     spdif_allow_32;

  int      spdif_status;

  // syncronization
  vtime_t  time_shift;
  vtime_t  time_factor;
  bool     dejitter;
  vtime_t  threshold;

  vtime_t  input_mean;
  vtime_t  input_stddev;
  vtime_t  output_mean;
  vtime_t  output_stddev;

  int      frames;
  int      errors;

  Speakers old_in_spk;
  int      old_spdif_status;
  matrix_t old_matrix;
  char     old_info[2048];
  char     old_jitter[128];

  // Matrix
  DoubleEdit  edt_matrix[NCHANNELS][NCHANNELS];
  // AGC
  DoubleEdit  edt_attack;
  DoubleEdit  edt_release;
  // Gain control
  DoubleEdit  edt_master;
  DoubleEdit  edt_gain;
  DoubleEdit  edt_voice;
  DoubleEdit  edt_sur;
  DoubleEdit  edt_lfe;
  // I/O Gains
  DoubleEdit  edt_in_gains[NCHANNELS];
  DoubleEdit  edt_out_gains[NCHANNELS];
  // DRC
  DoubleEdit  edt_drc_power;
  DoubleEdit  edt_drc_level;
  // Bass redirection
  DoubleEdit  edt_bass_freq;
  // Delay
  DoubleEdit  edt_delay[NCHANNELS];
  DoubleEdit  edt_time_shift;
  // Links
  LinkButton  lnk_home;
  LinkButton  lnk_forum;
  LinkButton  lnk_email;
  LinkButton  lnk_donate;

  virtual BOOL message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  void switch_on();
  void switch_off();

  void update();
  void init_controls();
  void set_dynamic_controls();
  void set_controls();
  void set_matrix_controls();
  void set_cpu_usage();

  void command(int control, int message);
};

#endif
