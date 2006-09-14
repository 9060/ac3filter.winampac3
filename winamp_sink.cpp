#include "winamp_sink.h"

WinampSink::WinampSink(In_Module *_in)
{
  in = _in;
  out = 0;

  spk = spk_unknown;
  size2time = 0;
  time = 0;

  paused = false;
  vol = 0;
  pan = 0;
}

WinampSink::~WinampSink()
{
  close();
}

bool
WinampSink::open(Speakers _spk)
{
  AutoLock autolock(&lock);

  close();

  if (!in->outMod)
    return false;

  if (!query_input(_spk)) 
    return false;

  int nch = _spk.nch();
  int sample_rate = _spk.sample_rate;
  int bps = 0;

  switch (_spk.format)
  {
    case FORMAT_PCM16: bps = 16; size2time = 1.0 / (2 * nch * _spk.sample_rate); break;
    case FORMAT_PCM24: bps = 24; size2time = 1.0 / (3 * nch * _spk.sample_rate); break;
    case FORMAT_PCM32: bps = 32; size2time = 1.0 / (4 * nch * _spk.sample_rate); break;
    default: return false;
  }

  if (in->outMod->Open(sample_rate, nch, bps, -1, -1))
    out = in->outMod;
  else
    return false;

  out->SetVolume(-666);
  spk = _spk;
  return true;
}

void
WinampSink::close()
{
  AutoLock autolock(&lock);

  if (out)
    out->Close();

  out = 0;
  spk = spk_unknown;
  size2time = 0;
  time = 0;
  paused = false;
}

///////////////////////////////////////////////////////////////////////////////
// PlaybackControl interface

void
WinampSink::pause()
{ 
  AutoLock autolock(&lock);
  if (out && !paused) 
    paused = !out->Pause(1); 
}

void
WinampSink::unpause()
{ 
  AutoLock autolock(&lock);
  if (out && !paused) 
    paused = !out->Pause(0);
}

bool
WinampSink::is_paused() const 
{ 
  return paused; 
}

void
WinampSink::stop()
{ 
  close(); 
}

void
WinampSink::flush()
{
  // We have to lock following section because we use output module
  // But we have to unlock before Sleep because we do not want to 
  // hang during flushing.

  lock.lock();
  int writed_ms = out->GetWrittenTime() - out->GetOutputTime();
  lock.unlock();

  // Wait until playback finishes and close output.
  Sleep(writed_ms);
  close(); 
}

vtime_t
WinampSink::get_playback_time() const
{
  AutoLock autolock(&lock);
  return out? time - (out->GetWrittenTime() - out->GetOutputTime()) / 1000: 0; 
}


double
WinampSink::get_vol() const
{
  return vol;
}

void
WinampSink::set_vol(double _vol)
{
  AutoLock autolock(&lock);

  vol = _vol;
  if (vol < -100) vol = -100;
  if (vol > 0) vol = 0;

  if (out)
    out->SetVolume((LONG)((vol+100) / 100 * 255));
}

double
WinampSink::get_pan() const
{
  return pan;
}

void
WinampSink::set_pan(double _pan)
{
  AutoLock autolock(&lock);

  pan = _pan;
  if (pan > 100) pan = 100;
  if (pan < -100) pan = -100;

  if (out)
    out->SetPan((LONG)(pan / 100 * 128));
}



///////////////////////////////////////////////////////////////////////////////
// Sink interface

bool
WinampSink::query_input(Speakers _spk) const
{
  switch (_spk.format)
  {
    case FORMAT_PCM16: break;
    case FORMAT_PCM24: break;
    case FORMAT_PCM32: break;
    default: return false;
  }

  switch (_spk.mask)
  {
    case MODE_1_0: break;
    case MODE_2_0: break;
    case MODE_2_2: break;
    case MODE_3_2: break;
    case MODE_5_1: break;
    default: return false;
  }

  if (_spk.sample_rate <= 0)
    return false;

  return true;
}

bool
WinampSink::set_input(Speakers _spk)
{
  return open(_spk);
}

Speakers
WinampSink::get_input() const
{
  return spk;
}

bool
WinampSink::process(const Chunk *_chunk)
{
  if (_chunk->is_dummy())
    return true;

  if (_chunk->spk != spk)
    if (!set_input(_chunk->spk))
      return false;

  if (_chunk->sync)
    time = _chunk->time;

  Chunk c = *_chunk;
  size_t size = 0;
  while (c.size)
  {
    size = out->CanWrite();
    while (size < 65536 && size < c.size)
    {
      Sleep(100);
      size = out->CanWrite();
    }

    if (size > c.size)
      size = c.size;
    
    out->Write((char *)c.rawdata, size);
    time += size * size2time;
    c.drop(size);
  }

  if (_chunk->eos)
    flush();

  return true;
}
