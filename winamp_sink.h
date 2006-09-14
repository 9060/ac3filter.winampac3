#ifndef WINAMP_SINK_H
#define WINAMP_SINK_H

#include "filter.h"
#include "renderer.h"
#include "winamp.h"

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
  int  vol;
  int  pan;

public:
  WinampSink(In_Module *in);
  ~WinampSink();

  bool open(Speakers _spk);
  void close();

  /////////////////////////////////////////////////////////
  // Playback control

  virtual void pause()           { if (out && !paused) paused = !out->Pause(1); }
  virtual void unpause()         { if (out && !paused) paused = !out->Pause(0); }
  virtual bool is_paused() const { return paused; }

  virtual void stop()            { close(); }
  virtual void flush()           { close(); }

  virtual vtime_t get_playback_time() const { return out? time - (out->GetWrittenTime() - out->GetOutputTime()) / 1000: 0; };
/*
  virtual size_t  get_buffer_size()   const;
  virtual vtime_t get_buffer_time()   const;
  virtual size_t  get_data_size()     const;
  virtual vtime_t get_data_time()     const;

  virtual double get_vol()            const { return vol; }
  virtual void   set_vol(double _vol)       { vol = _vol; if (out) out->SetVolume(); }

  virtual double get_pan()            const { return pan; }
  virtual void   set_pan(double _pan);      { pan = _pan; if (out) out->SetPan(); }
*/
  /////////////////////////////////////////////////////////
  // Sink interface

  virtual bool query_input(Speakers _spk) const;
  virtual bool set_input(Speakers _spk);
  virtual Speakers get_input() const;
  virtual bool process(const Chunk *_chunk);

};


#endif
