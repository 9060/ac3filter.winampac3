/*
  Sink selection
*/

#ifndef CONTROL_SINK_H
#define CONTROL_SINK_H

#include "../guids.h"
#include "../controls.h"

class ControlSink : public Controller
{
protected:
  IWinampAC3 *filter;
  int sink;

public:
  ControlSink(HWND dlg, IWinampAC3 *filter);
  ~ControlSink();

  virtual void init();
  virtual void update();

  virtual cmd_result command(int control, int message);
};

#endif
