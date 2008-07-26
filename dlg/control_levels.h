/*
  Input/output levels
*/

#ifndef CONTROL_LEVELS_H
#define CONTROL_LEVELS_H

#include "../guids.h"
#include "../controls.h"

class ControlLevels : public Controller
{
protected:
  IWinampAC3 *filter;
  IAudioProcessor *proc;
  bool invert_levels;

public:
  ControlLevels(HWND dlg, IWinampAC3 *filter, IAudioProcessor *proc, bool invert_levels);
  ~ControlLevels();

  virtual void init();
  virtual void update_dynamic();
};

#endif
