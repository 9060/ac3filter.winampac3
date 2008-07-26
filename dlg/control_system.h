/*
  System options:
  * Allowed formats
  * DirectShow options
  * Waveout/DirectSound switch
  * Filter merit
*/

#ifndef CONTROL_SYSTEM_H
#define CONTROL_SYSTEM_H

#include "../guids.h"
#include "../controls.h"

class ControlSystem : public Controller
{
protected:
  IWinampAC3 *filter;
  IDecoder   *dec;

  int  formats;
  int  reinit;
  bool query_sink;

public:
  ControlSystem(HWND dlg, IWinampAC3 *filter, IDecoder *dec);
  ~ControlSystem();

  virtual void update();
  virtual cmd_result command(int control, int message);
};

#endif
