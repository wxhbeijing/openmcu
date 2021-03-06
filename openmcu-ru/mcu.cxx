
#include <ptlib.h>

#include "version.h"
#include "mcu.h"
#include "h323.h"

#if USE_LIBJPEG
extern "C" {
#include <jpeglib.h>
}
#endif

const WORD DefaultHTTPPort = 1420;

extern PHTTPServiceProcess::Info ProductInfo;

static const char LogLevelKey[]           = "Log Level";
static const char TraceLevelKey[]         = "Trace level";
static const char UserNameKey[]           = "Username";
static const char PasswordKey[]           = "Password";
static const char HttpPortKey[]           = "HTTP Port";
static const char HttpLinkEventBufferKey[]= "Room control event buffer size";

static const char CallLogFilenameKey[]    = "Call log filename";

#if P_SSL
static const char HTTPCertificateFileKey[]  = "HTTP Certificate";
#endif
static const char DefaultRoomKey[]          = "Default room";
static const char DefaultRoomTimeLimitKey[] = "Room time limit";

static const char DefaultCallLogFilename[] = "mcu_log.txt"; 
static const char DefaultRoom[]            = "room101";
static const char CreateEmptyRoomKey[]     = "Auto create empty room";
static const char RecallLastTemplateKey[]  = "Auto recall last template";
static const char AllowLoopbackCallsKey[]  = "Allow loopback calls";

static const char SipListenerKey[]         = "SIP Listener";

#if OPENMCU_VIDEO
static const char RecorderFfmpegPathKey[]  = "Path to ffmpeg";
static const char RecorderFfmpegOptsKey[]  = "Ffmpeg options";
static const char RecorderFfmpegDirKey[]   = "Video Recorder directory";
static const char RecorderFrameWidthKey[]  = "Video Recorder frame width";
static const char RecorderFrameHeightKey[] = "Video Recorder frame height";
static const char RecorderFrameRateKey[]   = "Video Recorder frame rate";
static const char RecorderSampleRateKey[]  = "Video Recorder sound rate";
#ifdef _WIN32
static const char DefaultFfmpegPath[]         = "ffmpeg.exe";
#else
static const char DefaultFfmpegPath[]         = "/usr/local/bin/ffmpeg";
#endif
static const char DefaultFfmpegOptions[]      = "-y -f s16le -ac 1 -ar %S -i %A -f rawvideo -r %R -s %F -i %V -f asf -acodec pcm_s16le -ac 1 -vcodec msmpeg4v2 %O.asf";
#ifdef RECORDS_DIR
static const char DefaultRecordingDirectory[] = RECORDS_DIR;
#else
static const char DefaultRecordingDirectory[] = "records";
#endif
static const int  DefaultRecorderFrameWidth   = 704;
static const int  DefaultRecorderFrameHeight  = 576;
static const int  DefaultRecorderFrameRate    = 10;
static const int  DefaultRecorderSampleRate   = 16000;

static const char ForceSplitVideoKey[]   = "Force split screen video (enables Room Control page)";

static const char H264LevelForSIPKey[]        = "H.264 Default Level for SIP";
#endif

#define new PNEW


///////////////////////////////////////////////////////////////

class MyPConfigPage : public PConfigPage
{
 public:
   MyPConfigPage(PHTTPServiceProcess & app,const PString & title, const PString & section, const PHTTPAuthority & auth)
    : PConfigPage(app,title,section,auth){};
   void SetString(PString str){string=str;};
   PString GetString(){return string;};
 private:
};

#if USE_LIBJPEG
class JpegFrameHTTP : public PServiceHTTPString
{
  public:
    JpegFrameHTTP(OpenMCU & app, PHTTPAuthority & auth);
    BOOL OnGET (PHTTPServer & server, const PURL &url, const PMIMEInfo & info, const PHTTPConnectionInfo & connectInfo);
    PMutex mutex;
  private:
    OpenMCU & app;
};
#endif

class InteractiveHTTP : public PServiceHTTPString
{
  public:
    InteractiveHTTP(OpenMCU & app, PHTTPAuthority & auth);
    BOOL OnGET (PHTTPServer & server, const PURL &url, const PMIMEInfo & info, const PHTTPConnectionInfo & connectInfo);
  private:
    OpenMCU & app;
};

class MainStatusPage : public PServiceHTTPString
{
 // PCLASSINFO(MainStatusPage, PServiceHTTPString);

  public:
    MainStatusPage(OpenMCU & app, PHTTPAuthority & auth);
    
    virtual BOOL Post(
      PHTTPRequest & request,
      const PStringToString &,
      PHTML & msg
    );
  
  private:
    OpenMCU & app;
};

class InvitePage : public PServiceHTTPString
{
  public:
    InvitePage(OpenMCU & app, PHTTPAuthority & auth);

    virtual BOOL Post(
      PHTTPRequest & request,       // Information on this request.
      const PStringToString & data, // Variables in the POST data.
      PHTML & replyMessage          // Reply message for post.
    );
  
  private:
    OpenMCU & app;
};

class SelectRoomPage : public PServiceHTTPString
{
  public:
    SelectRoomPage(OpenMCU & app, PHTTPAuthority & auth);
    
    virtual BOOL Post(
      PHTTPRequest & request,
      const PStringToString &,
      PHTML & msg
    );
  
  private:
    OpenMCU & app;
};

class WelcomePage : public PServiceHTTPString
{
  public:
    WelcomePage(OpenMCU & app, PHTTPAuthority & auth);
    BOOL OnGET (PHTTPServer & server, const PURL &url, const PMIMEInfo & info, const PHTTPConnectionInfo & connectInfo);
  
  private:
    OpenMCU & app;
};

class RecordsBrowserPage : public PServiceHTTPString
{
  public:
    RecordsBrowserPage(OpenMCU & app, PHTTPAuthority & auth);
    BOOL OnGET (PHTTPServer & server, const PURL &url, const PMIMEInfo & info, const PHTTPConnectionInfo & connectInfo);
  
  private:
    OpenMCU & app;
};




///////////////////////////////////////////////////////////////
// This really isn't the default count only a counter
// for sending aliases and prefixes to the gatekeeper
int OpenMCU::defaultRoomCount = 5;

VideoMixConfigurator OpenMCU::vmcfg;

OpenMCU::OpenMCU()
  : OpenMCUProcessAncestor(ProductInfo)
{
  endpoint = NULL;
  sipendpoint = NULL;
}

void OpenMCU::Main()
{
  Suspend();
}

BOOL OpenMCU::OnStart()
{
  char ** argv=PXGetArgv();
  executableFile = argv[0];
  PDirectory exeDir = executableFile.GetDirectory();
  exeDir.Change();

#ifdef SYS_CONFIG_DIR
#ifdef _WIN32
  SetConfigurationPath(SYS_CONFIG_DIR + std::string("\\openmcu.ini"));
#else
  SetConfigurationPath(SYS_CONFIG_DIR + std::string("/openmcu.ini"));
#endif
#endif

  httpNameSpace.AddResource(new PHTTPDirectory("data", "data"));
  httpNameSpace.AddResource(new PServiceHTTPDirectory("html", "html"));

  manager  = CreateConferenceManager();
  endpoint = CreateEndPoint(*manager);
  sipendpoint = new OpenMCUSipEndPoint(endpoint);

  return PHTTPServiceProcess::OnStart();
}

void OpenMCU::OnStop()
{
  sipendpoint->terminating = 1;
  sipendpoint->WaitForTermination(10000);
  delete sipendpoint;
  sipendpoint = NULL;

  delete endpoint;
  endpoint = NULL;

  delete manager;
  manager = NULL;

  PHTTPServiceProcess::OnStop();
}

void OpenMCU::OnControl()
{
  // This function get called when the Control menu item is selected in the
  // tray icon mode of the service.
  PStringStream url;
  url << "http://";

  PString host = PIPSocket::GetHostName();
  PIPSocket::Address addr;
  if (PIPSocket::GetHostAddress(host, addr))
    url << host;
  else
    url << "localhost";

  url << ':' << DefaultHTTPPort;

  PURL::OpenBrowser(url);
}

