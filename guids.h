#ifndef WINAMPAC3_GUIDS_H
#define WINAMPAC3_GUIDS_H

#include <objbase.h>
#include "spk.h"
#include "registry.h"

extern HINSTANCE winampac3_instance;

void cr2crlf(char *buf, int size);

#define gettext(x) x
#define N_(x) x
#define _(x) x

// Output sink
#define SINK_WINAMP   0
#define SINK_DSOUND   1

// registry key
#define REG_KEY        "Software\\WinampAC3"
#define REG_KEY_PRESET "Software\\WinampAC3\\preset"
#define REG_KEY_MATRIX "Software\\WinampAC3\\matrix"
#define REG_KEY_EQ     "Software\\WinampAC3\\equalizer"

// preset settings
#define PRESET_SPK    0x01 // Speakers configuration: 
                           // spk
#define PRESET_PROC   0x02 // Audio processor settings:
                           // auto_gain, normalize, auto_matrix, normalize_matrix, voice_control, expand_stereo,
                           // drc, drc_power, master, clev, slev, lfelev
#define PRESET_GAINS  0x04 // Input/output gains: 
                           // input_gains, output_gains
#define PRESET_MATRIX 0x08 // Mixing matrix:
                           // matrix
#define PRESET_DELAY  0x10 // Delay settings:
                           // delay, delay_units, delays
#define PRESET_EQ     0x20 // Equalizer
#define PRESET_SYNC   0x40 // Syncronization settings:
                           // time_shift, time_factor, dejitter, threshold
#define PRESET_SYS    0x80 // System settings: 
                           // formats, spdif, config_autoload, 

#define PRESET_ALL    0xff // all settings
#define PRESET_PRESET 0x3f // settings that saved into preset (all except system settings and sync)

// Constants from dvd_graph.h
#define SPDIF_MODE_NONE                0
#define SPDIF_MODE_DISABLED            1
#define SPDIF_MODE_PASSTHROUGH         2
#define SPDIF_MODE_ENCODE              3

// Constants from spdif_wrapper.h
#define DTS_MODE_AUTO    0
#define DTS_MODE_WRAPPED 1
#define DTS_MODE_PADDED  2

#define DTS_CONV_NONE    0
#define DTS_CONV_16BIT   1
#define DTS_CONV_14BIT   2

#define EQ_BANDS 10

///////////////////////////////////////////////////////////////////////////////
// Interfaces
///////////////////////////////////////////////////////////////////////////////

struct AudioProcessorState;
class IWinampAC3;
class IDecoder;
class IAudioProcessor;


class IWinampAC3
{
public:
  // Tray icon
  STDMETHOD (get_tray)(bool *tray) = 0;
  STDMETHOD (set_tray)(bool  tray) = 0;

  // Output sink (SINK_XXXX constants)
  STDMETHOD (get_sink)(int *sink, int *current_sink) = 0;
  STDMETHOD (set_sink)(int  sink) = 0;

  // Reinit after seek/pause option
  STDMETHOD (get_reinit)(int *reinit) = 0;
  STDMETHOD (set_reinit)(int  reinit) = 0;

  // Timing
  STDMETHOD (get_playback_time)(vtime_t *time) = 0;

  // CPU usage
  STDMETHOD (get_cpu_usage)(double *cpu_usage) = 0;

  // Build and environment info
  STDMETHOD (get_env) (char *buf, int size) = 0;

  // Interfaces
  virtual IDecoder *get_decoder() = 0;
  virtual IAudioProcessor *get_audio_processor() = 0;

  // Config dialog
  STDMETHOD (config) (HWND parent) = 0;
};

class IDecoder
{
public:
  // Input/output format
  STDMETHOD (get_in_spk)  (Speakers *spk) = 0;
  STDMETHOD (get_out_spk) (Speakers *spk) = 0;

  // User format
  STDMETHOD (get_user_spk) (Speakers *spk) = 0;
  STDMETHOD (set_user_spk) (Speakers  spk) = 0;

  // Input formats to accept (formats bitmask)
  STDMETHOD (get_formats) (int *formats) = 0;
  STDMETHOD (set_formats) (int  formats) = 0;

  // Query sink about output format support
  STDMETHOD (get_query_sink) (bool *query_sink) = 0;
  STDMETHOD (set_query_sink) (bool  query_sink) = 0;

  // Use detector
  STDMETHOD (get_use_detector) (bool *use_detector) = 0;
  STDMETHOD (set_use_detector) (bool  use_detector) = 0;

  // Use SPDIF if possible
  STDMETHOD (get_use_spdif) (bool *use_spdif) = 0;
  STDMETHOD (set_use_spdif) (bool  use_spdif) = 0;

