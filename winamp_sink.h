/*
  Sink that uses winamp output plugin for audio playback.

  Winamp fills 'outMod' field at input plugin structure only when input plugin
  works. Therefore we must use it only when playback is started. When playback
  is finished (after flush() or stop() call) we must release output and forget
  about it. For next playback winamp may specify another output, therefore we
  should not keep output plugin open or try to use for next playback without
  complete reinit.

  Therefore flush() and stop() functions actually close audio output and zero
  'out' member. Because of this we must serialize all functions that use it.
  Output plugin is used in both working thread (Sink interfacel) and control
  thread (Playback control). To serialize all this function calls a critical
  section 'lock' is used.
*/

#ifndef WINAMP_SINK_H
#define WINAMP_SINK_H

#include "filter.h"
#include "renderer.h"
#include "winamp.h"
#include "win32\thread.h"

// Winamp sink
class WinampSink : public Sink, public PlaybackControl
{
protected:
  In_Module  *in;
  Out_Module *out;

  Speakers spk;
  double   size2time;
  vtime_t  time;

  bool paused;
  double vol;
  double pan;

  mutable CritSec lock;

public:
  WinampSink(In_Module *in);
  ~WinampSink();

  /////////////////////////////////////////////////////////
  // Own interface
  // This functions may be called from both control and 
  // working threads.

  bool open(Speakers _spk);
  void close();

  /////////////////////////////////////////////////////////
  // Playback control
  // This functions are called from control thread

  virtual void pause();
  virtual void unpause();
  virtual bool is_paused() const;

  virtual void stop();
  virtual void flush();

  virtual vtime_t get_playback_time() const;
/*
  virtual size_t  get_buffer_size()   const;
  virtual vtime_t get_buffer_time()   const;
  virtual size_t  get_data_size()     const;
  virtual vtime_t get_data_time()     const;
*/
  virtual double get_vol() const;
  virtual void   set_vol(double _vol);

  virtual double get_pan() const;
  virtual void   set_pan(double _pan);

  /////////////////////////////////////////////////////////
  // Sink interface
  // This functions are called from working thread

  virtual bool query_input(Speakers _spk) const;
  virtual bool set_input(Speakers _spk);
  virtual Speakers get_input() const;
  virtual bool process(const Chunk *_chunk);

};

#endif