BOOL OpenMCU::Initialise(const char * initMsg)
{
  PDirectory exeDir = executableFile.GetDirectory();
  exeDir.Change();

#ifdef _WIN32
  if(PXGetArgc()>1)
  {
    char ** argv=PXGetArgv();
    PString arg1=argv[1];
    if(arg1 *= "debug") cout.rdbuf(PError.rdbuf());
  }
#endif

  PConfig cfg("Parameters");

  // Set log level as early as possible
//  SetLogLevel((PSystemLog::Level)cfg.GetInteger(LogLevelKey, GetLogLevel()));
#if PTRACING

//    SetLogLevel(PSystemLog::Debug6);
//    PTrace::Initialise(6,"trace.txt");
  int TraceLevel=cfg.GetInteger(TraceLevelKey, 6);
  SetLogLevel((PSystemLog::Level)TraceLevel);
#  ifdef SERVER_LOGS
#    ifdef _WIN32
  PTrace::Initialise(TraceLevel,PString(SERVER_LOGS)+"\\trace.txt");
#    else
  PTrace::Initialise(TraceLevel,PString(SERVER_LOGS)+"/trace.txt");
#    endif
#  else
  PTrace::Initialise(TraceLevel,"trace.txt");
#  endif

#  ifdef GIT_REVISION
#    define _QUOTE_MACRO_VALUE1(x) #x
#    define _QUOTE_MACRO_VALUE(x) _QUOTE_MACRO_VALUE1(x)
  PTRACE(1,"OpenMCU\tREVISION " << _QUOTE_MACRO_VALUE(GIT_REVISION));
#    undef _QUOTE_MACRO_VALUE
#    undef _QUOTE_MACRO_VALUE1
#  endif

/*  if (GetLogLevel() >= PSystemLog::Warning)
    PTrace::SetLevel(GetLogLevel()-PSystemLog::Warning);
  else
    PTrace::SetLevel(0);
  PTrace::ClearOptions(PTrace::Timestamp);
  PTrace::SetOptions(PTrace::DateAndTime); */
#endif //if PTRACING

  vmcfg.go(vmcfg.bfw,vmcfg.bfh);

  // Get the HTTP basic authentication info
  PString adminUserName = cfg.GetString(UserNameKey);
  PString adminPassword = PHTTPPasswordField::Decrypt(cfg.GetString(PasswordKey));

  PHTTPSimpleAuth authority(GetName(), adminUserName, adminPassword);

  // Create the parameters URL page, and start adding fields to it
  MyPConfigPage * rsrc = new MyPConfigPage(*this, "Parameters", "Parameters", authority);

  // HTTP authentication username/password
  rsrc->Add(new PHTTPStringField(UserNameKey, 25, adminUserName, "<td rowspan='2' valign='top' style='background-color:#fee;padding:4px;border-left:2px solid #900;border-top:1px dotted #fcc'><b>Security</b>"));

  rsrc->Add(new PHTTPPasswordField(PasswordKey, 25, adminPassword));

//rsrc->Add(new PHTTPStringField("<h3>Other</h3><!--", 1, "","<td></td><td></td>"));

  // Log level for messages
  rsrc->Add(new PHTTPIntegerField(LogLevelKey,
                                  PSystemLog::Fatal, PSystemLog::NumLogLevels-1,
                                  GetLogLevel(),
                                  "<td><td rowspan='4' valign='top' style='background-color:#efe;padding:4px;border-right:2px solid #090;border-top:1px dotted #cfc'><b>Logging:</b><br><br>Log level: 1=Fatal only, 2=Errors, 3=Warnings, 4=Info, 5=Debug<br>Trace level: 0=No tracing ... 6=Very detailed<br>Event buffer size: 10...1000"));

  // default log file name
#ifdef SERVER_LOGS
#  ifdef _WIN32
  { PString lfn = cfg.GetString(CallLogFilenameKey, DefaultCallLogFilename);
    if(lfn.Find("\\")==P_MAX_INDEX) logFilename = PString(SERVER_LOGS)+"\\"+lfn; else logFilename = lfn;
  }
#  else
  { PString lfn = cfg.GetString(CallLogFilenameKey, DefaultCallLogFilename);
    if(lfn.Find("/")==P_MAX_INDEX) logFilename = PString(SERVER_LOGS)+"/"+lfn; else logFilename = lfn;
  }
#  endif
#else
  logFilename = cfg.GetString(CallLogFilenameKey, DefaultCallLogFilename);
#endif
  rsrc->Add(new PHTTPStringField(CallLogFilenameKey, 40, logFilename));

  // Trace level
  rsrc->Add(new PHTTPIntegerField(TraceLevelKey, 0, 6, TraceLevel));

  // Buffered events
  httpBuffer=cfg.GetInteger(HttpLinkEventBufferKey, 100);
  httpBufferedEvents.SetSize(httpBuffer);
  rsrc->Add(new PHTTPIntegerField(HttpLinkEventBufferKey, 10, 1000, httpBuffer));
  httpBufferIndex=0; httpBufferComplete=0;

#if P_SSL
  // SSL certificate file.
  PString certificateFile = cfg.GetString(HTTPCertificateFileKey, "server.pem");
  rsrc->Add(new PHTTPStringField(HTTPCertificateFileKey, 25, certificateFile));
  if (!SetServerCertificate(certificateFile, TRUE)) {
    PSYSTEMLOG(Fatal, "MCU\tCould not load certificate \"" << certificateFile << '"');
    return FALSE;
  }
#endif

  // HTTP Port number to use.
  WORD httpPort = (WORD)cfg.GetInteger(HttpPortKey, DefaultHTTPPort);
  rsrc->Add(new PHTTPIntegerField(HttpPortKey, 1, 32767, httpPort, "<td><td rowspan='5' valign='top' style='background-color:#fee;padding:4px;border-left:2px solid #900;border-top:1px dotted #fcc'><b>Network Setup</b><br /><br />Leave blank &laquo;NAT Router IP&raquo; if your OpenMCU isn't behind NAT.<br />\"Treat...\" - comma-separated LAN IPs (rarely used).\""));

  // SIP Listener setup
  sipListener = cfg.GetString(SipListenerKey, "0.0.0.0").Trim();
  if(sipListener=="")sipListener="0.0.0.0";
  rsrc->Add(new PHTTPStringField(SipListenerKey, 32, sipListener));
  if(sipListener=="0.0.0.0")sipListener="0.0.0.0 :5060";
  sipendpoint->Resume();

  endpoint->Initialise(cfg, rsrc);
  if(endpoint->behind_masq){PStringStream msq; msq<<"Masquerading as "<<*(endpoint->masqAddressPtr); HttpWriteEvent(msq);}

#if OPENMCU_VIDEO
  forceScreenSplit = cfg.GetBoolean(ForceSplitVideoKey, TRUE);
  rsrc->Add(new PHTTPBooleanField(ForceSplitVideoKey, forceScreenSplit));

  h264DefaultLevelForSip = cfg.GetString(H264LevelForSIPKey, "9").AsInteger();
  if(h264DefaultLevelForSip < 9)h264DefaultLevelForSip=9;
  else if(h264DefaultLevelForSip>13 && h264DefaultLevelForSip<20) h264DefaultLevelForSip=13;
  else if(h264DefaultLevelForSip>22 && h264DefaultLevelForSip<30) h264DefaultLevelForSip=22;
  else if(h264DefaultLevelForSip>32 && h264DefaultLevelForSip<40) h264DefaultLevelForSip=32;
  else if(h264DefaultLevelForSip>42 && h264DefaultLevelForSip<50) h264DefaultLevelForSip=42;
  else if(h264DefaultLevelForSip>51) h264DefaultLevelForSip=51;
  rsrc->Add(new PHTTPStringField(H264LevelForSIPKey, 2, PString(h264DefaultLevelForSip)));
#if USE_LIBYUV
  scaleFilter=libyuv::LIBYUV_FILTER;
#endif
#endif

  // get default "room" (conference) name
  defaultRoomName = cfg.GetString(DefaultRoomKey, DefaultRoom);
  rsrc->Add(new PHTTPStringField(DefaultRoomKey, 25, defaultRoomName, "<td rowspan='5' valign='top' style='background-color:#efe;padding:4px;border-right:2px solid #090;border-top:1px dotted #cfc'><b>Room Setup</b>"));

  // create/don't create empty room with default name at start
  rsrc->Add(new PHTTPBooleanField(CreateEmptyRoomKey, FALSE));

  // recall last template after room created
  recallRoomTemplate = cfg.GetBoolean(RecallLastTemplateKey, FALSE);
  rsrc->Add(new PHTTPBooleanField(RecallLastTemplateKey, recallRoomTemplate));

  // get conference time limit 
  roomTimeLimit = cfg.GetInteger(DefaultRoomTimeLimitKey, 0);
  rsrc->Add(new PHTTPIntegerField(DefaultRoomTimeLimitKey, 0, 10800, roomTimeLimit));

  OnCreateConfigPage(cfg, *rsrc);

  // allow/disallow self-invite:
  allowLoopbackCalls = cfg.GetBoolean(AllowLoopbackCallsKey, FALSE);
  rsrc->Add(new PHTTPBooleanField(AllowLoopbackCallsKey, allowLoopbackCalls));

  { // video recorder setup
    vr_ffmpegPath  = cfg.GetString( RecorderFfmpegPathKey,  DefaultFfmpegPath);
    vr_ffmpegOpts  = cfg.GetString( RecorderFfmpegOptsKey,  DefaultFfmpegOptions);
    vr_ffmpegDir   = cfg.GetString( RecorderFfmpegDirKey,   DefaultRecordingDirectory);
    vr_framewidth  = cfg.GetInteger(RecorderFrameWidthKey,  DefaultRecorderFrameWidth);
    vr_frameheight = cfg.GetInteger(RecorderFrameHeightKey, DefaultRecorderFrameHeight);
    vr_framerate   = cfg.GetInteger(RecorderFrameRateKey,   DefaultRecorderFrameRate);
    vr_sampleRate  = cfg.GetInteger(RecorderSampleRateKey,  DefaultRecorderSampleRate);
    PString opts = vr_ffmpegOpts;
    PStringStream frameSize; frameSize << vr_framewidth << "x" << vr_frameheight;
    PStringStream frameRate; frameRate << vr_framerate;
#ifdef _WIN32
    PStringStream outFile; outFile << vr_ffmpegDir << "\\%o";
#else
    PStringStream outFile; outFile << vr_ffmpegDir << "/%o";
#endif
    opts.Replace("%F",frameSize,TRUE,0);
    opts.Replace("%R",frameRate,TRUE,0);
    opts.Replace("%S",PString(vr_sampleRate),TRUE,0);
    opts.Replace("%O",outFile,TRUE,0);
    PStringStream tmp;
    tmp << vr_ffmpegPath << " " << opts;
    ffmpegCall=tmp;
    PStringStream recorderInfo;
    recorderInfo
      << "<td rowspan='7' valign='top' style='background-color:#fee;padding:4px;border-left:2px solid #900;border-top:1px dotted #fcc'>"
      << "<b>Video Recorder Setup:</b><br><br>"
      << "Use the following definitions to set ffmpeg command-line options:<br>"
      << "<b>%V</b> - input video stream,<br>"
      << "<b>%A</b> - input audio stream,<br>"
      << "<b>%F</b> - frame size, "
      << "<b>%R</b> - frame rate,<br>"
      << "<b>%S</b> - sample rate for audio,<br>"
      << "<b>%O</b> - name without extension"
      << "<br><br>";
    if(!PFile::Exists(vr_ffmpegPath)) recorderInfo << "<b><font color=red>ffmpeg doesn't exist - check the path!</font></b>";
    else
    { PFileInfo info;
      PFilePath path(vr_ffmpegPath);
      PFile::GetInfo(path, info);
      if(!(info.type & 3)) recorderInfo << "<b><font color=red>Warning: ffmpeg neither file, nor symlink!</font></b>";
      else if(!(info.permissions & 0111)) recorderInfo << "<b><font color=red>ffmpeg permissions check failed</font></b>";
      else
      {
        if(!PDirectory::Exists(vr_ffmpegDir)) if(!PFile::Exists(vr_ffmpegDir)) { PDirectory::Create(vr_ffmpegDir,0700); PThread::Sleep(50); }
        if(!PDirectory::Exists(vr_ffmpegDir)) recorderInfo << "<b><font color=red>Directory does not exist: " << vr_ffmpegDir << "</font></b>";
        else
        { PFileInfo info;
          PFilePath path(vr_ffmpegDir);
          PFile::GetInfo(path, info);
          if(!(info.type & 6)) recorderInfo << "<b><font color=red>Warning: output directory neither directory, nor symlink!</font></b>";
          else if(!(info.permissions & 0222)) recorderInfo << "<b><font color=red>output directory permissions check failed</font></b>";
          else recorderInfo << "<b><font color=green>Looks good.</font> Execution script preview:</b><br><tt>" << ffmpegCall << "</tt>";
        }
      }
    }
    rsrc->Add(new PHTTPStringField(RecorderFfmpegPathKey, 40, vr_ffmpegPath, recorderInfo));
    rsrc->Add(new PHTTPStringField(RecorderFfmpegOptsKey, 40, vr_ffmpegOpts));
    rsrc->Add(new PHTTPStringField(RecorderFfmpegDirKey, 40, vr_ffmpegDir));
    rsrc->Add(new PHTTPIntegerField(RecorderFrameWidthKey, 176, 1920, vr_framewidth));
    rsrc->Add(new PHTTPIntegerField(RecorderFrameHeightKey, 144, 1152, vr_frameheight));
    rsrc->Add(new PHTTPIntegerField(RecorderFrameRateKey, 1, 100, vr_framerate));
    rsrc->Add(new PHTTPIntegerField(RecorderSampleRateKey, 2000, 1000000, vr_sampleRate));
  }

  // Finished the resource to add, generate HTML for it and add to name space
  PServiceHTML html("System Parameters");
  rsrc->BuildHTML(html);
  httpNameSpace.AddResource(rsrc, PHTTPSpace::Overwrite);
  PStringStream html0; BeginPage(html0,"Parameters","Parameters","$PARAMETERS$");
  PString html1 = rsrc->GetString();
  PStringStream html2; EndPage(html2,GetCopyrightText());
  PStringStream htmlpage; htmlpage << html0 << html1 << html2;

  rsrc->SetString(htmlpage);

  // Create the status page
  httpNameSpace.AddResource(new MainStatusPage(*this, authority), PHTTPSpace::Overwrite);

  // Create invite conference page
  httpNameSpace.AddResource(new InvitePage(*this, authority), PHTTPSpace::Overwrite);

  // Create room selection page
  httpNameSpace.AddResource(new SelectRoomPage(*this, authority), PHTTPSpace::Overwrite);

  // Create video recording directory browser page:
  httpNameSpace.AddResource(new RecordsBrowserPage(*this, authority), PHTTPSpace::Overwrite);

#if USE_LIBJPEG
  // Create JPEG frame via HTTP
  httpNameSpace.AddResource(new JpegFrameHTTP(*this, authority), PHTTPSpace::Overwrite);
#endif

  httpNameSpace.AddResource(new InteractiveHTTP(*this, authority), PHTTPSpace::Overwrite);

  // Add log file links
/*
  if (!systemLogFileName && (systemLogFileName != "-")) {
    httpNameSpace.AddResource(new PHTTPFile("logfile.txt", systemLogFileName, authority));
    httpNameSpace.AddResource(new PHTTPTailFile("tail_logfile", systemLogFileName, authority));
  }
*/  
  // create the home page
/*
  PStringStream shtml;
  BeginPage(shtml,"OpenMCU Home","OpenMCU Home","$WELCOME$");
  EndPage(shtml,GetCopyrightText());
  httpNameSpace.AddResource(new PServiceHTTPString("welcome.html", shtml), PHTTPSpace::Overwrite);
*/
  httpNameSpace.AddResource(new WelcomePage(*this, authority), PHTTPSpace::Overwrite);

  // create monitoring page
  PString monitorText =
#  ifdef GIT_REVISION
#    define _QUOTE_MACRO_VALUE1(x) #x
#    define _QUOTE_MACRO_VALUE(x) _QUOTE_MACRO_VALUE1(x)
                        (PString("OpenMCU REVISION ") + _QUOTE_MACRO_VALUE(GIT_REVISION) +"\n\n") +
#    undef _QUOTE_MACRO_VALUE
#    undef _QUOTE_MACRO_VALUE1
#  endif
                        "<!--#equival monitorinfo-->"
                        "<!--#equival mcuinfo-->";
  httpNameSpace.AddResource(new PServiceHTTPString("monitor.txt", monitorText, "text/plain", authority), PHTTPSpace::Overwrite);

  // adding web server links (eg. images):
#ifdef SYS_RESOURCE_DIR
#  ifdef _WIN32
#    define WEBSERVER_LINK(r1) httpNameSpace.AddResource(new PHTTPFile(r1, PString(SYS_RESOURCE_DIR) + "\\" + r1), PHTTPSpace::Overwrite)
#  else
#    define WEBSERVER_LINK(r1) httpNameSpace.AddResource(new PHTTPFile(r1, PString(SYS_RESOURCE_DIR) + "/" + r1), PHTTPSpace::Overwrite)
#  endif
#else
#  define WEBSERVER_LINK(r1) httpNameSpace.AddResource(new PHTTPFile(r1), PHTTPSpace::Overwrite)
#endif
  WEBSERVER_LINK("i15_mic_on.gif");
  WEBSERVER_LINK("i15_mic_off.gif");
  WEBSERVER_LINK("openmcu.ru_drop_Abdylas_Tynyshov.gif");
  WEBSERVER_LINK("openmcu.ru_vad_vad.gif");
  WEBSERVER_LINK("openmcu.ru_vad_disable.gif");
  WEBSERVER_LINK("openmcu.ru_vad_chosenvan.gif");
  WEBSERVER_LINK("i15_inv.gif");
  WEBSERVER_LINK("openmcu.ru_launched_Ypf.gif");
  WEBSERVER_LINK("openmcu.ru_remove.gif");
  WEBSERVER_LINK("i20_close.gif");
  WEBSERVER_LINK("i20_vad.gif");
  WEBSERVER_LINK("i20_vad2.gif");
  WEBSERVER_LINK("i20_static.gif");
  WEBSERVER_LINK("i20_plus.gif");
  WEBSERVER_LINK("i24_shuff.gif");
  WEBSERVER_LINK("i24_left.gif");
  WEBSERVER_LINK("i24_right.gif");
  WEBSERVER_LINK("i24_mix.gif");
  WEBSERVER_LINK("i24_clr.gif");
  WEBSERVER_LINK("i24_revert.gif");
  WEBSERVER_LINK("openmcu.ico");

  // set up the HTTP port for listening & start the first HTTP thread
  if (ListenForHTTP(httpPort))
    PSYSTEMLOG(Info, "Opened master socket for HTTP: " << httpListeningSocket->GetPort());
  else {
    PSYSTEMLOG(Fatal, "Cannot run without HTTP port: " << httpListeningSocket->GetErrorText());
    return FALSE;
  }

  if(cfg.GetBoolean(CreateEmptyRoomKey, FALSE)) GetEndpoint().OutgoingConferenceRequest(defaultRoomName);

  PSYSTEMLOG(Info, "Service " << GetName() << ' ' << initMsg);
  return TRUE;
}