  // SPDIF passthrough (formats bitmask)
  STDMETHOD (get_spdif_pt)(int *spdif_pt) = 0;
  STDMETHOD (set_spdif_pt)(int  spdif_pt) = 0;

  // SPDIF as PCM output
  STDMETHOD (get_spdif_as_pcm)(bool *spdif_as_pcm) = 0;
  STDMETHOD (set_spdif_as_pcm)(bool  spdif_as_pcm) = 0;

  // SPDIF encode
  STDMETHOD (get_spdif_encode)(bool *spdif_encode) = 0;
  STDMETHOD (set_spdif_encode)(bool  spdif_encode) = 0;

  // SPDIF stereo PCM passthrough
  STDMETHOD (get_spdif_stereo_pt)(bool *spdif_stereo_pt) = 0;
  STDMETHOD (set_spdif_stereo_pt)(bool  spdif_stereo_pt) = 0;

  // SPDIF bitrate
  STDMETHOD (get_spdif_bitrate)(int *spdif_bitrate) = 0;
  STDMETHOD (set_spdif_bitrate)(int  spdif_bitrate) = 0;

  // SPDIF check sample rate
  STDMETHOD (get_spdif_check_sr)(bool *spdif_check_sr) = 0;
  STDMETHOD (set_spdif_check_sr)(bool  spdif_check_sr) = 0;
  STDMETHOD (get_spdif_allow_48)(bool *spdif_allow_48) = 0;
  STDMETHOD (set_spdif_allow_48)(bool  spdif_allow_48) = 0;
  STDMETHOD (get_spdif_allow_44)(bool *spdif_allow_44) = 0;
  STDMETHOD (set_spdif_allow_44)(bool  spdif_allow_44) = 0;
  STDMETHOD (get_spdif_allow_32)(bool *spdif_allow_32) = 0;
  STDMETHOD (set_spdif_allow_32)(bool  spdif_allow_32) = 0;

  // SPDIF/DTS output mode
  STDMETHOD (get_dts_mode) (int *dts_mode) = 0;
  STDMETHOD (set_dts_mode) (int  dts_mode) = 0;

  // SPDIF/DTS conversion
  STDMETHOD (get_dts_conv) (int *dts_conv) = 0;
  STDMETHOD (set_dts_conv) (int  dts_conv) = 0;

  // SPDIF status
  STDMETHOD (get_spdif_status)(int *spdif_status) = 0;

  // Linear time transform
  STDMETHOD (get_time_shift)   (vtime_t *time_shift) = 0;
  STDMETHOD (set_time_shift)   (vtime_t  time_shift) = 0;
  STDMETHOD (get_time_factor)  (vtime_t *time_factor) = 0;
  STDMETHOD (set_time_factor)  (vtime_t  time_factor) = 0;

  // Jitter correction
  STDMETHOD (get_dejitter)     (bool *dejitter) = 0;
  STDMETHOD (set_dejitter)     (bool  dejitter) = 0;
  STDMETHOD (get_threshold)    (vtime_t *threshold) = 0;
  STDMETHOD (set_threshold)    (vtime_t  threshold) = 0;
  STDMETHOD (get_jitter)       (vtime_t *input_mean, vtime_t *input_stddev, vtime_t *output_mean, vtime_t *output_stddev) = 0;
                               
  // Stats
  STDMETHOD (get_frames)(int  *frames, int *errors) = 0;
  STDMETHOD (get_info)  (char *info, int len) = 0;

  // Load/save settings
  STDMETHOD (load_params) (Config *config, int what) = 0;
  STDMETHOD (save_params) (Config *config, int what) = 0;
};

struct AudioProcessorState
{
  // AGC options
  bool auto_gain;
  bool normalize;
  sample_t attack;
  sample_t release;

  // Matrix options
  bool auto_matrix;
  bool normalize_matrix;
  bool voice_control;
  bool expand_stereo;

  // Master gain
  sample_t master;
  sample_t gain;

  // Mix levels
  sample_t clev;
  sample_t slev;
  sample_t lfelev;

  // Input/output gains
  sample_t input_gains[NCHANNELS];
  sample_t output_gains[NCHANNELS];

  // Input/output levels
  sample_t input_levels[NCHANNELS];
  sample_t output_levels[NCHANNELS];

  // Equalizer
  bool     eq;
  int      eq_freq[EQ_BANDS];
  double   eq_gain[EQ_BANDS];

  // Matrix
  matrix_t matrix;

  // DRC
  bool     drc;
  sample_t drc_power;
  sample_t drc_level;

  // Bass redirection
  bool     bass_redir;
  int      bass_freq;

  // Delay
  bool     delay;
  int      delay_units;
  float    delays[NCHANNELS];
};

