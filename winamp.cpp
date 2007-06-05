#include "winamp.h"
#include "winampac3.h"

#include "resource.h"
#include "dlg_info.h"
#include "dlg_conf.h"

#include "parsers\file_parser.h"
#include "parsers\ac3\ac3_header.h"
#include "parsers\dts\dts_header.h"
#include "parsers\mpa\mpa_header.h"
#include "parsers\spdif_header.h"
#include "parsers\multi_header.h"


///////////////////////////////////////////////////////////////////////////////
//
//  Winamp In module and support functions
//
///////////////////////////////////////////////////////////////////////////////

void config(HWND parent);
void about(HWND parent);

void init();
void quit();

int  isourfile(char *fn);

int  play(char *filename);
void pause();
void unpause();
int  ispaused();
void stop();

int  getlength();
int  getoutputtime();
void setoutputtime(int seek_pos);

void setvolume(int volume);
void setpan(int pan);

int  infodlg(char *fn, HWND hwnd);
void getfileinfo(char *filename, char *title, int *length_in_ms);
void eq_set(int on, char data[10], int preamp);

In_Module mod = 
{
  0x100,
  "WinampAC3 decoder (AC3/DTS)",
  0,      // hMainWindow
  0,      // hDllInstance
  "AC3\0AC3 Audio File (*.AC3)\0DTS\0DTS Audio File (*.DTS)\0",
  1,      // is_seekable
  1,      // uses output
  config,
  about,
  init,
  quit,
  getfileinfo,
  infodlg,
  isourfile,
  play,
  pause,
  unpause,
  ispaused,
  stop,

  getlength,
  getoutputtime,
  setoutputtime,
    
  setvolume,
  setpan,
    
  0,0,0,0,0,0,0,0,0, // vis stuff  
  0,0, // dsp
  0,   // equalizer
    
  NULL,           // setinfo
    
  0 // out_mod
};

WinampAC3       *winampac3;
IAudioProcessor *proc;
IDecoder        *dec;

HINSTANCE     hinstance;


void config(HWND parent)
{
  TabDlg dlg(hinstance, MAKEINTRESOURCE(IDD_TABDLG), parent); 
  ConfigDlg *sheet;
  sheet = ConfigDlg::create_main(hinstance, winampac3);
  dlg.add_page(0, sheet, "Main");
  sheet = ConfigDlg::create_mixer(hinstance, winampac3);
  dlg.add_page(1, sheet, "Mixer");
  sheet = ConfigDlg::create_gains(hinstance, winampac3);
  dlg.add_page(2, sheet, "Gains");
  sheet = ConfigDlg::create_system(hinstance, winampac3);
  dlg.add_page(3, sheet, "System");
  sheet = ConfigDlg::create_about(hinstance, winampac3);
  dlg.add_page(4, sheet, "About");
  dlg.exec("WinampAC3 configuration");
}

void about(HWND parent)
{
  TabDlg dlg(hinstance, MAKEINTRESOURCE(IDD_TABDLG), parent); 
  ConfigDlg *sheet;
  sheet = ConfigDlg::create_about(hinstance, winampac3);
  dlg.add_page(0, sheet, "About");
  dlg.exec("WinampAC3 configuration");
}

void init() 
{
  winampac3 = new WinampAC3(&mod);
  proc = winampac3->get_audio_processor();
  dec  = winampac3->get_decoder();
  dec->load_params(0, PRESET_ALL);
}

void quit() 
{
  dec->save_params(0, PRESET_ALL);
  delete winampac3;
}


void getfileinfo(char *filename, char *title, int *length_in_ms)
{
  if (!filename || !*filename)  // currently playing file
  {
    if (length_in_ms) *length_in_ms = winampac3->get_length();
    if (title)         
    {
      const char *fn = winampac3->get_filename();
      const char *p = fn + strlen(fn);
      while (*p != '\\' && p >= fn) p--;
      strcpy(title, ++p);
    }
  }
  else if (length_in_ms || title) // some other file
  {
    if (length_in_ms) *length_in_ms = 0;
    if (title)        *title = 0;

    // title
    if (title)
    {
      char *p = filename + strlen(filename);
      while (*p != '\\' && p >= filename) p--;
      strcpy(title, ++p);
    }

    if (length_in_ms) 
    {
      // probe the file
      FileParser file;
      const HeaderParser *parser_list[] = { &spdif_header, &ac3_header, &dts_header, &mpa_header };
      MultiHeader multi_parser(parser_list, array_size(parser_list));

      if (file.open(filename, &multi_parser, 1000000))
        if (file.stats())
          *length_in_ms = (int)(file.get_size(FileParser::time) * 1000);
    }
  }
}

int infodlg(char *filename, HWND hwndParent)
{
  InfoDlg dlg(hinstance, hwndParent, filename); 
  dlg.run();
  return 0;
}

// used for detecting URL streams.. unused here. strncmp(fn,"http://",7) to detect HTTP streams, etc
int isourfile(char *fn) 
{
  return 0; 
} 


int  play(char *filename)  { return !winampac3->play(filename); }
void pause()               { winampac3->pause();                }
void unpause()             { winampac3->unpause();              }
int  ispaused()            { return winampac3->is_paused();     }
void stop()                { winampac3->stop();                 }

int  getlength()           { return winampac3->get_length();    } 
int  getoutputtime()       { return winampac3->get_pos();       }
void setoutputtime(int seek_pos) { winampac3->seek(seek_pos);   }

void setvolume(int volume) { winampac3->set_volume(volume);     }
void setpan(int pan)       { winampac3->set_pan(pan);           }



BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
  hinstance = hModule;
  return TRUE;
}

__declspec( dllexport ) In_Module * winampGetInModule2()
{
  return &mod;
}
