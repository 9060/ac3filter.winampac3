#include "../resource_ids.h"
#include "control_sink.h"

static const int controls[] =
{
  IDC_RB_SINK_WINAMP,
  IDC_RB_SINK_DSOUND,
  0
};

///////////////////////////////////////////////////////////////////////////////

ControlSink::ControlSink(HWND _dlg, IWinampAC3 *_filter):
Controller(_dlg, ::controls), filter (_filter)
{
}

ControlSink::~ControlSink()
{
}

void ControlSink::init()
{
}

void ControlSink::update()
{
  filter->get_sink(&sink, 0);
  CheckDlgButton(hdlg, IDC_RB_SINK_WINAMP, sink == SINK_WINAMP? BST_CHECKED: BST_UNCHECKED);
  CheckDlgButton(hdlg, IDC_RB_SINK_DSOUND, sink == SINK_DSOUND? BST_CHECKED: BST_UNCHECKED);
};

ControlSink::cmd_result ControlSink::command(int control, int message)
{
  switch (control)
  {
    case IDC_RB_SINK_WINAMP:
    {
      sink = SINK_WINAMP;
      filter->set_sink(sink);
      update();
      return cmd_ok;
    }

    case IDC_RB_SINK_DSOUND:
    {
      sink = SINK_DSOUND;
      filter->set_sink(sink);
      update();
      return cmd_ok;
    }
  }
  return cmd_not_processed;
}