PCREATE_SERVICE_MACRO(mcuinfo,P_EMPTY,P_EMPTY)
{
  return OpenMCU::Current().GetEndpoint().GetMonitorText();
}

void OpenMCU::OnConfigChanged()
{
}

PString OpenMCU::GetNewRoomNumber()
{
  static PAtomicInteger number = 100;
  return PString(PString::Unsigned, ++number);
}

void OpenMCU::LogMessage(const PString & str)
{
  static PMutex logMutex;
  static PTextFile logFile;

  PTime now;
  PString msg = now.AsString("dd/MM/yyyy") & str;
  logMutex.Wait();

  if (!logFile.IsOpen()) {
    if(!logFile.Open(logFilename, PFile::ReadWrite))
    {
      PTRACE(1,"OpenMCU\tCan not open log file: " << logFilename << "\n" << msg << flush);
      logMutex.Signal();
      return;
    }
    if(!logFile.SetPosition(0, PFile::End))
    {
      PTRACE(1,"OpenMCU\tCan not change log position, log file name: " << logFilename << "\n" << msg << flush);
      logFile.Close();
      logMutex.Signal();
      return;
    }
  }

  if(!logFile.WriteLine(msg))
  {
    PTRACE(1,"OpenMCU\tCan not write to log file: " << logFilename << "\n" << msg << flush);
  }
  logFile.Close();
  logMutex.Signal();
}

