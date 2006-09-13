#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include <math.h>

#include "winampac3_ver.h"
#include "guids.h"
#include "dlg_conf.h"

#include "resource_ids.h"
#include "resource.h"

#include "registry.h"
#include "filters\delay.h"


#define SAFE_RELEASE(p) { if (p) p->Release(); p = 0; }

const double min_gain_level  = -20.0;
const double max_gain_level  = +20.0;
const double min_level = -50.0;
const double sonic_speed = 330; // m/s
const int ticks = 5;

const int matrix_controls[6][6] =
{
  { IDC_EDT_L_L,   IDC_EDT_C_L,   IDC_EDT_R_L,   IDC_EDT_SL_L,   IDC_EDT_SR_L,   IDC_EDT_LFE_L },
  { IDC_EDT_L_C,   IDC_EDT_C_C,   IDC_EDT_R_C,   IDC_EDT_SL_C,   IDC_EDT_SR_C,   IDC_EDT_LFE_C },
  { IDC_EDT_L_R,   IDC_EDT_C_R,   IDC_EDT_R_R,   IDC_EDT_SL_R,   IDC_EDT_SR_R,   IDC_EDT_LFE_R },
  { IDC_EDT_L_SL,  IDC_EDT_C_SL,  IDC_EDT_R_SL,  IDC_EDT_SL_SL,  IDC_EDT_SR_SL,  IDC_EDT_LFE_SL },
  { IDC_EDT_L_SR,  IDC_EDT_C_SR,  IDC_EDT_R_SR,  IDC_EDT_SL_SR,  IDC_EDT_SR_SR,  IDC_EDT_LFE_SR },
  { IDC_EDT_L_LFE, IDC_EDT_C_LFE, IDC_EDT_R_LFE, IDC_EDT_SL_LFE, IDC_EDT_SR_LFE, IDC_EDT_LFE_LFE }
};

const int idc_level_in[6]   = { IDC_IN_L,  IDC_IN_C,  IDC_IN_R,  IDC_IN_SL,  IDC_IN_SR,  IDC_IN_LFE  };
const int idc_level_out[6]  = { IDC_OUT_L, IDC_OUT_C, IDC_OUT_R, IDC_OUT_SL, IDC_OUT_SR, IDC_OUT_LFE };

const int idc_slider_in[6]  = { IDC_SLIDER_IN_L,  IDC_SLIDER_IN_C,  IDC_SLIDER_IN_R,  IDC_SLIDER_IN_SL,  IDC_SLIDER_IN_SR,  IDC_SLIDER_IN_LFE  };
const int idc_slider_out[6] = { IDC_SLIDER_OUT_L, IDC_SLIDER_OUT_C, IDC_SLIDER_OUT_R, IDC_SLIDER_OUT_SL, IDC_SLIDER_OUT_SR, IDC_SLIDER_OUT_LFE };

const int idc_edt_in[6]     = { IDC_EDT_IN_L,  IDC_EDT_IN_C,  IDC_EDT_IN_R,  IDC_EDT_IN_SL,  IDC_EDT_IN_SR,  IDC_EDT_IN_LFE  };
const int idc_edt_out[6]    = { IDC_EDT_OUT_L, IDC_EDT_OUT_C, IDC_EDT_OUT_R, IDC_EDT_OUT_SL, IDC_EDT_OUT_SR, IDC_EDT_OUT_LFE };

const int idc_edt_delay[6]  = { IDC_EDT_DL, IDC_EDT_DC, IDC_EDT_DR, IDC_EDT_DSL, IDC_EDT_DSR, IDC_EDT_DLFE };


