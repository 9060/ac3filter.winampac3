#ifndef AC3THREAD_H
#define AC3THREAD_H

#include "guids.h"
#include "winamp.h"
#include "win32\thread.h"
#include "win32\cpu.h"

#include "parsers\file_parser.h"
#include "parsers\ac3\ac3_parser.h"
#include "parsers\dts\dts_parser.h"

#include "com_dec.h"
#include "sink\sink_dsound.h"

///////////////////////////////////////////////////////////////////////////////
//
// Decoder Thread
//
///////////////////////////////////////////////////////////////////////////////

class WinampAC3 : public IWinampAC3, public Thread
{
protected:
  In_Module  *mod;
  CPUMeter    cpu;

  // Decoding process objects
  FileParser  file;
  Parser     *parser;
  AC3Parser   ac3_parser;
  DTSParser   dts_parser;
  COMDecoder  dec;

  Sink       *sink;
  DSoundSink  dsound;

  HANDLE      ev_play;
  HANDLE      ev_stop;

  int         seek_pos;
  double      pos;

  enum { state_start, state_stop, state_process }
              state;

public:
  WinampAC3(In_Module *mod);
  ~WinampAC3();

  DWORD process();

  /////////////////////////////////////////////////////////
  // Winamp control interface

  // Playback control
  virtual bool play(const char *filename);
  virtual bool stop();
  virtual const char *get_filename();

  virtual void pause();
  virtual void unpause();
  virtual bool is_paused();

  // Positioning
  virtual void  seek(int seek_pos);
  virtual int   get_length();
  virtual int   get_pos();

  /////////////////////////////////////////////////////////
  // Other interfaces

  virtual IDecoder        *get_decoder();
  virtual IAudioProcessor *get_audio_processor();

  /////////////////////////////////////////////////////////
  // User interface (used in config dialog)

  STDMETHODIMP get_playback_time(vtime_t *time);
  STDMETHODIMP get_cpu_usage(double *cpu_usage);
  STDMETHODIMP get_env(char *buf, int size);
};


#endif
