#include "winampac3.h"
#include "parsers\ac3\ac3_header.h"
#include "parsers\dts\dts_header.h"
#include "parsers\mpa\mpa_header.h"
#include "parsers\spdif\spdif_header.h"
#include "parsers\multi_header.h"

WinampAC3::WinampAC3(In_Module *_mod):
wa_sink(_mod), dec(2048)
{
  mod      = _mod;

  tray     = false;
  isink    = 0;
  reinit   = 0;

  sink     = &wa_sink;
  ctrl     = &wa_sink;

  base_pos = 0;
  state    = state_stop;

  ev_play  = CreateEvent(0, true, false, 0);
  ev_stop  = CreateEvent(0, true, true,  0);

  RegistryKey reg(REG_KEY);
  reg.get_bool ("tray", tray);
  reg.get_int32("sink", isink);
  reg.get_int32("reinit", reinit);

  const HeaderParser *parser_list[] = { &spdif_header, &ac3_header, &dts_header, &mpa_header };
  header_parser.set_parsers(parser_list, array_size(parser_list));

  ds_sink.open_dsound(0);
  create(false);
}

WinampAC3::~WinampAC3()
{
  stop();
  f_terminate = true;               
  state = state_stop;
  SetEvent(ev_play);

  CloseHandle(ev_play);
  CloseHandle(ev_stop);
}

IDecoder *
WinampAC3::get_decoder()
{
  return &dec;
}

IAudioProcessor *
WinampAC3::get_audio_processor()
{
  return &dec;
}


bool 
WinampAC3::play(const char *_filename)
{
  if (!stop()) return false;

  /////////////////////////////////////////////////////////
  // Open file

  if (!file.open(_filename, &header_parser, 1000000))
    return false;

  if (!file.stats())
    return false;

  if (!file.load_frame())
    return false;

  /////////////////////////////////////////////////////////
  // Determine output format

  Speakers user_spk;
  Speakers out_spk;

  out_spk = file.get_spk();
  dec.get_user_spk(&user_spk);

  if (user_spk.format == FORMAT_UNKNOWN)
  {
    out_spk.format = FORMAT_PCM16;
    out_spk.level = 32767;
  }
  else
  {
    out_spk.format = user_spk.format;
    out_spk.level = user_spk.level;
  }

  if (user_spk.mask)
    out_spk.mask = user_spk.mask;

  if (user_spk.sample_rate)
    out_spk.sample_rate = user_spk.sample_rate;

  /////////////////////////////////////////////////////////
  // Setup

  switch (isink)
  {
  case SINK_DSOUND:
    sink     = &ds_sink;
    ctrl     = &ds_sink;
    break;

  case SINK_WINAMP:
    sink     = &wa_sink;
    ctrl     = &wa_sink;
    break;
  }

  dec.set_sink(sink);
  if (!dec.set_input(file.get_spk()))
    return false;

  /////////////////////////////////////////////////////////
  // Reset the file parser
  // (otherwise we will loose the first frame that is
  // aleready loaded for setup)

  file.seek(0);

  /////////////////////////////////////////////////////////
  // Run!

  state = state_start;
  SetEvent(ev_play);
  return true;
}

bool 
WinampAC3::stop()
{
  // Unblock processing and syncronize with working thread
  ResetEvent(ev_play);
  ctrl->stop();
  WaitForSingleObject(ev_stop, 2000);

  // Close everything

  base_pos = 0;
  state = state_stop;
  
  file.close();
  wa_sink.close();
  ds_sink.close();

  return true;
}

const char *
WinampAC3::get_filename()
{
  return file.get_filename();
}

void
WinampAC3::pause()
{
  ctrl->pause();
}

void
WinampAC3::unpause()
{
  if (reinit)
  {
    Speakers sink_spk = sink->get_input();
    bool use_spdif;
    dec.get_use_spdif(&use_spdif);
    if (use_spdif)
    {
      // Unblock processing and syncronize with working thread
      ResetEvent(ev_play);
      ctrl->stop();
      WaitForSingleObject(ev_stop, 2000);

      // Close current SPDIF output and run
      dec.reset();
      ds_sink.close();
      SetEvent(ev_play);
    }
  }

  ctrl->unpause();
}

bool
WinampAC3::is_paused()
{
  return ctrl->is_paused();
}

// Positioning

void
WinampAC3::seek(int _pos)
{
  // Unblock processing and syncronize with working thread
  ResetEvent(ev_play);
  ctrl->stop();
  WaitForSingleObject(ev_stop, 2000);

  // Reopen SPDIF output
  if (reinit)
  {
    Speakers sink_spk = sink->get_input();
    bool use_spdif;
    dec.get_use_spdif(&use_spdif);
    if (use_spdif)
    {
      // Close current SPDIF output
      dec.reset();
      ds_sink.close();
    }
  }

  // Seek and run
  base_pos = _pos;
  file.seek(vtime_t(base_pos) / 1000, FileParser::time);
  state = state_start;
  SetEvent(ev_play);
}