#define dlg_printf(dlg, ctrl, format, params)                     \
{                                                                 \
  char buf[255];                                                  \
  sprintf(buf, format, ##params);                                 \
  SendDlgItemMessage(dlg, ctrl, WM_SETTEXT, 0, (LONG)(LPSTR)buf); \
}

///////////////////////////////////////////////////////////////////////////////
// Registry functions
///////////////////////////////////////////////////////////////////////////////

bool delete_reg_key(const char *name, HKEY root)
{
  HKEY  key;
  char  buf[256];
  DWORD len;

  if (RegOpenKeyEx(root, name, 0, KEY_READ | KEY_WRITE, &key) != ERROR_SUCCESS)
    return false;

  len = 256;
  while (RegEnumKeyEx(key, 0, (LPTSTR)buf, &len, 0, 0, 0, 0) == ERROR_SUCCESS)
  {
    len = 256;
    if (!delete_reg_key(buf, key))
    {
      RegCloseKey(key);
      return false;
    }
  }

  len = 256;
  while (RegEnumValue(key, 0, (LPTSTR)buf, &len, 0, 0, 0, 0) == ERROR_SUCCESS)
  {
    len = 256;
    if (RegDeleteValue(key, buf) != ERROR_SUCCESS)
    {
      RegCloseKey(key);
      return false;
    }
  }

  RegCloseKey(key);
  RegDeleteKey(root, name);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Speakers list definition
///////////////////////////////////////////////////////////////////////////////

const char *spklist[] = 
{
  "AS IS (no change)",
  "1/0 - mono",
  "2/0 - stereo",
  "3/0 - 3 front",
  "2/1 - surround",
  "3/1 - surround",
  "2/2 - quadro",
  "3/2 - 5 channels",
  "1/0+SW 1.1 mono",
  "2/0+SW 2.1 stereo",
  "3/0+SW 3.1 front",
  "2/1+SW 3.1 surround",
  "3/1+SW 4.1 surround",
  "2/2+SW 4.1 quadro",
  "3/2+SW 5.1 channels",
  "Dolby Surround/ProLogic",
  "Dolby ProLogic II",
};

const char *fmtlist[] = 
{
  "PCM 16bit",
  "PCM 24bit",
  "PCM 32bit",
  "PCM Float",
};

Speakers list2spk(int ispk, int ifmt, int sample_rate)
{
  int format = FORMAT_PCM16;
  int mask = MODE_STEREO;
  int relation = NO_RELATION;
  sample_t level = 32767;

  switch (ispk)
  {
    case  0: mask = 0;        break;
    case  1: mask = MODE_1_0; break;
    case  2: mask = MODE_2_0; break;
    case  3: mask = MODE_3_0; break;
    case  4: mask = MODE_2_1; break;
    case  5: mask = MODE_3_1; break;
    case  6: mask = MODE_2_2; break;
    case  7: mask = MODE_3_2; break;
           
    case  8: mask = MODE_1_0 | CH_MASK_LFE; break;
    case  9: mask = MODE_2_0 | CH_MASK_LFE; break;
    case 10: mask = MODE_3_0 | CH_MASK_LFE; break;
    case 11: mask = MODE_2_1 | CH_MASK_LFE; break;
    case 12: mask = MODE_3_1 | CH_MASK_LFE; break;
    case 13: mask = MODE_2_2 | CH_MASK_LFE; break;
    case 14: mask = MODE_3_2 | CH_MASK_LFE; break;

    case 15: mask = MODE_STEREO; relation = RELATION_DOLBY; break;
    case 16: mask = MODE_STEREO, relation = RELATION_DOLBY2; break;
  }

  switch (ifmt)
  {
    case 0: format = FORMAT_PCM16;    level = 32767; break;
    case 1: format = FORMAT_PCM24;    level = 8388607; break;
    case 2: format = FORMAT_PCM32;    level = 2147483647; break;
    case 3: format = FORMAT_PCMFLOAT; level = 1.0; break;
  }

  return Speakers(format, mask, sample_rate, level, relation);
}

int spk2ispk(Speakers spk)
{
  switch (spk.relation)
  {
    case RELATION_DOLBY:   return 15;
    case RELATION_DOLBY2:  return 16;
    default:
      switch (spk.mask)
      {
        case 0:            return 0;
        case MODE_1_0:     return 1;
        case MODE_2_0:     return 2;
        case MODE_3_0:     return 3;
        case MODE_2_1:     return 4;
        case MODE_3_1:     return 5;
        case MODE_2_2:     return 6;
        case MODE_3_2:     return 7;
                                  
        case MODE_1_0 | CH_MASK_LFE: return  8;
        case MODE_2_0 | CH_MASK_LFE: return  9;
        case MODE_3_0 | CH_MASK_LFE: return 10;
        case MODE_2_1 | CH_MASK_LFE: return 11;
        case MODE_3_1 | CH_MASK_LFE: return 12;
        case MODE_2_2 | CH_MASK_LFE: return 13;
        case MODE_3_2 | CH_MASK_LFE: return 14;
      }
  }
  return 0;
}

int spk2ifmt(Speakers spk)
{
  switch (spk.format)
  {
    case FORMAT_PCM16:    return 0;
    case FORMAT_PCM24:    return 1;
    case FORMAT_PCM32:    return 2;
    case FORMAT_PCMFLOAT: return 3;
  }
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Delay units
///////////////////////////////////////////////////////////////////////////////

char *units_list[] =
{
  "Samples",
  "Millisecs",
  "Meters",
  "Centimeters",
  "Feet",
  "Inches"
};

int list2units(int list)
{
  switch (list)
  {
    case 0:  return DELAY_SP;
    case 1:  return DELAY_MS;
    case 2:  return DELAY_M;
    case 3:  return DELAY_CM;
    case 4:  return DELAY_FT;
    case 5:  return DELAY_IN;
    default: return DELAY_SP;
  }
}

int units2list(int units)
{
  switch (units)
  {
    case DELAY_SP: return 0;
    case DELAY_MS: return 1;
    case DELAY_M : return 2;
    case DELAY_CM: return 3;
    case DELAY_FT: return 4;
    case DELAY_IN: return 5;
    default:       return 0;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Initialization
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Initialization / Deinitialization
///////////////////////////////////////////////////////////////////////////////

ConfigDlg *ConfigDlg::create_main(HMODULE hmodule, IWinampAC3 *winampac3)
{ return new ConfigDlg(hmodule, MAKEINTRESOURCE(IDD_MAIN), winampac3); }

ConfigDlg *ConfigDlg::create_mixer(HMODULE hmodule, IWinampAC3 *winampac3)
{ return new ConfigDlg(hmodule, MAKEINTRESOURCE(IDD_MIXER), winampac3); }

ConfigDlg *ConfigDlg::create_gains(HMODULE hmodule, IWinampAC3 *winampac3)
{ return new ConfigDlg(hmodule, MAKEINTRESOURCE(IDD_GAINS), winampac3); }

ConfigDlg *ConfigDlg::create_system(HMODULE hmodule, IWinampAC3 *winampac3)
{ return new ConfigDlg(hmodule, MAKEINTRESOURCE(IDD_SYS), winampac3); }

ConfigDlg *ConfigDlg::create_about(HMODULE hmodule, IWinampAC3 *winampac3)
{ return new ConfigDlg(hmodule, MAKEINTRESOURCE(IDD_ABOUT), winampac3); }


ConfigDlg::ConfigDlg(HMODULE _hmodule, LPCSTR _dlg_res, IWinampAC3 *_winampac3) 
:TabSheet(_hmodule, _dlg_res)
{
  winampac3 = _winampac3;
  dec = winampac3->get_decoder();
  proc = winampac3->get_audio_processor();

  InitCommonControls();
}

void
ConfigDlg::switch_on()
{
  /////////////////////////////////////
  // Refresh dialog state

  refresh = true;
  reload_state();
  init_controls();
  set_dynamic_controls();
  set_controls();
  set_cpu_usage();

  /////////////////////////////////////
  // Start timers

  RegistryKey reg(REG_KEY);
  int refresh_time = 100;
  reg.get_int32("refresh", refresh_time);

  SetTimer(hwnd, 1, refresh_time, 0);  // for all dynamic controls
  SetTimer(hwnd, 2, 1000, 0);          // for CPU usage (should be averaged)

  TabSheet::switch_on();
}

void
ConfigDlg::switch_off()
{
  KillTimer(hwnd, 1);
  KillTimer(hwnd, 2);
  TabSheet::switch_off();
}

///////////////////////////////////////////////////////////////////////////////
// Handle messages
///////////////////////////////////////////////////////////////////////////////

BOOL 
ConfigDlg::message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_COMMAND:
      command(LOWORD(wParam), HIWORD(wParam));
      return 1;

    case WM_HSCROLL:
    case WM_VSCROLL:
      command(GetDlgCtrlID((HWND)lParam), LOWORD(wParam));
      return 1;

    case WM_TIMER:
      if (IsWindowVisible(hwnd))
        if (visible)
        {
          reload_state();
          if (in_spk != old_in_spk)
          {
            set_controls();
            set_dynamic_controls();
          }
          else
            if (wParam == 1) 
              set_dynamic_controls();

          if (wParam == 2) 
            set_cpu_usage();
        }
        else
        {
          reload_state();
          set_controls();
          set_dynamic_controls();
          set_cpu_usage();
          refresh = true;
          visible = true;
        }
      else
        visible = false;

      return 1;

  }

  return TabSheet::message(hwnd, uMsg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////
// Controls initalization/update
///////////////////////////////////////////////////////////////////////////////

void 
ConfigDlg::update()
{
  reload_state();
  set_dynamic_controls();
  set_controls();
}


void 
ConfigDlg::reload_state()
{
  dec->get_in_spk(&in_spk);
  dec->get_out_spk(&out_spk);
  dec->get_user_spk(&user_spk);

  dec->get_formats(&formats);
  dec->get_query_sink(&query_sink);

  // spdif
  dec->get_use_spdif(&use_spdif);
  dec->get_spdif_pt(&spdif_pt);
  dec->get_spdif_as_pcm(&spdif_as_pcm);
  dec->get_spdif_encode(&spdif_encode);
  dec->get_spdif_stereo_pt(&spdif_stereo_pt);

  dec->get_spdif_check_sr(&spdif_check_sr);
  dec->get_spdif_allow_48(&spdif_allow_48);
  dec->get_spdif_allow_44(&spdif_allow_44);
  dec->get_spdif_allow_32(&spdif_allow_32);

  dec->get_spdif_status(&spdif_status);

  // syncronization
  dec->get_time_shift(&time_shift);
  dec->get_time_factor(&time_factor);
  dec->get_dejitter(&dejitter);
  dec->get_threshold(&threshold);
  dec->get_jitter(&input_mean, &input_stddev, &output_mean, &output_stddev);


  dec->get_frames(&frames, &errors);

  vtime_t time;
  winampac3->get_playback_time(&time);
  proc->get_state(this, time);
}

void 
ConfigDlg::init_controls()
{
  int i, j, ch;

  /////////////////////////////////////
  // Speakers

  SendDlgItemMessage(hwnd, IDC_CMB_SPK, CB_RESETCONTENT, 0, 0);
  for (i = 0; i < sizeof(spklist) / sizeof(spklist[0]); i++)
    SendDlgItemMessage(hwnd, IDC_CMB_SPK, CB_ADDSTRING, 0, (LONG)spklist[i]);

  /////////////////////////////////////
  // CPU usage

  SendDlgItemMessage(hwnd, IDC_CPU, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

  /////////////////////////////////////
  // Formats

  SendDlgItemMessage(hwnd, IDC_CMB_FORMAT, CB_RESETCONTENT, 0, 0);
  for (i = 0; i < sizeof(fmtlist) / sizeof(fmtlist[0]); i++)
    SendDlgItemMessage(hwnd, IDC_CMB_FORMAT, CB_ADDSTRING, 0, (LONG)fmtlist[i]);

  /////////////////////////////////////
  // Matrix

  for (i = 0; i < 6; i++)
    for (j = 0; j < 6; j++)
      edt_matrix[i][j].link(hwnd, matrix_controls[i][j]);

  /////////////////////////////////////
  // Gains

  SendDlgItemMessage(hwnd, IDC_SLIDER_MASTER, TBM_SETRANGE, TRUE, MAKELONG(min_gain_level, max_gain_level) * ticks);
  SendDlgItemMessage(hwnd, IDC_SLIDER_MASTER, TBM_SETTIC, 0, 0);
  SendDlgItemMessage(hwnd, IDC_SLIDER_GAIN,   TBM_SETRANGE, TRUE, MAKELONG(min_gain_level, max_gain_level) * ticks);
  SendDlgItemMessage(hwnd, IDC_SLIDER_GAIN,   TBM_SETTIC, 0, 0);

  SendDlgItemMessage(hwnd, IDC_SLIDER_LFE,    TBM_SETRANGE, TRUE, MAKELONG(min_gain_level, max_gain_level) * ticks);
  SendDlgItemMessage(hwnd, IDC_SLIDER_LFE,    TBM_SETTIC, 0, 0);
  SendDlgItemMessage(hwnd, IDC_SLIDER_VOICE,  TBM_SETRANGE, TRUE, MAKELONG(min_gain_level, max_gain_level) * ticks);
  SendDlgItemMessage(hwnd, IDC_SLIDER_VOICE,  TBM_SETTIC, 0, 0);
  SendDlgItemMessage(hwnd, IDC_SLIDER_SUR,    TBM_SETRANGE, TRUE, MAKELONG(min_gain_level, max_gain_level) * ticks);
  SendDlgItemMessage(hwnd, IDC_SLIDER_SUR,    TBM_SETTIC, 0, 0);

  edt_master.link(hwnd, IDC_EDT_MASTER);
  edt_gain  .link(hwnd, IDC_EDT_GAIN);
  edt_voice .link(hwnd, IDC_EDT_VOICE);
  edt_sur   .link(hwnd, IDC_EDT_SUR);
  edt_lfe   .link(hwnd, IDC_EDT_LFE);

  edt_gain.enable(false);

 
  /////////////////////////////////////
  // I/O Gains

  for (ch = 0; ch < NCHANNELS; ch++)
  {
    SendDlgItemMessage(hwnd, idc_slider_in[ch],  TBM_SETRANGE, TRUE, MAKELONG(min_gain_level, max_gain_level) * ticks);
    SendDlgItemMessage(hwnd, idc_slider_out[ch], TBM_SETRANGE, TRUE, MAKELONG(min_gain_level, max_gain_level) * ticks);
    SendDlgItemMessage(hwnd, idc_slider_in[ch],  TBM_SETTIC, 0, 0);
    SendDlgItemMessage(hwnd, idc_slider_out[ch], TBM_SETTIC, 0, 0);
    edt_in_gains[ch].link(hwnd, idc_edt_in[ch]);
    edt_out_gains[ch].link(hwnd, idc_edt_out[ch]);
  }

  /////////////////////////////////////
  // I/O Levels

  for (ch = 0; ch < NCHANNELS; ch++)
  {
    SendDlgItemMessage(hwnd, idc_level_in[ch],  PBM_SETBARCOLOR, 0, RGB(0, 128, 0));
    SendDlgItemMessage(hwnd, idc_level_out[ch], PBM_SETBARCOLOR, 0, RGB(0, 128, 0));
    // log scale
    SendDlgItemMessage(hwnd, idc_level_in[ch],  PBM_SETRANGE, 0, MAKELPARAM(0, -min_level * ticks));
    SendDlgItemMessage(hwnd, idc_level_out[ch], PBM_SETRANGE, 0, MAKELPARAM(0, -min_level * ticks));
//    // linear scale
//    SendDlgItemMessage(hwnd, in_ch2control[ch],  PBM_SETRANGE, 0, MAKELPARAM(0, 256));
//    SendDlgItemMessage(hwnd, out_ch2control[ch], PBM_SETRANGE, 0, MAKELPARAM(0, 256));
  }

  /////////////////////////////////////
  // Delay

  for (ch = 0; ch < NCHANNELS; ch++)
    edt_delay[ch].link(hwnd, idc_edt_delay[ch]);

  /////////////////////////////////////
  // Syncronization

  edt_time_shift.link(hwnd, IDC_EDT_TIME_SHIFT);
  SendDlgItemMessage(hwnd, IDC_SLIDER_TIME_SHIFT, TBM_SETRANGE, TRUE, MAKELONG(-500, 500));
  SendDlgItemMessage(hwnd, IDC_SLIDER_TIME_SHIFT, TBM_SETTIC, 0, 0);

  /////////////////////////////////////
  // AGC

  edt_attack.link(hwnd, IDC_EDT_ATTACK);
  edt_release.link(hwnd, IDC_EDT_RELEASE);

  /////////////////////////////////////
  // DRC

  SendDlgItemMessage(hwnd, IDC_SLIDER_DRC_POWER, TBM_SETRANGE, TRUE, MAKELONG(min_gain_level, max_gain_level) * ticks);
  SendDlgItemMessage(hwnd, IDC_SLIDER_DRC_POWER, TBM_SETTIC, 0, 0);
  SendDlgItemMessage(hwnd, IDC_SLIDER_DRC_LEVEL, TBM_SETRANGE, TRUE, MAKELONG(min_gain_level, max_gain_level) * ticks);
  SendDlgItemMessage(hwnd, IDC_SLIDER_DRC_LEVEL, TBM_SETTIC, 0, 0);
  edt_drc_power.link(hwnd, IDC_EDT_DRC_POWER);
  edt_drc_level.link(hwnd, IDC_EDT_DRC_LEVEL);

  /////////////////////////////////////
  // Bass redirection

  edt_bass_freq.link(hwnd, IDC_EDT_BASS_FREQ);

  /////////////////////////////////////
  // Delay units

  SendDlgItemMessage(hwnd, IDC_CMB_UNITS, CB_RESETCONTENT, 0, 0);
  for (i = 0; i < sizeof(units_list) / sizeof(units_list[0]); i++)
    SendDlgItemMessage(hwnd, IDC_CMB_UNITS, CB_ADDSTRING, 0, (LONG)units_list[i]);

  /////////////////////////////////////
  // Links

  lnk_home.link(hwnd, IDC_LNK_HOME);
  lnk_forum.link(hwnd, IDC_LNK_FORUM);
  lnk_email.link(hwnd, IDC_LNK_EMAIL);
  lnk_donate.link(hwnd, IDC_LNK_DONATE);

  /////////////////////////////////////
  // Build and environment info

  char info[1024];

  strncpy(info, valib_build_info(), sizeof(info));
  info[sizeof(info)-1] = 0;
  cr2crlf(info, sizeof(info));
  SetDlgItemText(hwnd, IDC_EDT_ENV, info);

  strncpy(info, valib_credits(), sizeof(info));
  info[sizeof(info)-1] = 0;
  cr2crlf(info, sizeof(info));
  SetDlgItemText(hwnd, IDC_EDT_CREDITS, info);

  /////////////////////////////////////
  // Version

  char ver1[255];
  char ver2[255];
  GetDlgItemText(hwnd, IDC_VER, ver1, array_size(ver1));
  sprintf(ver2, ver1, WINAMPAC3_VER);
  SetDlgItemText(hwnd, IDC_VER, ver2);
}

void 
ConfigDlg::set_dynamic_controls()
{
  /////////////////////////////////////////////////////////
  // Input format

  if (old_in_spk != in_spk || refresh)
  {
    char buf[128];
    old_in_spk = in_spk;
    sprintf(buf, "%s %s %iHz", in_spk.format_text(), in_spk.mode_text(), in_spk.sample_rate);
    SetDlgItemText(hwnd, IDC_LBL_INPUT, buf);
  }

  /////////////////////////////////////
  // SPDIF mode

  if (spdif_status != old_spdif_status || refresh)
  {
    old_spdif_status = spdif_status;
    switch (old_spdif_status)
    {
      case SPDIF_MODE_NONE:
        SetDlgItemText(hwnd, IDC_CHK_SPDIF, "Use SPDIF");
        break;

      case SPDIF_MODE_DISABLED:
        SetDlgItemText(hwnd, IDC_CHK_SPDIF, "Use SPDIF (disabled)");
        break;

      case SPDIF_MODE_PASSTHROUGH:
        SetDlgItemText(hwnd, IDC_CHK_SPDIF, "Use SPDIF (passthrough)");
        break;

      case SPDIF_MODE_ENCODE:
        SetDlgItemText(hwnd, IDC_CHK_SPDIF, "Use SPDIF (AC3 encode)");
        break;

      default:
        SetDlgItemText(hwnd, IDC_CHK_SPDIF, "Use SPDIF (Unknown)");
        break;
    }
  }

  /////////////////////////////////////
  // Stream info

  char info[sizeof(old_info)];
  memset(info, 0, sizeof(old_info));
  dec->get_info(info, sizeof(old_info));
  if (memcmp(info, old_info, sizeof(old_info)) || refresh)
  {
    memcpy(old_info, info, sizeof(old_info));
    SendDlgItemMessage(hwnd, IDC_EDT_INFO, WM_SETTEXT, 0, (LONG)(LPSTR)info);
  }

  dlg_printf(hwnd, IDC_EDT_FRAMES, "%i", frames);
  dlg_printf(hwnd, IDC_EDT_ERRORS, "%i", errors);

  /////////////////////////////////////
  // Auto gain control

  SendDlgItemMessage(hwnd, IDC_SLIDER_GAIN, TBM_SETPOS, TRUE, long(-value2db(gain) * ticks));
  edt_gain.update_value(value2db(gain));

  /////////////////////////////////////
  // I/O Levels

  for (int ch = 0; ch < NCHANNELS; ch++)
  {
    // log scale
    SendDlgItemMessage(hwnd, idc_level_in[ch],  PBM_SETPOS, input_levels[ch]  > 0? long(-(min_level - value2db(input_levels[ch])) * ticks): -1000 * ticks,  0);
    SendDlgItemMessage(hwnd, idc_level_out[ch], PBM_SETPOS, output_levels[ch] > 0? long(-(min_level - value2db(output_levels[ch])) * ticks): -1000 * ticks,  0);
    SendDlgItemMessage(hwnd, idc_level_out[ch], PBM_SETBARCOLOR, 0, (output_levels[ch] > 0.99)? RGB(255, 0, 0): RGB(0, 128, 0));
//    // linear scale
//    SendDlgItemMessage(hwnd, in_ch2control[ch],  PBM_SETPOS,   long(in_levels[ch] * 256 / in_level),  0);
//    SendDlgItemMessage(hwnd, out_ch2control[ch], PBM_SETPOS,   long(out_levels[ch]* 256 / out_level), 0);
  }

  /////////////////////////////////////
  // DRC

  SendDlgItemMessage(hwnd, IDC_SLIDER_DRC_LEVEL, TBM_SETPOS, TRUE, long(-value2db(drc_level) * ticks));
  edt_drc_level.update_value(value2db(drc_level));

  /////////////////////////////////////
  // Syncronization

  char jitter[sizeof(old_jitter)];
  sprintf(jitter, "Input\tmean: %ims\tstddev: %ims\r\nOutput\tmean: %ims\tstddev: %ims", 
    int(input_mean * 1000), int(input_stddev * 1000), 
    int(output_mean * 1000), int(output_stddev * 1000));

  if (memcmp(jitter, old_jitter, strlen(jitter)) || refresh)
  {
    memcpy(old_jitter, jitter, sizeof(old_jitter));
    SendDlgItemMessage(hwnd, IDC_EDT_JITTER, WM_SETTEXT, 0, (LONG)(LPSTR)jitter);
  }

  /////////////////////////////////////
  // Matrix controls

  if (auto_matrix)
    set_matrix_controls();

  refresh = false;
}

void 
ConfigDlg::set_controls()
{
  int ch;

  /////////////////////////////////////
  // Speakers

  SendDlgItemMessage(hwnd, IDC_CMB_SPK,    CB_SETCURSEL, spk2ispk(user_spk), 0);
  SendDlgItemMessage(hwnd, IDC_CMB_FORMAT, CB_SETCURSEL, spk2ifmt(user_spk), 0);

  /////////////////////////////////////
  // SPDIF

  CheckDlgButton(hwnd, IDC_CHK_SPDIF, use_spdif? BST_CHECKED: BST_UNCHECKED);

  /////////////////////////////////////
  // SPDIF passthrough

  CheckDlgButton(hwnd, IDC_CHK_SPDIF_MPA, (spdif_pt & FORMAT_MASK_MPA) != 0? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_SPDIF_AC3, (spdif_pt & FORMAT_MASK_AC3) != 0? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_SPDIF_DTS, (spdif_pt & FORMAT_MASK_DTS) != 0? BST_CHECKED: BST_UNCHECKED);

  /////////////////////////////////////
  // SPDIF options

  CheckDlgButton(hwnd, IDC_CHK_SPDIF_AS_PCM, spdif_as_pcm? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_SPDIF_ENCODE, spdif_encode? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_SPDIF_STEREO_PT, spdif_stereo_pt? BST_CHECKED: BST_UNCHECKED);

  CheckDlgButton(hwnd, IDC_CHK_SPDIF_CHECK_SR, spdif_check_sr? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_SPDIF_ALLOW_48, spdif_allow_48? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_SPDIF_ALLOW_44, spdif_allow_44? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_SPDIF_ALLOW_32, spdif_allow_32? BST_CHECKED: BST_UNCHECKED);

  EnableWindow(GetDlgItem(hwnd, IDC_CHK_SPDIF_STEREO_PT), spdif_encode);
  EnableWindow(GetDlgItem(hwnd, IDC_CHK_SPDIF_ALLOW_48), spdif_check_sr);
  EnableWindow(GetDlgItem(hwnd, IDC_CHK_SPDIF_ALLOW_44), spdif_check_sr);
  EnableWindow(GetDlgItem(hwnd, IDC_CHK_SPDIF_ALLOW_32), spdif_check_sr);

  /////////////////////////////////////
  // Formats

  CheckDlgButton(hwnd, IDC_CHK_PCM, (formats & FORMAT_CLASS_PCM_LE) != 0? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_MPA, (formats & FORMAT_MASK_MPA) != 0? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_AC3, (formats & FORMAT_MASK_AC3) != 0? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_DTS, (formats & FORMAT_MASK_DTS) != 0? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hwnd, IDC_CHK_PES, (formats & FORMAT_MASK_PES) != 0? BST_CHECKED: BST_UNCHECKED);

  /////////////////////////////////////
  // Query sink and force reinit

  CheckDlgButton(hwnd, IDC_CHK_QUERY_SINK, query_sink? BST_CHECKED: BST_UNCHECKED);

  {
    RegistryKey reg(REG_KEY);
    int reinit_samples = 0;
    reg.get_int32("reinit_samples", reinit_samples);
    CheckDlgButton(hwnd, IDC_CHK_REINIT, reinit_samples > 0? BST_CHECKED: BST_UNCHECKED);
  }


  /////////////////////////////////////
  // Auto gain control

  SendDlgItemMessage(hwnd, IDC_SLIDER_MASTER, TBM_SETPOS, TRUE, long(-value2db(master) * ticks));
  edt_master.update_value(value2db(master));

  SendDlgItemMessage(hwnd, IDC_CHK_AUTO_GAIN, BM_SETCHECK, auto_gain? BST_CHECKED: BST_UNCHECKED, 1);
  SendDlgItemMessage(hwnd, IDC_CHK_NORMALIZE, BM_SETCHECK, normalize? BST_CHECKED: BST_UNCHECKED, 1);
  EnableWindow(GetDlgItem(hwnd, IDC_CHK_NORMALIZE), auto_gain);

  edt_attack.update_value(attack);
  edt_release.update_value(release);

  /////////////////////////////////////
  // Gain controls

  SendDlgItemMessage(hwnd, IDC_SLIDER_VOICE, TBM_SETPOS, TRUE, long(-value2db(clev)   * ticks));
  SendDlgItemMessage(hwnd, IDC_SLIDER_SUR,   TBM_SETPOS, TRUE, long(-value2db(slev)   * ticks));
  SendDlgItemMessage(hwnd, IDC_SLIDER_LFE,   TBM_SETPOS, TRUE, long(-value2db(lfelev) * ticks));

  edt_voice.update_value(value2db(clev));
  edt_sur  .update_value(value2db(slev));
  edt_lfe  .update_value(value2db(lfelev));

  /////////////////////////////////////
  // I/O Gains

  for (ch = 0; ch < NCHANNELS; ch++)
  {
    SendDlgItemMessage(hwnd, idc_slider_in[ch],  TBM_SETPOS, TRUE, long(-value2db(input_gains[ch])  * ticks));
    SendDlgItemMessage(hwnd, idc_slider_out[ch], TBM_SETPOS, TRUE, long(-value2db(output_gains[ch]) * ticks));
    edt_in_gains[ch].update_value(value2db(input_gains[ch]));
    edt_out_gains[ch].update_value(value2db(output_gains[ch]));
  }

  /////////////////////////////////////
  // Delay

  for (ch = 0; ch < NCHANNELS; ch++)
  {
    edt_delay[ch].update_value(delays[ch]);
    edt_delay[ch].enable(delay);
  }
  SendDlgItemMessage(hwnd, IDC_CHK_DELAY, BM_SETCHECK, delay? BST_CHECKED: BST_UNCHECKED, 1);
  SendDlgItemMessage(hwnd, IDC_CMB_UNITS, CB_SETCURSEL, units2list(delay_units), 0);
  EnableWindow(GetDlgItem(hwnd, IDC_CMB_UNITS), delay);

  /////////////////////////////////////
  // Syncronization

  edt_time_shift.update_value(time_shift * 1000);
  SendDlgItemMessage(hwnd, IDC_SLIDER_TIME_SHIFT, TBM_SETPOS, TRUE, int(time_shift * 1000));

  SendDlgItemMessage(hwnd, IDC_CHK_JITTER, BM_SETCHECK, dejitter? BST_CHECKED: BST_UNCHECKED, 1);
//  SetDlgItemInt(hwnd, IDC_LBL_JITTER, int(jitter * 1000), false);

  /////////////////////////////////////
  // DRC

  SendDlgItemMessage(hwnd, IDC_CHK_DRC, BM_SETCHECK, drc? BST_CHECKED: BST_UNCHECKED, 1);
  SendDlgItemMessage(hwnd, IDC_SLIDER_DRC_POWER, TBM_SETPOS, TRUE, long(-drc_power * ticks));
  edt_drc_power.update_value(drc_power);

  /////////////////////////////////////
  // Bass redirection

  SendDlgItemMessage(hwnd, IDC_CHK_BASS_REDIR, BM_SETCHECK, bass_redir? BST_CHECKED: BST_UNCHECKED, 1);
  edt_bass_freq.update_value(bass_freq);

  /////////////////////////////////////
  // Matrix Flags

  SendDlgItemMessage(hwnd, IDC_CHK_AUTO_MATRIX,   BM_SETCHECK, auto_matrix?      BST_CHECKED: BST_UNCHECKED, 1);
  SendDlgItemMessage(hwnd, IDC_CHK_EXPAND_STEREO, BM_SETCHECK, expand_stereo?    BST_CHECKED: BST_UNCHECKED, 1);
  SendDlgItemMessage(hwnd, IDC_CHK_VOICE_CONTROL, BM_SETCHECK, voice_control?    BST_CHECKED: BST_UNCHECKED, 1);
  SendDlgItemMessage(hwnd, IDC_CHK_NORM_MATRIX,   BM_SETCHECK, normalize_matrix? BST_CHECKED: BST_UNCHECKED, 1);
  EnableWindow(GetDlgItem(hwnd, IDC_CHK_EXPAND_STEREO), auto_matrix);
  EnableWindow(GetDlgItem(hwnd, IDC_CHK_VOICE_CONTROL), auto_matrix);
  EnableWindow(GetDlgItem(hwnd, IDC_CHK_NORM_MATRIX), auto_matrix);

  /////////////////////////////////////
  // Presets

  #define fill_combobox(control, registry)                                                \
  {                                                                                       \
    HKEY key;                                                                             \
    char preset[256];                                                                     \
    int  n;                                                                               \
                                                                                          \
                                                                                          \
    SendDlgItemMessage(hwnd, control, WM_GETTEXT, 256, (LONG)preset);                    \
    SendDlgItemMessage(hwnd, control, CB_RESETCONTENT, 0, 0);                            \
                                                                                          \
    if (RegOpenKeyEx(HKEY_CURRENT_USER, registry, 0, KEY_READ, &key) == ERROR_SUCCESS)    \
    {                                                                                     \
      char buf[256];                                                                      \
      int i = 0;                                                                          \
      DWORD len = 256;                                                                    \
      while (RegEnumKeyEx(key, i++, (LPTSTR)buf, &len, 0, 0, 0, 0) == ERROR_SUCCESS)      \
      {                                                                                   \
        SendDlgItemMessage(hwnd, control, CB_ADDSTRING, 0, (LONG)buf);                   \
        len = 256;                                                                        \
      }                                                                                   \
      RegCloseKey(key);                                                                   \
    }                                                                                     \
                                                                                          \
    n = SendDlgItemMessage(hwnd, control, CB_FINDSTRINGEXACT, 0, (LONG)preset);          \
    if (n != CB_ERR)                                                                      \
      SendDlgItemMessage(hwnd, control, CB_SETCURSEL, n, 0);                             \
    SendDlgItemMessage(hwnd, control, WM_SETTEXT, 0, (LONG)preset);                      \
  }

  fill_combobox(IDC_CMB_PRESET, REG_KEY_PRESET);
  fill_combobox(IDC_CMB_MATRIX, REG_KEY_MATRIX);

  /////////////////////////////////////
  // Matrix

  set_matrix_controls();
}

void 
ConfigDlg::set_matrix_controls()
{
  bool auto_matrix;
  matrix_t matrix;

  proc->get_matrix(&matrix);
  proc->get_auto_matrix(&auto_matrix);

  if (memcmp(old_matrix, matrix, sizeof(matrix_t)) || !auto_matrix || refresh)
  {
    memcpy(old_matrix, matrix, sizeof(matrix_t));
    for (int i = 0; i < 6; i++)
      for (int j = 0; j < 6; j++)
      {
        edt_matrix[i][j].update_value(matrix[j][i]);
        SendDlgItemMessage(hwnd, matrix_controls[j][i], EM_SETREADONLY, auto_matrix, 0);
      }
  }
}

void 
ConfigDlg::set_cpu_usage()
{
  /////////////////////////////////////
  // CPU usage

  double cpu_usage;
  winampac3->get_cpu_usage(&cpu_usage);
  dlg_printf(hwnd, IDC_CPU_LABEL, "%i%%", int(cpu_usage*100));
  SendDlgItemMessage(hwnd, IDC_CPU, PBM_SETPOS, int(cpu_usage * 100),  0);
}

///////////////////////////////////////////////////////////////////////////////
// Commands
///////////////////////////////////////////////////////////////////////////////

void 
ConfigDlg::command(int control, int message)
{
  /////////////////////////////////////
  // Matrix controls

  if (message == CB_ENTER)
  {
    matrix_t matrix;
    proc->get_matrix(&matrix);
    bool update_matrix = false;

    for (int i = 0; i < 6; i++)
      for (int j = 0; j < 6; j++)
        if (control == matrix_controls[i][j])
        {
          matrix[j][i] = edt_matrix[i][j].value;
          update_matrix = true;
        }

    if (update_matrix)
    {
      proc->set_matrix(&matrix);
      set_matrix_controls();
    }
  }

  switch (control)
  {
    /////////////////////////////////////
    // Speaker selection

    case IDC_CMB_SPK:
    case IDC_CMB_FORMAT:
      if (message == CBN_SELENDOK)
      {
        int ispk = SendDlgItemMessage(hwnd, IDC_CMB_SPK, CB_GETCURSEL, 0, 0);
        int ifmt = SendDlgItemMessage(hwnd, IDC_CMB_FORMAT, CB_GETCURSEL, 0, 0);

        Speakers spk = list2spk(ispk, ifmt, 0);
        dec->set_user_spk(spk);
        update();
      }
      break;

    /////////////////////////////////////
    // SPDIF if possible

    case IDC_CHK_SPDIF:
    {
      use_spdif = IsDlgButtonChecked(hwnd, IDC_CHK_SPDIF) == BST_CHECKED;
      dec->set_use_spdif(use_spdif);
      update();
      break;
    }

    /////////////////////////////////////
    // SPDIF passthrough

    case IDC_CHK_SPDIF_MPA:
    case IDC_CHK_SPDIF_AC3:
    case IDC_CHK_SPDIF_DTS:
    {
      spdif_pt = 0;
      spdif_pt |= IsDlgButtonChecked(hwnd, IDC_CHK_SPDIF_MPA) == BST_CHECKED? FORMAT_MASK_MPA: 0;
      spdif_pt |= IsDlgButtonChecked(hwnd, IDC_CHK_SPDIF_AC3) == BST_CHECKED? FORMAT_MASK_AC3: 0;
      spdif_pt |= IsDlgButtonChecked(hwnd, IDC_CHK_SPDIF_DTS) == BST_CHECKED? FORMAT_MASK_DTS: 0;
      dec->set_spdif_pt(spdif_pt);
      update();
      break;
    }

    /////////////////////////////////////
    // SPDIF options

    case IDC_CHK_SPDIF_AS_PCM:
    {
      spdif_as_pcm = IsDlgButtonChecked(hwnd, IDC_CHK_SPDIF_AS_PCM) == BST_CHECKED;
      dec->set_spdif_as_pcm(spdif_as_pcm);
      update();
      break;
    }

    case IDC_CHK_SPDIF_ENCODE:
    {
      spdif_encode = IsDlgButtonChecked(hwnd, IDC_CHK_SPDIF_ENCODE) == BST_CHECKED;
      dec->set_spdif_encode(spdif_encode);
      update();
      break;
    }

    case IDC_CHK_SPDIF_STEREO_PT:
    {
      spdif_stereo_pt = IsDlgButtonChecked(hwnd, IDC_CHK_SPDIF_STEREO_PT) == BST_CHECKED;
      dec->set_spdif_stereo_pt(spdif_stereo_pt);
      update();
      break;
    }

    case IDC_CHK_SPDIF_CHECK_SR:
    {
      spdif_check_sr = IsDlgButtonChecked(hwnd, IDC_CHK_SPDIF_CHECK_SR) == BST_CHECKED;
      dec->set_spdif_check_sr(spdif_check_sr);
      update();
      break;
    }

    case IDC_CHK_SPDIF_ALLOW_48:
    {
      spdif_allow_48 = IsDlgButtonChecked(hwnd, IDC_CHK_SPDIF_ALLOW_48) == BST_CHECKED;
      dec->set_spdif_allow_48(spdif_allow_48);
      update();
      break;
    }

    case IDC_CHK_SPDIF_ALLOW_44:
    {
      spdif_allow_44 = IsDlgButtonChecked(hwnd, IDC_CHK_SPDIF_ALLOW_44) == BST_CHECKED;
      dec->set_spdif_allow_44(spdif_allow_44);
      update();
      break;
    }

    case IDC_CHK_SPDIF_ALLOW_32:
    {
      spdif_allow_32 = IsDlgButtonChecked(hwnd, IDC_CHK_SPDIF_ALLOW_32) == BST_CHECKED;
      dec->set_spdif_allow_32(spdif_allow_32);
      update();
      break;
    }

    /////////////////////////////////////
    // Formats

    case IDC_CHK_PCM:
    case IDC_CHK_MPA:
    case IDC_CHK_AC3:
    case IDC_CHK_DTS:
    case IDC_CHK_PES:
    {
      formats = FORMAT_CLASS_PCM_BE; // Always allow DVD LPCM
      formats |= IsDlgButtonChecked(hwnd, IDC_CHK_PCM) == BST_CHECKED? FORMAT_CLASS_PCM: 0;
      formats |= IsDlgButtonChecked(hwnd, IDC_CHK_MPA) == BST_CHECKED? FORMAT_MASK_MPA: 0;
      formats |= IsDlgButtonChecked(hwnd, IDC_CHK_AC3) == BST_CHECKED? FORMAT_MASK_AC3: 0;
      formats |= IsDlgButtonChecked(hwnd, IDC_CHK_DTS) == BST_CHECKED? FORMAT_MASK_DTS: 0;
      formats |= IsDlgButtonChecked(hwnd, IDC_CHK_PES) == BST_CHECKED? FORMAT_MASK_PES: 0;
      dec->set_formats(formats);
      update();
      break;
    }

    /////////////////////////////////////
    // Query sink

    case IDC_CHK_QUERY_SINK:
    {
      query_sink = IsDlgButtonChecked(hwnd, IDC_CHK_QUERY_SINK) == BST_CHECKED;
      dec->set_query_sink(query_sink);
      update();
      break;
    }

    /////////////////////////////////////
    // Force reinit

    case IDC_CHK_REINIT:
    {
      RegistryKey reg(REG_KEY);
      int reinit_samples = IsDlgButtonChecked(hwnd, IDC_CHK_REINIT) == BST_CHECKED? 128: 0;
      reg.set_int32("reinit_samples", reinit_samples);
      update();
      break;
    }

    /////////////////////////////////////
    // Auto gain control

    case IDC_SLIDER_MASTER:
      if (message == TB_THUMBPOSITION || message == TB_ENDTRACK)
      {
        master = db2value(-double(SendDlgItemMessage(hwnd, IDC_SLIDER_MASTER,TBM_GETPOS, 0, 0))/ticks);
        proc->set_master(master);
        update();
      }
      break;

    case IDC_EDT_MASTER:
      if (message == CB_ENTER)
      {
        proc->set_master(db2value(edt_master.value));
        update();
      }
      break;

    case IDC_EDT_ATTACK:
      if (message == CB_ENTER)
      {
        attack = edt_attack.value;
        proc->set_attack(attack);
        update();
      }
      break;

    case IDC_EDT_RELEASE:
      if (message == CB_ENTER)
      {
        release = edt_release.value;
        proc->set_release(release);
        update();
      }
      break;


    /////////////////////////////////////
    // Gain controls

    case IDC_SLIDER_VOICE:
    case IDC_SLIDER_SUR:
    case IDC_SLIDER_LFE:
      if (message == TB_THUMBPOSITION || message == TB_ENDTRACK)
      {
        clev   = db2value(-double(SendDlgItemMessage(hwnd, IDC_SLIDER_VOICE, TBM_GETPOS, 0, 0))/ticks);
        slev   = db2value(-double(SendDlgItemMessage(hwnd, IDC_SLIDER_SUR,   TBM_GETPOS, 0, 0))/ticks);
        lfelev = db2value(-double(SendDlgItemMessage(hwnd, IDC_SLIDER_LFE,   TBM_GETPOS, 0, 0))/ticks);
        proc->set_clev(clev);
        proc->set_slev(slev);
        proc->set_lfelev(lfelev);
        update();
      }
      break;

    case IDC_EDT_VOICE:
    case IDC_EDT_SUR:
    case IDC_EDT_LFE:
      if (message == CB_ENTER)
      {  
        clev   = db2value(edt_voice.value);
        slev   = db2value(edt_sur.value);
        lfelev = db2value(edt_lfe.value);
        proc->set_clev(clev);
        proc->set_slev(slev);
        proc->set_lfelev(lfelev);
        update();
      }
      break;

    /////////////////////////////////////
    // I/O Gains

    case IDC_SLIDER_IN_L:
    case IDC_SLIDER_IN_C:
    case IDC_SLIDER_IN_R:
    case IDC_SLIDER_IN_SL:
    case IDC_SLIDER_IN_SR:
    case IDC_SLIDER_IN_LFE:
      if (message == TB_THUMBPOSITION || message == TB_ENDTRACK)
      {
        for (int ch = 0; ch < NCHANNELS; ch++)
          input_gains[ch] = db2value(-double(SendDlgItemMessage(hwnd, idc_slider_in[ch], TBM_GETPOS, 0, 0))/ticks);

        proc->set_input_gains(input_gains);
        update();
      }
      break;

    case IDC_EDT_IN_L:
    case IDC_EDT_IN_C:
    case IDC_EDT_IN_R:
    case IDC_EDT_IN_SL:
    case IDC_EDT_IN_SR:
    case IDC_EDT_IN_LFE:
      if (message == CB_ENTER)
      {
        for (int ch = 0; ch < NCHANNELS; ch++)
          input_gains[ch] = db2value(edt_in_gains[ch].value);

        proc->set_input_gains(input_gains);
        update();
      }
      break;

    case IDC_SLIDER_OUT_L:
    case IDC_SLIDER_OUT_C:
    case IDC_SLIDER_OUT_R:
    case IDC_SLIDER_OUT_SL:
    case IDC_SLIDER_OUT_SR:
    case IDC_SLIDER_OUT_LFE:
      if (message == TB_THUMBPOSITION || message == TB_ENDTRACK)
      {
        for (int ch = 0; ch < NCHANNELS; ch++)
          output_gains[ch] = db2value(-double(SendDlgItemMessage(hwnd, idc_slider_out[ch], TBM_GETPOS, 0, 0))/ticks);

        proc->set_output_gains(output_gains);
        update();
      }
      break;

    case IDC_EDT_OUT_L:
    case IDC_EDT_OUT_C:
    case IDC_EDT_OUT_R:
    case IDC_EDT_OUT_SL:
    case IDC_EDT_OUT_SR:
    case IDC_EDT_OUT_LFE:
      if (message == CB_ENTER)
      {
        for (int ch = 0; ch < NCHANNELS; ch++)
          output_gains[ch] = db2value(edt_out_gains[ch].value);

        proc->set_output_gains(output_gains);
        update();
      }
      break;


    /////////////////////////////////////
    // Delay

    case IDC_CHK_DELAY:
    {
      delay = (SendDlgItemMessage(hwnd, IDC_CHK_DELAY, BM_GETCHECK, 0, 0) == BST_CHECKED);
      proc->set_delay(delay);
      update();
      break;
    }

    case IDC_EDT_DL:
    case IDC_EDT_DC:
    case IDC_EDT_DR:
    case IDC_EDT_DSL:
    case IDC_EDT_DSR:
    case IDC_EDT_DLFE:
      if (message == CB_ENTER)
      {
        for (int ch = 0; ch < NCHANNELS; ch++)
          delays[ch] = (float)edt_delay[ch].value;

        proc->set_delays(delays);
        update();
      }
      break;

    case IDC_CMB_UNITS:
      if (message == CBN_SELENDOK)
      {
        delay_units = list2units(SendDlgItemMessage(hwnd, IDC_CMB_UNITS, CB_GETCURSEL, 0, 0));
        proc->set_delay_units(delay_units);
        update();
      }
      break;

    /////////////////////////////////////
    // Syncronization

    case IDC_EDT_TIME_SHIFT:
      if (message == CB_ENTER)
      {
        time_shift = vtime_t(edt_time_shift.value) / 1000;
        dec->set_time_shift(time_shift);
        update();
      }
      break;

    case IDC_SLIDER_TIME_SHIFT:
      if (message == TB_THUMBPOSITION || message == TB_ENDTRACK)
      {
        time_shift = vtime_t(SendDlgItemMessage(hwnd, IDC_SLIDER_TIME_SHIFT, TBM_GETPOS, 0, 0)) / 1000;
        dec->set_time_shift(time_shift);
        update();
      }
      break;

    case IDC_CHK_JITTER:
    {
      dejitter = (SendDlgItemMessage(hwnd, IDC_CHK_JITTER, BM_GETCHECK, 0, 0) == BST_CHECKED);
      dec->set_dejitter(dejitter);
      update();
      break;
    }


    /////////////////////////////////////
    // DRC

    case IDC_CHK_DRC:
    {
      drc = (SendDlgItemMessage(hwnd, IDC_CHK_DRC, BM_GETCHECK, 0, 0) == BST_CHECKED);
      proc->set_drc(drc);
      update();
      break;
    }

    case IDC_SLIDER_DRC_POWER:
      if (message == TB_THUMBPOSITION || message == TB_ENDTRACK)
      {
        drc_power = -double(SendDlgItemMessage(hwnd, IDC_SLIDER_DRC_POWER, TBM_GETPOS, 0, 0)) / ticks;
        proc->set_drc_power(drc_power);
        update();
      }
      break;

    case IDC_EDT_DRC_POWER:
      if (message == CB_ENTER)
      {
        drc_power = edt_drc_power.value;
        proc->set_drc_power(drc_power);
        update();
      }
      break;


    /////////////////////////////////////
    // Bass redirection

    case IDC_CHK_BASS_REDIR:
    {
      bass_redir = (SendDlgItemMessage(hwnd, IDC_CHK_BASS_REDIR, BM_GETCHECK, 0, 0) == BST_CHECKED);
      proc->set_bass_redir(bass_redir);
      update();
      break;
    }

    case IDC_EDT_BASS_FREQ:
      if (message == CB_ENTER)
      {
        bass_freq = (int)edt_bass_freq.value;
        proc->set_bass_freq(bass_freq);
        update();
      }
      break;


    /////////////////////////////////////
    // Flags

    case IDC_CHK_AUTO_MATRIX:
    {
      auto_matrix = (SendDlgItemMessage(hwnd, IDC_CHK_AUTO_MATRIX, BM_GETCHECK, 0, 0) == BST_CHECKED);
      proc->set_auto_matrix(auto_matrix);
      refresh = true;
      update();
      break;
    }

    case IDC_CHK_NORM_MATRIX:
    {
      normalize_matrix = (SendDlgItemMessage(hwnd, IDC_CHK_NORM_MATRIX, BM_GETCHECK, 0, 0) == BST_CHECKED);
      proc->set_normalize_matrix(normalize_matrix);
      update();
      break;
    }

    case IDC_CHK_AUTO_GAIN:
    {
      auto_gain = (SendDlgItemMessage(hwnd, IDC_CHK_AUTO_GAIN, BM_GETCHECK, 0, 0) == BST_CHECKED);
      proc->set_auto_gain(auto_gain);
      update();
      break;
    }

    case IDC_CHK_NORMALIZE:
    {
      normalize = (SendDlgItemMessage(hwnd, IDC_CHK_NORMALIZE, BM_GETCHECK, 0, 0) == BST_CHECKED);
      proc->set_normalize(normalize);
      update();
      break;
    }

    case IDC_CHK_EXPAND_STEREO:
    {
      expand_stereo = (SendDlgItemMessage(hwnd, IDC_CHK_EXPAND_STEREO, BM_GETCHECK, 0, 0) == BST_CHECKED);
      proc->set_expand_stereo(expand_stereo);
      update();
      break;
    }

    case IDC_CHK_VOICE_CONTROL:
    {
      voice_control = (SendDlgItemMessage(hwnd, IDC_CHK_VOICE_CONTROL, BM_GETCHECK, 0, 0) == BST_CHECKED);
      proc->set_voice_control(voice_control);
      update();
      break;
    }

    /////////////////////////////////////
    // Preset

    case IDC_CMB_PRESET:
      if (message == CBN_SELENDOK)
      {
        char buf[256];
        char preset[256];
        SendDlgItemMessage(hwnd, IDC_CMB_PRESET, CB_GETLBTEXT, SendDlgItemMessage(hwnd, IDC_CMB_PRESET, CB_GETCURSEL, 0, 0), (LONG)preset);
        SendDlgItemMessage(hwnd, IDC_CMB_PRESET, WM_SETTEXT, 0, (LONG)preset);
        sprintf(buf, REG_KEY_PRESET"\\%s", preset);

        RegistryKey reg(buf);
        dec->load_params(&reg, PRESET_ALL);
        update();
      }
      if (message == CB_ENTER)
      {
        char buf[256];
        char preset[256];
        SendDlgItemMessage(hwnd, IDC_CMB_PRESET, WM_GETTEXT, 256, (LONG)preset);
        sprintf(buf, REG_KEY_PRESET"\\%s", preset);

        RegistryKey reg;
        reg.create_key(buf);
        dec->save_params(&reg, PRESET_PRESET);
        update();
      }

      break;

    case IDC_BTN_PRESET_SAVE:
    {
      char buf[256];
      char preset[256];
      SendDlgItemMessage(hwnd, IDC_CMB_PRESET, WM_GETTEXT, 256, (LONG)preset);
      sprintf(buf, REG_KEY_PRESET"\\%s", preset);

      RegistryKey reg;
      reg.create_key(buf);
      dec->save_params(&reg, PRESET_PRESET);
      update();
      break;
    }

    case IDC_BTN_PRESET_DELETE:
    {
      char buf[256];
      char preset[256];
      SendDlgItemMessage(hwnd, IDC_CMB_PRESET, WM_GETTEXT, 256, (LONG)preset);

      sprintf(buf, "Are you sure you want to delete '%s' preset?", preset);
      if (MessageBox(hwnd, buf, "Delete confirmation", MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
      {     
        sprintf(buf, REG_KEY_PRESET"\\%s", preset);
        delete_reg_key(buf, HKEY_CURRENT_USER);
        SendDlgItemMessage(hwnd, IDC_CMB_PRESET, WM_SETTEXT, 0, (LONG)"");
        update();
      }
      break;
    }
/*
    case IDC_BTN_PRESET_FILE:
    {
      char filename[MAX_PATH];

      if FAILED(dec->get_config_file(filename, MAX_PATH))
        filename[0] = 0;

      FileDlg dlg(hwnd, filename);
      switch (dlg.exec())
      {
      case IDC_BTN_FILE_SAVE:
        if (dlg.preset) dec->save_params(dlg.filename, true);
        if (dlg.matrix) dec->save_matrix(dlg.filename, true);
        if (dlg.delay)  dec->save_delay (dlg.filename, true);
        if (dlg.eq)     dec->save_eq9(dlg.filename, true);
        break;

      case IDC_BTN_FILE_LOAD:
        if (dlg.preset) dec->load_params(dlg.filename, true);
        if (dlg.matrix) dec->load_matrix(dlg.filename, true);
        if (dlg.delay)  dec->load_delay (dlg.filename, true);
        if (dlg.eq)     dec->load_eq9(dlg.filename, true);
        break;
      }
      break;
    }
*/
    /////////////////////////////////////
    // Matrix combo box 

    case IDC_CMB_MATRIX:
      if (message == CBN_SELENDOK)
      {
        char buf[256];
        char preset[256];
        SendDlgItemMessage(hwnd, IDC_CMB_MATRIX, CB_GETLBTEXT, SendDlgItemMessage(hwnd, IDC_CMB_MATRIX, CB_GETCURSEL, 0, 0), (LONG)preset);
        SendDlgItemMessage(hwnd, IDC_CMB_MATRIX, WM_SETTEXT, 0, (LONG)preset);
        sprintf(buf, REG_KEY_MATRIX"\\%s", preset);

        proc->set_auto_matrix(false);
        RegistryKey reg(buf);
        dec->load_params(&reg, PRESET_MATRIX);
        update();
      }
      if (message == CB_ENTER)
      {
        char buf[256];
        char preset[256];
        SendDlgItemMessage(hwnd, IDC_CMB_MATRIX, WM_GETTEXT, 256, (LONG)preset);
        sprintf(buf, REG_KEY_MATRIX"\\%s", preset);

        RegistryKey reg;
        reg.create_key(buf);
        dec->save_params(&reg, PRESET_MATRIX);
        update();
      }
      break;

    case IDC_BTN_MATRIX_SAVE:
    {
      char buf[256];
      char preset[256];
      SendDlgItemMessage(hwnd, IDC_CMB_MATRIX, WM_GETTEXT, 256, (LONG)preset);
      sprintf(buf, REG_KEY_MATRIX"\\%s", preset);

      RegistryKey reg;
      reg.create_key(buf);
      dec->save_params(&reg, PRESET_MATRIX);
      update();
      break;
    }

    case IDC_BTN_MATRIX_DELETE:
    {
      char buf[256];
      char preset[256];
      SendDlgItemMessage(hwnd, IDC_CMB_MATRIX, WM_GETTEXT, 256, (LONG)preset);

      sprintf(buf, "Are you sure you want to delete '%s' matrix?", preset);
      if (MessageBox(hwnd, buf, "Delete confirmation", MB_ICONEXCLAMATION | MB_YESNO) == IDYES)
      {     
        sprintf(buf, REG_KEY_MATRIX"\\%s", preset);
        delete_reg_key(buf, HKEY_CURRENT_USER);
        SendDlgItemMessage(hwnd, IDC_CMB_MATRIX, WM_SETTEXT, 0, (LONG)"");
        proc->set_auto_matrix(true);
        update();
      }
      break;
    }
  }
}


