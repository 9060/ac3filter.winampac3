#include "winampac3.h"


WinampAC3::WinampAC3(In_Module *_mod):
dsound(0)
{
  mod      = _mod;

  parser   = 0;
  sink     = &dsound;

  seek_pos = -1;
  state    = state_stop;

  ev_play  = CreateEvent(0, true, false, 0);
  ev_stop  = CreateEvent(0, true, true,  0);

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

  // open file
  if (file.open(&ac3_parser, _filename) && file.probe())
    parser = &ac3_parser;
  else if (file.open(&dts_parser, _filename) && file.probe())
    parser = &dts_parser;
  else
  {
    parser = 0;
    return false;
  }
  file.stats();

  // determine output format
  Speakers user_spk;
  Speakers out_spk;

  out_spk = file.get_spk();
  dec.get_user_spk(&user_spk);

  if (user_spk.format == FORMAT_UNKNOWN)
  {
    out_spk.format = FORMAT_PCM16;
    out_spk.level = 32768;
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

  // setup
  dec.set_input(file.get_spk());
  dsound.set_input(out_spk);
  dec.set_sink(&dsound);

  // run!
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

  pos      = 0;
  seek_pos = -1;
  state = state_stop;
  
  dsound.stop();
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
  dsound.pause();
}

void
WinampAC3::unpause()
{
  dsound.unpause();
}

bool
WinampAC3::is_paused()
{
  return dsound.is_paused();
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
  return (int)(dsound.get_playback_time() * 1000);
}


DWORD 
WinampAC3::process()
{
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
      pos      = seek_pos;
      seek_pos = -1;

      dsound.stop();
      file.seek(int(pos), FileParser::ms);
      state = state_start;
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
          state = state_stop;
        else
        {
          if (file.load_frame())
          {
            Chunk chunk(file.get_spk(), file.get_frame(), file.get_frame_size());
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

        Chunk chunk;
        dec.get_chunk(&chunk);
        sink->process(&chunk);
      }
    }
  }
  cpu.stop();
  return 0;
}


/////////////////////////////////////////////////////////
// User interface (used in config dialog)

STDMETHODIMP 
WinampAC3::get_playback_time(vtime_t *_time)
{
  *_time = dsound.get_playback_time();
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
