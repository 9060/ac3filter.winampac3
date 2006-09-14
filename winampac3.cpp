#include "winampac3.h"


WinampAC3::WinampAC3(In_Module *_mod):
wa_sink(_mod), ds_sink(0)
{
  mod      = _mod;

  isink    = 0;
  reinit   = 0;

  sink     = &wa_sink;
  ctrl     = &wa_sink;

  seek_pos = -1;
  state    = state_stop;

  ev_play  = CreateEvent(0, true, false, 0);
  ev_stop  = CreateEvent(0, true, true,  0);

  RegistryKey reg(REG_KEY);
  reg.get_int32("sink", isink);
  reg.get_int32("reinit", reinit);

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

  if (file.open(&ac3_parser, _filename) && file.probe())
  {}
  else if (file.open(&dts_parser, _filename) && file.probe())
  {}
  else
    return false;

  file.stats();

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

  if (!sink->set_input(out_spk))
    if (!sink->set_input(Speakers(FORMAT_PCM16, MODE_STEREO, out_spk.sample_rate)))
      return false;

  dec.set_sink(sink);
  if (!dec.set_input(file.get_spk()))
    return false;

  /////////////////////////////////////////////////////////
  // Run!

  state = state_start;
  SetEvent(ev_play);
  return true;
}

bool 
WinampAC3::stop()
{
  // syncronisation
  ResetEvent(ev_play);
  WaitForSingleObject(ev_stop, 2000);

  seek_pos = -1;
  state = state_stop;
  
  ctrl->stop();
  file.close();
  dec.reset();

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
  ctrl->stop();
}

void
WinampAC3::unpause()
{
  ctrl->unpause();
}

bool
WinampAC3::is_paused()
{
  return ctrl->is_paused();
}

// Positioning

void
WinampAC3::seek(int _seek_pos)
{
  seek_pos = _seek_pos;
}

int
WinampAC3::get_length()
{
  return (int)file.get_size(FileParser::ms);
}

int
WinampAC3::get_pos()
{
  return (int)(ctrl->get_playback_time() * 1000);
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

    // seeking
    if (seek_pos != -1)
    {
      ctrl->stop();
      file.seek(int(seek_pos), FileParser::ms);
      state = state_start;
      seek_pos = -1;
    }

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
        else
        {
          if (file.load_frame())
          {
            chunk.set_rawdata(file.get_spk(), file.get_frame(), file.get_frame_size());
            chunk.set_sync(true, (vtime_t)file.get_pos(FileParser::ms)/1000);
            dec.process(&chunk);
          }
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
  RegistryKey reg;
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
  RegistryKey reg;
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
