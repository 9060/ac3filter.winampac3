/*
  WinampSink
  ==========

  Sink that uses winamp output plugin for audio playback.

  Winamp fills 'outMod' field at input plugin structure only when input plugin
  works. Therefore we must use it only when playback is started. When playback
  is finished (close() call) we must release output and forget about it.
  For next playback winamp may specify another output, therefore we should not
  keep output plugin open or try to use for next playback without reinit.

  Output plugin is used in both working thread (Sink interfacel) and control
  thread (Playback control). To serialize all this function calls a critical
  section 'output_lock' is used.

  Blocking functions (process() and flush()) cannot take output lock for a long
  time because it may block control thread and lead to deadlock. Therefore to
  serialize playback functions 'playback_lock' is used.

  stop() must force blocking functions to finish. To signal these functions to
  unblock 'ev_stop' is used. Blocking functions must wait on this event and
  stop execution immediately when signaled.
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

  mutable CritSec output_lock;
  mutable CritSec playback_lock;
  HANDLE  ev_stop;

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
  // flush() and stop() may be called from working thread
  // (from set_input() and process() calls)

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
  // This functions are called only from working thread

  virtual bool query_input(Speakers _spk) const;
  virtual bool set_input(Speakers _spk);
  virtual Speakers get_input() const;
  virtual bool process(const Chunk *_chunk);

};

#endif