int
WinampAC3::get_length()
{
  return (int)(file.get_size(FileParser::time) * 1000);
}

int
WinampAC3::get_pos()
{
  return base_pos + (int)(ctrl->get_playback_time() * 1000);
}

// Playback options

void
WinampAC3::set_volume(int volume)
{
  // Winamp volume is 0-255 but PlaybackControl uses decibels.
  // Therefore we need to convert units:
  // 0 -> -100dB
  // 255 -> 0dB

  ctrl->set_vol(double(volume) / 255 * 100 - 100);
}

void
WinampAC3::set_pan(int pan)
{
  // Winamp pan is -128 - +128 but PlaybackControl uses decibels.
  // Therefore we need to convert units:
  // -128 -> -100dB
  // +128 -> +100dB

  ctrl->set_pan(double(pan) / 128 * 100);
}



DWORD 
WinampAC3::process()
{
  Chunk chunk;

  cpu.start();
  while (1)
  {
    ///////////////////////////////////////////////////////
    // Syncronization with control thread

    SetEvent(ev_stop);
    WaitForSingleObject(ev_play, INFINITE);
    ResetEvent(ev_stop);

    // termination
    if (f_terminate)
      return 0;

    ///////////////////////////////////////////////////////
    // Now we have all decoding objects live and initialized

    switch (state)
    {
    case state_start:
      // here we can init something in future...
      state = state_process;
      break;

    case state_stop:
      // here we can uninit something in future...
      PostMessage(mod->hMainWindow, WM_WA_MPEG_EOF, 0, 0);
      ResetEvent(ev_play);
      break;

    case state_process:
      if (dec.is_empty())
      {
        ///////////////////////////////
        // decoder is empty
        // fill decoder

        if (file.eof())
        {
          chunk.set_empty(file.get_spk(), false, 0, true);
          dec.process(&chunk);
          state = state_flush;
        }
        else if (file.load_frame())
        {
          chunk.set_rawdata(file.get_spk(), file.get_frame(), file.get_frame_size());
          dec.process(&chunk);
        }
      }
      else
      {
        ///////////////////////////////
        // decoder is full
        // do output

        dec.get_chunk(&chunk);
        sink->process(&chunk);
      }
      break;

    case state_flush:
      if (dec.is_empty())
      {
        state = state_stop;
      }
      else
      {
        dec.get_chunk(&chunk);
        sink->process(&chunk);
      }
      break;
    }
  }
  cpu.stop();
  return 0;
}


/////////////////////////////////////////////////////////
// User interface (used in config dialog)

// Tray icon

STDMETHODIMP WinampAC3::get_tray(bool *_tray)
{
  if (_tray)
    *_tray = tray;
  return S_OK;
}

STDMETHODIMP WinampAC3::set_tray(bool _tray)
{
  tray = _tray;
  RegistryKey reg(REG_KEY);
  reg.set_bool("tray", tray);
/*
  /////////////////////////////////////////////////////////
  // Show tray icon if enabled.
  //
  // We cannot hide tray icon when user disables this
  // option because it may be disabled from config dialog
  // called from tray icon. During hiding config dialog
  // is destructed and caller of this function became
  // invalid.

  if (tray)
    tray_ctl.show();
*/
  return S_OK;
}

// Setup sink used for output (SINK_XXXX constants)

STDMETHODIMP WinampAC3::get_sink(int *_sink, int *_current_sink)
{
  if (_sink)
    *_sink = isink;
  
  if (_current_sink)
  {
    *_current_sink = -1;
    if (sink == &wa_sink) *_current_sink = SINK_WINAMP;
    if (sink == &ds_sink) *_current_sink = SINK_DSOUND;
  }

  return S_OK;
}

STDMETHODIMP WinampAC3::set_sink(int  _sink)
{
  switch (_sink)
  {
    case 0: break;
    case 1: break;
    default: return E_FAIL;
  }

  isink = _sink;
  RegistryKey reg(REG_KEY);
  reg.set_int32("sink", isink);
  return S_OK;
}

// Reinit sound card after pause option

STDMETHODIMP WinampAC3::get_reinit(int *_reinit)
{
  if (_reinit)
    *_reinit = reinit;
  return S_OK;
}

STDMETHODIMP WinampAC3::set_reinit(int  _reinit)
{
  reinit = _reinit;
  RegistryKey reg(REG_KEY);
  reg.set_int32("reinit", reinit);
  return S_OK;
}

// Various info

STDMETHODIMP 
WinampAC3::get_playback_time(vtime_t *_time)
{
  *_time = ctrl->get_playback_time();
  return S_OK;
}

// CPU usage
STDMETHODIMP 
WinampAC3::get_cpu_usage(double *_cpu_usage)
{
  *_cpu_usage = cpu.usage();
  return S_OK;
}

// Build and environment info
STDMETHODIMP
WinampAC3::get_env(char *_buf, int _size)
{
  const char *env = valib_build_info();
  int len = strlen(env) + 1;
  memcpy(_buf, env, MIN(_size, len));
  cr2crlf(_buf, _size);
  return S_OK;
}