ConferenceManager * OpenMCU::CreateConferenceManager()
{
  return new ConferenceManager();
}

OpenMCUH323EndPoint * OpenMCU::CreateEndPoint(ConferenceManager & manager)
{
  return new OpenMCUH323EndPoint(manager);
}

//////////////////////////////////////////////////////////////////////////////////////////////

PCREATE_SERVICE_MACRO_BLOCK(RoomStatus,P_EMPTY,P_EMPTY,block)
{
  return OpenMCU::Current().GetEndpoint().GetRoomStatus(block);
}

MainStatusPage::MainStatusPage(OpenMCU & _app, PHTTPAuthority & auth)
  : PServiceHTTPString("Status", "", "text/html; charset=utf-8", auth),
    app(_app)
{
  PStringStream html;
  

  html << "<meta http-equiv=\"Refresh\" content=\"30\">\n";
  BeginPage(html,"Status Page","Status Page","$STATUS$");
  html << "<p>"
       << "<table class=\"table table-striped table-bordered table-condensed\">"
       << "<tr>"
       << "<th>"
       << "Room Name"
       << "<th>"
       << "Room Members"

       << "<!--#macrostart RoomStatus-->"
         << "<tr>"
         << "<td>"
         << "<!--#status RoomName-->"
         << "<td>"
         << "<!--#status RoomMembers-->"
       << "<!--#macroend RoomStatus-->"

       << "</table>"

       << "<p>";
       
         EndPage(html,app.GetCopyrightText());
  string = html;
}


BOOL MainStatusPage::Post(PHTTPRequest & request,
                          const PStringToString & data,
                          PHTML & msg)
{
  return TRUE;
}


///////////////////////////////////////////////////////////////


#if USE_LIBJPEG

MCUVideoMixer* jpegMixer;

void jpeg_init_destination(j_compress_ptr cinfo){
  if(jpegMixer->myjpeg.GetSize()<32768)jpegMixer->myjpeg.SetSize(32768);
  cinfo->dest->next_output_byte=&jpegMixer->myjpeg[0];
  cinfo->dest->free_in_buffer=jpegMixer->myjpeg.GetSize();
}

boolean jpeg_empty_output_buffer(j_compress_ptr cinfo){
  PINDEX oldsize=jpegMixer->myjpeg.GetSize();
  jpegMixer->myjpeg.SetSize(oldsize+16384);
  cinfo->dest->next_output_byte = &jpegMixer->myjpeg[oldsize];
  cinfo->dest->free_in_buffer = jpegMixer->myjpeg.GetSize() - oldsize;
  return true;
}

void jpeg_term_destination(j_compress_ptr cinfo){
  jpegMixer->jpegSize=jpegMixer->myjpeg.GetSize() - cinfo->dest->free_in_buffer;
  jpegMixer->jpegTime=(long)time(0);
}

JpegFrameHTTP::JpegFrameHTTP(OpenMCU & _app, PHTTPAuthority & auth)
  : PServiceHTTPString("Jpeg", "", "image/jpeg", auth),
    app(_app)
{
}