class IAudioProcessor
{
public:
  // AGC options
  STDMETHOD (get_auto_gain)    (bool *auto_gain) = 0;
  STDMETHOD (set_auto_gain)    (bool  auto_gain) = 0;
  STDMETHOD (get_normalize)    (bool *normalize) = 0;
  STDMETHOD (set_normalize)    (bool  normalize) = 0;
  STDMETHOD (get_attack)       (sample_t *attack) = 0;
  STDMETHOD (set_attack)       (sample_t  attack) = 0;
  STDMETHOD (get_release)      (sample_t *release) = 0;
  STDMETHOD (set_release)      (sample_t  release) = 0;
  // Matrix options
  STDMETHOD (get_auto_matrix)  (bool *auto_matrix) = 0;
  STDMETHOD (set_auto_matrix)  (bool  auto_matrix) = 0;
  STDMETHOD (get_normalize_matrix)(bool *normalize_matrix) = 0;
  STDMETHOD (set_normalize_matrix)(bool  normalize_matrix) = 0;
  STDMETHOD (get_voice_control)(bool *voice_control) = 0;
  STDMETHOD (set_voice_control)(bool  voice_control) = 0;
  STDMETHOD (get_expand_stereo)(bool *expand_stereo) = 0;
  STDMETHOD (set_expand_stereo)(bool  expand_stereo) = 0;
  // Master gain
  STDMETHOD (get_master)       (sample_t *master) = 0;
  STDMETHOD (set_master)       (sample_t  master) = 0;
  STDMETHOD (get_gain)         (sample_t *gain) = 0;
  // Mix levels              
  STDMETHOD (get_clev)         (sample_t *clev) = 0;
  STDMETHOD (set_clev)         (sample_t  clev) = 0;
  STDMETHOD (get_slev)         (sample_t *slev) = 0;
  STDMETHOD (set_slev)         (sample_t  slev) = 0;
  STDMETHOD (get_lfelev)       (sample_t *lfelev) = 0;
  STDMETHOD (set_lfelev)       (sample_t  lfelev) = 0;
  // Input/output gains
  STDMETHOD (get_input_gains)  (sample_t *input_gains)  = 0;
  STDMETHOD (set_input_gains)  (sample_t *input_gains)  = 0;
  STDMETHOD (get_output_gains) (sample_t *output_gains) = 0;
  STDMETHOD (set_output_gains) (sample_t *output_gains) = 0;
  // Input/output levels
  STDMETHOD (get_levels)       (vtime_t time, sample_t *input_levels, sample_t *output_levels) = 0;
  // Matrix                    
  STDMETHOD (get_matrix)       (matrix_t *matrix) = 0;
  STDMETHOD (set_matrix)       (matrix_t *matrix) = 0;
  // DRC                       
  STDMETHOD (get_drc)          (bool *drc) = 0;
  STDMETHOD (set_drc)          (bool  drc) = 0;
  STDMETHOD (get_drc_power)    (sample_t *drc_power) = 0;
  STDMETHOD (set_drc_power)    (sample_t  drc_power) = 0;
  STDMETHOD (get_drc_level)    (sample_t *drc_level) = 0;
  // Bass redirection
  STDMETHOD (get_bass_redir)   (bool *bass_redir) = 0;
  STDMETHOD (set_bass_redir)   (bool  bass_redir) = 0;
  STDMETHOD (get_bass_freq)    (int  *bass_freq) = 0;
  STDMETHOD (set_bass_freq)    (int   bass_freq) = 0;
  // Eqalizer
  STDMETHOD (get_eq)           (bool *eq) = 0;
  STDMETHOD (set_eq)           (bool  eq) = 0;
  STDMETHOD (get_eq_bands)     (int *freqs, double *gains) = 0;
  STDMETHOD (set_eq_bands)     (const int *freqs, const double *gains) = 0;
  // Spectrum
  STDMETHOD (get_spectrum_length) (size_t *length) = 0;
  STDMETHOD (set_spectrum_length) (size_t  length) = 0;
  STDMETHOD (get_spectrum)     (sample_t *data, double *bin2hz) = 0;
  // Delay
  STDMETHOD (get_delay)        (bool *delay) = 0;
  STDMETHOD (set_delay)        (bool  delay) = 0;
  STDMETHOD (get_delay_units)  (int *delay_units) = 0;
  STDMETHOD (set_delay_units)  (int  delay_units) = 0;
  STDMETHOD (get_delays)       (float *delays) = 0;
  STDMETHOD (set_delays)       (float *delays) = 0;

  STDMETHOD (get_state)        (AudioProcessorState *state, vtime_t time = 0) = 0;
  STDMETHOD (set_state)        (AudioProcessorState *state) = 0;
};


#endif
