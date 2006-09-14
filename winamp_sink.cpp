#include "winamp_sink.h"

const int wa_order[5][6] = 
{
  { CH_M,   CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE },  // mono
  { CH_L,   CH_R,     CH_NONE,  CH_NONE,  CH_NONE,  CH_NONE },  // stereo
  { CH_L,   CH_R,     CH_SL,    CH_SR,    CH_NONE,  CH_NONE },  // quadro
  { CH_L,   CH_R,     CH_C,     CH_SL,    CH_SR,    CH_NONE },  // 5 channels
  { CH_L,   CH_R,     CH_C,     CH_LFE,   CH_SL,    CH_SR   }   // 5.1 channels
};



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

void WinampSink::close()
{
  if (out)
    out->Close();

  out = 0;
  spk = spk_unknown;
  size2time = 0;
  time = 0;
  paused = false;
}


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
  while (c.size)
  {
    while (out->CanWrite() < 65536)
      Sleep(100);

    int size = out->CanWrite();
    if (size > c.size)
      size = c.size;
    
    out->Write((char *)c.rawdata, size);
    time += size * size2time;
    c.drop(size);
  }

  return true;
}