BOOL JpegFrameHTTP::OnGET (PHTTPServer & server, const PURL &url, const PMIMEInfo & info, const PHTTPConnectionInfo & connectInfo)
{
  PHTTPRequest * req = CreateRequest(url, info, connectInfo.GetMultipartFormInfo(), server); // check authorization
  if(!CheckAuthority(server, *req, connectInfo)) {delete req; return FALSE;}
  delete req;

  PString request=url.AsString();
  PINDEX q;
  if((q=request.Find("?"))==P_MAX_INDEX) return FALSE;

  request=request.Mid(q+1,P_MAX_INDEX);
  PStringToString data;
  PURL::SplitQueryVars(request,data);

  PString room=data("room"); if (room.GetLength()==0) return FALSE;

  int width=atoi(data("w"));
  int height=atoi(data("h"));

  unsigned requestedMixer=0;
  if(data.Contains("mixer")) requestedMixer=(unsigned)data("mixer").AsInteger();

  const unsigned long t1=(unsigned long)time(0);

//  PWaitAndSignal m(mutex); // no more required: the following mutex will do the same:
  app.GetEndpoint().GetConferenceManager().GetConferenceListMutex().Wait();

  ConferenceListType & conferenceList = app.GetEndpoint().GetConferenceManager().GetConferenceList();
  for(ConferenceListType::iterator r = conferenceList.begin(); r != conferenceList.end(); ++r)
  {
    Conference & conference = *(r->second);
    if(conference.GetNumber()==room)
    {
      if(conference.videoMixerList==NULL){ app.GetEndpoint().GetConferenceManager().GetConferenceListMutex().Signal(); return FALSE; }
      jpegMixer=conference.VMLFind(requestedMixer);
      if(jpegMixer==NULL) { app.GetEndpoint().GetConferenceManager().GetConferenceListMutex().Signal(); return FALSE; }
      if(t1-(jpegMixer->jpegTime)>1)
      {
        if(width<1||height<1||width>2048||height>2048)
        { width=OpenMCU::vmcfg.vmconf[jpegMixer->GetPositionSet()].splitcfg.mockup_width;
          height=OpenMCU::vmcfg.vmconf[jpegMixer->GetPositionSet()].splitcfg.mockup_height;
        }
        struct jpeg_compress_struct cinfo; struct jpeg_error_mgr jerr;
        JSAMPROW row_pointer[1];
        int row_stride;
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        cinfo.image_width = width;
        cinfo.image_height = height;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;

        PINDEX amount=width*height*3/2;
        unsigned char *videoData=new unsigned char[amount];

        ((MCUSimpleVideoMixer*)jpegMixer)->ReadMixedFrame((void*)videoData,width,height,amount);
        PColourConverter * converter = PColourConverter::Create("YUV420P", "RGB24", width, height);
        converter->SetDstFrameSize(width, height);
        unsigned char * bitmap = new unsigned char[width*height*3];
        converter->Convert(videoData,bitmap);
        delete converter;
        delete videoData;

        jpeg_set_defaults(&cinfo);
        cinfo.dest = new jpeg_destination_mgr;
        cinfo.dest->init_destination = &jpeg_init_destination;
        cinfo.dest->empty_output_buffer = &jpeg_empty_output_buffer;
        cinfo.dest->term_destination = &jpeg_term_destination;
        jpeg_start_compress(&cinfo,TRUE);
        row_stride = cinfo.image_width * 3;
        while (cinfo.next_scanline < cinfo.image_height)
        { row_pointer[0] = (JSAMPLE *) & bitmap [cinfo.next_scanline * row_stride];
          (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }
        jpeg_finish_compress(&cinfo);
        delete bitmap; delete cinfo.dest; cinfo.dest=NULL;
        jpeg_destroy_compress(&cinfo);
        jpegMixer->jpegTime=t1;
      }

      PTime now;
      PStringStream message;
      message << "HTTP/1.1 200 OK\r\n"
              << "Date: " << now.AsString(PTime::RFC1123, PTime::GMT) << "\r\n"
              << "Server: OpenMCU.ru\r\n"
              << "MIME-Version: 1.0\r\n"
              << "Cache-Control: no-cache, must-revalidate\r\n"
              << "Expires: Sat, 26 Jul 1997 05:00:00 GMT\r\n"
              << "Content-Type: image/jpeg\r\n"
              << "Content-Length: " << jpegMixer->jpegSize << "\r\n"
              << "Connection: Close\r\n"
              << "\r\n";  //that's the last time we need to type \r\n instead of just \n

      server.Write((const char*)message,message.GetLength());
      server.Write(jpegMixer->myjpeg.GetPointer(),jpegMixer->jpegSize);

      app.GetEndpoint().GetConferenceManager().GetConferenceListMutex().Signal();
      server.flush();
      return TRUE;
    }
  }
  app.GetEndpoint().GetConferenceManager().GetConferenceListMutex().Signal();
  return FALSE;
}
#endif //#if USE_LIBJPEG

InteractiveHTTP::InteractiveHTTP(OpenMCU & _app, PHTTPAuthority & auth)
  : PServiceHTTPString("Comm", "", "text/html; charset=utf-8", auth),
    app(_app)
{
}

BOOL InteractiveHTTP::OnGET (PHTTPServer & server, const PURL &url, const PMIMEInfo & info, const PHTTPConnectionInfo & connectInfo)
{
  PHTTPRequest * req = CreateRequest(url, info, connectInfo.GetMultipartFormInfo(), server); // check authorization
  if(!CheckAuthority(server, *req, connectInfo)) {delete req; return FALSE;}
  delete req;

  PString request=url.AsString();
  PINDEX q;
  PStringToString data;

  if((q=request.Find("?"))!=P_MAX_INDEX)
  {
    request=request.Mid(q+1,P_MAX_INDEX);
    PURL::SplitQueryVars(request,data);
  }

  PString room=data("room");

  PStringStream message;
  PTime now;
  int idx;

  message << "HTTP/1.1 200 OK\r\n"
          << "Date: " << now.AsString(PTime::RFC1123, PTime::GMT) << "\r\n"
          << "Server: OpenMCU.ru\r\n"
          << "MIME-Version: 1.0\r\n"
          << "Cache-Control: no-cache, must-revalidate\r\n"
          << "Expires: Sat, 26 Jul 1997 05:00:00 GMT\r\n"
          << "Content-Type: text/html;charset=utf-8\r\n"
          << "Connection: Close\r\n"
          << "\r\n";  //that's the last time we need to type \r\n instead of just \n
  server.Write((const char*)message,message.GetLength());
  server.flush();

  message="<html><body style='font-size:9px;font-family:Verdana,Arial;padding:0px;margin:1px;color:#000'><script>p=parent</script>\n";
  message << OpenMCU::Current().HttpStartEventReading(idx,room);

//PTRACE(1,"!!!!!\tsha1('123')=" << PMessageDigestSHA1::Encode("123")); // sha1 works!! I'll try with websocket in future

  if(room!="")
  {
    OpenMCU::Current().GetEndpoint().GetConferenceManager().GetConferenceListMutex().Wait();
    ConferenceListType & conferenceList = OpenMCU::Current().GetEndpoint().GetConferenceManager().GetConferenceList();
    ConferenceListType::iterator r;
    for (r = conferenceList.begin(); r != conferenceList.end(); ++r) if(r->second->GetNumber() == room) break;
    if(r != conferenceList.end() ) {
      Conference & conference = *(r->second);
      message << "<script>p.splitdata=Array(";
      for (unsigned i=0;i<OpenMCU::vmcfg.vmconfs;i++)
      {
        PString split=OpenMCU::vmcfg.vmconf[i].splitcfg.Id;
        split.Replace("\"","\\x22",TRUE,0);
        if(i!=0) message << ",";
        message << "\"" << split << "\"";
      }
      message << ");\n"
        << "p." << OpenMCU::Current().GetEndpoint().GetMemberListOptsJavascript(conference) << "\n"
        << "p." << OpenMCU::Current().GetEndpoint().GetConferenceOptsJavascript(conference) << "\n"
        << "p.tl=Array" << conference.GetTemplateList() << "\n"
        << "p.seltpl=\"" << conference.GetSelectedTemplateName() << "\"\n"
        << "p.build_page();\n"
        << "</script>\n";
    }
    OpenMCU::Current().GetEndpoint().GetConferenceManager().GetConferenceListMutex().Signal();
  }

  while(server.Write((const char*)message,message.GetLength())) {
    server.flush();
    int count=0;
    message = OpenMCU::Current().HttpGetEvents(idx,room);
    while (message.GetLength()==0 && count < 20){
      count++;
      PThread::Sleep(100);
      message = OpenMCU::Current().HttpGetEvents(idx,room);
    }
    if(message.Find("<script>")==P_MAX_INDEX) message << "<script>p.alive()</script>\n";
  }
  return FALSE;
}

InvitePage::InvitePage(OpenMCU & _app, PHTTPAuthority & auth)
  : PServiceHTTPString("Invite", "", "text/html; charset=utf-8", auth),
    app(_app)
{
  PStringStream html;

  BeginPage(html,"Invite User","Invite User","$INVITE$");

  html << "<p>"
    << "<form method=\"POST\" class=\"well form-inline\">"
    << "<input type=\"text\" class=\"input-small\" placeholder=\"" << app.GetDefaultRoomName() << "\" name=\"room\" value=\"" << app.GetDefaultRoomName() << "\"> "
    << "<input type=\"text\" class=\"input-small\" placeholder=\"Address\" name=\"address\"><script language='javascript'><!--\ndocument.forms[0].address.focus(); //--></script>"
    << "&nbsp;&nbsp;<input type=\"submit\" class=\"btn\" value=\"Invite\">"
    << "</form>";

  EndPage(html,app.GetCopyrightText());

  string = html;
}


BOOL InvitePage::Post(PHTTPRequest & request,
                          const PStringToString & data,
                          PHTML & msg)
{
  PString room    = data("room");
  PString address = data("address");
  PStringStream html;

  if (room.IsEmpty() || address.IsEmpty()) {
    BeginPage(html,"Invite Failed","Invite Failed","$INVITE_F$");
    html << "<div class=\"alert alert-error\"><b>Insufficient information to perform INVITE</b></div>";
    EndPage(html,OpenMCU::Current().GetCopyrightText()); msg = html;
    return TRUE;
  }

  OpenMCUH323EndPoint & ep = app.GetEndpoint();
  BOOL created = ep.OutgoingConferenceRequest(room);

  if (!created) {
    BeginPage(html,"Invite Failed","Invite Failed","$INVITE_F$");
    html << "<div class=\"alert\"><b>Cannot create</b> room " << room;
    html << "</div>";
    EndPage(html,OpenMCU::Current().GetCopyrightText()); msg = html;
    return TRUE;
  }

  PString h323Token;
  PString * userData = new PString(room);
  if (ep.MakeCall(address, h323Token, userData) == NULL) {
    BeginPage(html,"Invite Failed","Invite Failed","$INVITE_F$");
    html << "<div class=\"alert\"><b>Cannot create make call to</b> " << address;
    html << "</div>";
    EndPage(html,OpenMCU::Current().GetCopyrightText()); msg = html;
    ep.GetConferenceManager().RemoveConference(room);
    return TRUE;
  } else {
    PStringStream msg; msg << "Inviting " << address;
    OpenMCU::Current().HttpWriteEventRoom(msg,room);
  }

  BeginPage(html,"Invite Succeeded","Invite Succeeded","$INVITE_S$");
  html << "<div class=\"alert alert-success\">Inviting " << address << " to room " << room;
  html << "</div>";

  html << "<p><h3>Invite another:</h3>"
    << "<form method=\"POST\" class=\"well form-inline\">"
    << "<input type=\"text\" class=\"input-small\" name=\"room\" placeholder=\"" << room << "\" value=\"" << room << "\"> "
    << "<input type=\"text\" class=\"input-small\" name=\"address\" placeholder=\"address\"><script language='javascript'><!--\ndocument.forms[0].address.focus(); //--></script>"
    << "&nbsp;&nbsp;&nbsp;<input type=\"submit\" class=\"btn\" value=\"Invite\">"
    << "</form>";

  EndPage(html,OpenMCU::Current().GetCopyrightText()); msg = html;

  return TRUE;
}

PCREATE_SERVICE_MACRO_BLOCK(RoomList,P_EMPTY,P_EMPTY,block)
{
  return OpenMCU::Current().GetEndpoint().GetRoomList(block);
}

SelectRoomPage::SelectRoomPage(OpenMCU & _app, PHTTPAuthority & auth)
  : PServiceHTTPString("Select", "", "text/html; charset=utf-8", auth),
    app(_app)
{
  PStringStream html;

  BeginPage(html,"Select Room","Select Room","$SELECT$");

  html << "<p>"
       << "<form method=\"POST\" class=\"well form-inline\">"
       << "<!--#macrostart RoomList-->"
         << "<!--#status List-->"
       << "<!--#macroend RoomList-->"
       << "&nbsp;";

  EndPage(html,app.GetCopyrightText());

  string = html;
}

BOOL SelectRoomPage::Post(PHTTPRequest & request,
                          const PStringToString & data,
                          PHTML & msg)
{
  if(OpenMCU::Current().GetForceScreenSplit())
  msg << OpenMCU::Current().GetEndpoint().SetRoomParams(data);
  else msg << ErrorPage(request.localAddr.AsString(),request.localPort,423,"Locked","Room Control feature is locked","To unlock the page: click &laquo;<a href='/Parameters'>Parameters</a>&raquo;, check &laquo;Force split screen video and enable Room Control feature&raquo; and accept.<br/><br/>");
  return TRUE;
}

WelcomePage::WelcomePage(OpenMCU & _app, PHTTPAuthority & auth)
  : PServiceHTTPString("welcome.html", "", "text/html; charset=utf-8", auth),
    app(_app)
{}

BOOL WelcomePage::OnGET (PHTTPServer & server, const PURL &url, const PMIMEInfo & info, const PHTTPConnectionInfo & connectInfo)
{
  { PHTTPRequest * req = CreateRequest(url, info, connectInfo.GetMultipartFormInfo(), server); // check authorization
    if(!CheckAuthority(server, *req, connectInfo)) {delete req; return FALSE;}
    delete req;
  }
  PStringToString data;
  { PString request=url.AsString(); PINDEX q;
    if((q=request.Find("?"))!=P_MAX_INDEX) { request=request.Mid(q+1,P_MAX_INDEX); PURL::SplitQueryVars(request,data); }
  }
  PStringStream shtml;
  BeginPage(shtml,"OpenMCU Home","OpenMCU Home","$WELCOME$");
  shtml << "<br><b>Monitor Text (<span style='cursor:pointer;text-decoration:underline' onclick='javascript:{if(document.selection){var range=document.body.createTextRange();range.moveToElementText(document.getElementById(\"monitorTextId\"));range.select();}else if(window.getSelection){var range=document.createRange();range.selectNode(document.getElementById(\"monitorTextId\"));window.getSelection().addRange(range);}}'>select all</span>)</b><div style='padding:5px;border:1px dotted #595;width:100%;height:auto;max-height:300px;overflow:auto'><pre style='margin:0px;padding:0px' id='monitorTextId'>"
#  ifdef GIT_REVISION
#    define _QUOTE_MACRO_VALUE1(x) #x
#    define _QUOTE_MACRO_VALUE(x) _QUOTE_MACRO_VALUE1(x)
        << "OpenMCU REVISION " << _QUOTE_MACRO_VALUE(GIT_REVISION) << "\n"
#    undef _QUOTE_MACRO_VALUE
#    undef _QUOTE_MACRO_VALUE1
#  endif
        << app.GetEndpoint().GetMonitorText() << "</pre></div>";
  EndPage(shtml,app.GetCopyrightText());
  { PStringStream message; PTime now; message
      << "HTTP/1.1 200 OK\r\n"
      << "Date: " << now.AsString(PTime::RFC1123, PTime::GMT) << "\r\n"
      << "Server: OpenMCU.ru\r\n"
      << "MIME-Version: 1.0\r\n"
      << "Cache-Control: no-cache, must-revalidate\r\n"
      << "Expires: Sat, 26 Jul 1997 05:00:00 GMT\r\n"
      << "Content-Type: text/html;charset=utf-8\r\n"
      << "Content-Length: " << shtml.GetLength() << "\r\n"
      << "Connection: Close\r\n"
      << "\r\n";  //that's the last time we need to type \r\n instead of just \n
    server.Write((const char*)message,message.GetLength());
  }
  server.Write((const char*)shtml,shtml.GetLength());
  server.flush();
  return TRUE;
}


RecordsBrowserPage::RecordsBrowserPage(OpenMCU & _app, PHTTPAuthority & auth)
  : PServiceHTTPString("Records", "", "text/html; charset=utf-8", auth),
    app(_app)
{}

BOOL RecordsBrowserPage::OnGET (PHTTPServer & server, const PURL &url, const PMIMEInfo & info, const PHTTPConnectionInfo & connectInfo)
{
  { PHTTPRequest * req = CreateRequest(url, info, connectInfo.GetMultipartFormInfo(), server); // check authorization
    if(!CheckAuthority(server, *req, connectInfo)) {delete req; return FALSE;}
    delete req;
  }
  PStringToString data;
  { PString request=url.AsString(); PINDEX q;
    if((q=request.Find("?"))!=P_MAX_INDEX) { request=request.Mid(q+1,P_MAX_INDEX); PURL::SplitQueryVars(request,data); }
  }
  if(data.Contains("getfile")) // just download
  {
#ifdef _WIN32
    PString filePathStr = OpenMCU::Current().vr_ffmpegDir + "\\" + data("getfile");
#else
    PString filePathStr = OpenMCU::Current().vr_ffmpegDir + "/" + data("getfile");
#endif
    if(!PFile::Exists(filePathStr))
    { PHTTPRequest * request = CreateRequest(url, info, connectInfo.GetMultipartFormInfo(), server);
      request->entityBody = connectInfo.GetEntityBody();
      PStringStream msg; msg << ErrorPage(request->localAddr.AsString(), request->localPort, 404, "Not Found", "The file cannot be found",
        "The requested URL <a href=\"" + data("getfile") + "\">" + filePathStr + "</a> was not found on this server.<br/><br/>");
      delete request;
      PStringStream message; PTime now; message
        << "HTTP/1.1 404 Not Found\r\n"
        << "Date: " << now.AsString(PTime::RFC1123, PTime::GMT) << "\r\n"
        << "Server: OpenMCU.ru\r\n"
        << "MIME-Version: 1.0\r\n"
        << "Cache-Control: no-cache, must-revalidate\r\n"
        << "Expires: Sat, 26 Jul 1997 05:00:00 GMT\r\n"
        << "Content-Type: text/html;charset=utf-8\r\n"
        << "Content-Length: " << msg.GetLength() << "\r\n"
        << "Connection: Close\r\n"
        << "\r\n";  //that's the last time we need to type \r\n instead of just \n
      message << msg; server.Write((const char*)message,message.GetLength()); server.flush();
      return FALSE;
    }
    FILE *f=fopen(filePathStr,"rb"); if(f==NULL)
    { PHTTPRequest * request = CreateRequest(url, info, connectInfo.GetMultipartFormInfo(), server);
      request->entityBody = connectInfo.GetEntityBody();
      PStringStream msg; msg << ErrorPage(request->localAddr.AsString(), request->localPort, 403, "Forbidden", "The file cannot be downloaded",
        "You don't have permission to access <a href=\"" + data("getfile") + "\">" + filePathStr + "</a> on this server.<br/><br/>");
      delete request;
      PStringStream message; PTime now; message
        << "HTTP/1.1 403 Forbidden\r\n"
        << "Date: " << now.AsString(PTime::RFC1123, PTime::GMT) << "\r\n"
        << "Server: OpenMCU.ru\r\n"
        << "MIME-Version: 1.0\r\n"
        << "Cache-Control: no-cache, must-revalidate\r\n"
        << "Expires: Sat, 26 Jul 1997 05:00:00 GMT\r\n"
        << "Content-Type: text/html;charset=utf-8\r\n"
        << "Content-Length: " << msg.GetLength() << "\r\n"
        << "Connection: Close\r\n"
        << "\r\n";  //that's the last time we need to type \r\n instead of just \n
      message << msg; server.Write((const char*)message,message.GetLength()); server.flush();
      return FALSE;
    }
    fseek(f, 0, SEEK_END); size_t fileSize = ftell(f); rewind(f);

    PStringStream message; PTime now; message
      << "HTTP/1.1 200 OK\r\n"
      << "Date: " << now.AsString(PTime::RFC1123, PTime::GMT) << "\r\n"
      << "Server: OpenMCU.ru\r\n"
      << "MIME-Version: 1.0\r\n"
      << "Cache-Control: no-cache, must-revalidate\r\n"
      << "Expires: Sat, 26 Jul 1997 05:00:00 GMT\r\n"
      << "Content-Type: application/octet-stream\r\n"
      << "Content-Disposition: attachment; filename=\"" << PURL::TranslateString(data("getfile"),PURL::QueryTranslation) << "\"\r\n"
      << "Content-Transfer-Encoding: binary\r\n"
      << "Content-Length: " << fileSize << "\r\n"
      << "Connection: Close\r\n"
      << "\r\n";
    PTimeInterval oldTimeout=server.GetWriteTimeout();
    server.SetWriteTimeout(150000);
    if(!server.Write((const char*)message, message.GetLength())) {fclose(f); return FALSE;}
    server.flush();

    char *buffer = (char*)malloc(PMIN(65536, fileSize));
    size_t p=0, result;
    while (p<fileSize)
    { size_t blockSize = PMIN(65536, fileSize-p);
      result=fread(buffer, 1, blockSize, f);
      if(blockSize != result)
      { PTRACE(1,"mcu.cxx\tFile read error: " << p << "/" << fileSize << ", filename: " << filePathStr);
        fclose(f); free(buffer);
        return FALSE;
      }
      if(!server.Write((const char*)buffer, result))
      { PTRACE(1,"mcu.cxx\tServer write error: " << p << "/" << fileSize << ", filename: " << filePathStr);
        fclose(f); free(buffer);
        return FALSE;
      }
      server.flush();
      p+=result;
    }
    fclose(f); free(buffer);
    server.SetWriteTimeout(oldTimeout);
    return TRUE;
  }

  PString dir=OpenMCU::Current().vr_ffmpegDir;
  BOOL isDir=PFile::Exists(dir); if(isDir)
  { PFileInfo info;
    PFile::GetInfo(PFilePath(dir), info);
    isDir=((info.type & 6) != 0);
  }
#ifdef _WIN32
  PString dir0(dir);
  if(dir0.Right(1)!="\\") dir0+='\\'; else dir=dir.Right(dir.GetLength()-1);
  dir0+='*';
  WIN32_FIND_DATA d;
  HANDLE f = FindFirstFileA(dir0, &d); if (f==INVALID_HANDLE_VALUE)
#else
  if(dir.Right(1)=="/") dir=dir.Right(dir.GetLength()-1);
  DIR *d; if(isDir) isDir=((d=opendir(dir)) != NULL); if(!isDir)
#endif
  { PHTTPRequest * request = CreateRequest(url, info, connectInfo.GetMultipartFormInfo(), server);
    request->entityBody = connectInfo.GetEntityBody();
    PStringStream msg; msg << ErrorPage(request->localAddr.AsString(), request->localPort, 404, "Not Found", "The file cannot be found",
      "The requested URL <a href=\"Records\">/Records</a> was not found on this server.<br/><br/>");
    delete request;
    PStringStream message; PTime now; message
      << "HTTP/1.1 404 Not Found\r\n"
      << "Date: " << now.AsString(PTime::RFC1123, PTime::GMT) << "\r\n"
      << "Server: OpenMCU.ru\r\n"
      << "MIME-Version: 1.0\r\n"
      << "Cache-Control: no-cache, must-revalidate\r\n"
      << "Expires: Sat, 26 Jul 1997 05:00:00 GMT\r\n"
      << "Content-Type: text/html;charset=utf-8\r\n"
      << "Content-Length: " << msg.GetLength() << "\r\n"
      << "Connection: Close\r\n"
      << "\r\n";  //that's the last time we need to type \r\n instead of just \n
    message << msg; server.Write((const char*)message,message.GetLength()); server.flush();
    return FALSE;
  }

// directory d is now opened
  PStringArray fileList;

#ifdef _WIN32
  do if(strcmp(".", d.cFileName) && strcmp("..", d.cFileName))
  {
    FILE *f=fopen(dir + '\\' + d.cFileName,"rb");
    if(f!=NULL)
    {
      fseek(f, 0, SEEK_END);
      PStringStream r; r << d.cFileName << "," << ftell(f);
      fileList.AppendString(r);
      fclose(f);
    }
  }
  while(FindNextFileA(f, &d) != 0 || GetLastError() != ERROR_NO_MORE_FILES);
  FindClose(f);
#else
  struct dirent *e;
  while((e = readdir(d))) if(strcmp(".", e->d_name) && strcmp("..", e->d_name))
  {
    FILE *f=fopen(dir + '/' + e->d_name,"rb");
    if(f!=NULL)
    {
      fseek(f, 0, SEEK_END);
      PStringStream r; r << e->d_name << "," << ftell(f);
      fileList.AppendString(r);
      fclose(f);
    }
  }
  closedir(d);
#endif

  PINDEX sortMode=0;
  if(data.Contains("sort")) {sortMode = data("sort").AsInteger(); if(sortMode<0 || sortMode>7) sortMode=0;}
  PStringStream shtml;
  BeginPage(shtml,"Records","Records","$RECORDS$");


  shtml << "<h1>" << dir << "</h1>";
  if(fileList.GetSize()==0) shtml << "The direcory does not contain records at the moment."; else
  {
    shtml << "<table style='border:2px solid #82b8e3'><tr><th style='border:2px solid #82b8e3;padding:2px;background-color:#144a59;color:#afc'>N</th>"
      << "<th style='border:2px solid #82b8e3;padding:2px;background-color:#144a59'><a style='color:#fff' href='/Records?sort=" << ((sortMode!=0)?'0':'1') << "'>Date/Time</a></th>"
      << "<th style='border:2px solid #82b8e3;padding:2px;background-color:#144a59'><a style='color:#fff' href='/Records?sort=" << ((sortMode!=2)?'2':'3') << "'>Room</a></th>"
      << "<th style='border:2px solid #82b8e3;padding:2px;background-color:#144a59'><a style='color:#fff' href='/Records?sort=" << ((sortMode!=4)?'4':'5') << "'>Resolution</th>"
      << "<th style='border:2px solid #82b8e3;padding:2px;background-color:#144a59'><a style='color:#fff' href='/Records?sort=" << ((sortMode!=6)?'6':'7') << "'>File Size</th></tr>";

    for(PINDEX i=0;i<fileList.GetSize()-1; i++)
    { PString s=fileList[i]; PINDEX pos1=s.Find("__"); if(pos1!=P_MAX_INDEX)
      { PString roomName1 = s.Left(pos1); s=s.Mid(pos1+2,P_MAX_INDEX);
        pos1=s.Find("__"); if(pos1!=P_MAX_INDEX)
        { PString dateTime1 = s.Left(pos1); s=s.Mid(pos1+2,P_MAX_INDEX);
          PString videoResolution1; pos1=s.Find('.'); PINDEX pos2=s.Find(',');
          if(pos1!=P_MAX_INDEX) videoResolution1=s.Left(pos1); else videoResolution1=s.Left(pos2);
          PINDEX fileSize1 = s.Mid(pos2+1,P_MAX_INDEX).AsInteger();
          for(PINDEX j=i+1;j<fileList.GetSize(); j++)
          { PString s=fileList[j]; pos1=s.Find("__"); if(pos1!=P_MAX_INDEX)
            { PString roomName2 = s.Left(pos1); s=s.Mid(pos1+2,P_MAX_INDEX);
              pos1=s.Find("__"); if(pos1!=P_MAX_INDEX)
              { PString dateTime2 = s.Left(pos1); s=s.Mid(pos1+2,P_MAX_INDEX);
                PString videoResolution2; pos1=s.Find('.'); PINDEX pos2=s.Find(',');
                if(pos1!=P_MAX_INDEX) videoResolution2=s.Left(pos1); else videoResolution2=s.Left(pos2);
                PINDEX fileSize2 = s.Mid(pos2+1,P_MAX_INDEX).AsInteger();

                if((sortMode==0) && (dateTime2 < dateTime1)) {}
                else if((sortMode==1) && (dateTime2 >= dateTime1)) {}
                else if((sortMode==2) && (roomName2 >= roomName1)) {}
                else if((sortMode==3) && (roomName2 <= roomName1)) {}
                else if((sortMode==4) && (videoResolution2 <= videoResolution1)) {}
                else if((sortMode==5) && (videoResolution2 >= videoResolution1)) {}
                else if((sortMode==6) && (fileSize2 <= fileSize1)) {}
                else if((sortMode==7) && (fileSize2 >= fileSize1)) {}
                else { PString s=fileList[i]; fileList[i]=fileList[j]; fileList[j]=s; } // or just swap PString pointers?

              }
            }
          }
        }
      }
    }

    for(PINDEX i=0;i<fileList.GetSize(); i++)
    { PString s0=fileList[i]; PINDEX pos1=s0.Find("__"); if(pos1!=P_MAX_INDEX)
      { PString roomName = s0.Left(pos1); PString s=s0.Mid(pos1+2,P_MAX_INDEX);
        pos1=s.Find("__"); if(pos1!=P_MAX_INDEX)
        { PString dateTime = s.Left(pos1); s=s.Mid(pos1+2,P_MAX_INDEX);
          PString videoResolution; pos1=s.Find('.'); PINDEX pos2=s.Find(',');
          if(pos1!=P_MAX_INDEX) videoResolution=s.Left(pos1); else videoResolution=s.Left(pos2);
          PINDEX fileSize = s.Mid(pos2+1,P_MAX_INDEX).AsInteger();
          shtml << "<tr>"
            << "<td style='border:2px solid #82b8e3;padding:2px;background-color:#add0ed'><a href='/Records?getfile=" << s0.Left(s0.Find(',')) << "' download>" << (i+1) << "</a></td>"
            << "<td style='border:2px solid #82b8e3;padding:2px;background-color:#add0ed'>" << dateTime.Mid(7,2) << '.' << dateTime.Mid(5,2) << '.' << dateTime.Mid(0,4) << ' ' << dateTime.Mid(10,2) << ':' << dateTime.Mid(12,2) << "</td>"
            << "<td style='border:2px solid #82b8e3;padding:2px;background-color:#add0ed'>" << roomName << "</td>"
            << "<td style='border:2px solid #82b8e3;padding:2px;background-color:#add0ed'>" << videoResolution << "</td>"
            << "<td style='border:2px solid #82b8e3;padding:2px;background-color:#add0ed;text-align:right'><a href='/Records?getfile=" << s0.Left(s0.Find(',')) << "' download>" << fileSize << "</a></td>"
            << "</tr>";
        }
      }
    }
    shtml << "</table>";

  } // if(fileList.GetSize()==0) ... else {

  EndPage(shtml,app.GetCopyrightText());
  { PStringStream message; PTime now; message
      << "HTTP/1.1 200 OK\r\n"
      << "Date: " << now.AsString(PTime::RFC1123, PTime::GMT) << "\r\n"
      << "Server: OpenMCU.ru\r\n"
      << "MIME-Version: 1.0\r\n"
      << "Cache-Control: no-cache, must-revalidate\r\n"
      << "Expires: Sat, 26 Jul 1997 05:00:00 GMT\r\n"
      << "Content-Type: text/html;charset=utf-8\r\n"
      << "Content-Length: " << shtml.GetLength() << "\r\n"
      << "Connection: Close\r\n"
      << "\r\n";  //that's the last time we need to type \r\n instead of just \n
    server.Write((const char*)message,message.GetLength());
  }
  server.Write((const char*)shtml,shtml.GetLength());
  server.flush();
  return TRUE;
}


// End of File ///////////////////////////////////////////////////////////////
