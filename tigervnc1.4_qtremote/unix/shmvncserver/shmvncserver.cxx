/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2004-2008 Constantin Kaplinsky.  All Rights Reserved.
 *    
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// FIXME: Check cases when screen width/height is not a multiply of 32.
//        e.g. 800x600.

#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>
#include <rfb/VNCServerST.h>
#include <rfb/Configuration.h>
#include <rfb/Timer.h>
#include <network/TcpSocket.h>

#if QUERY_CONNECT_DIALOG
#include <vncconfig/QueryConnectDialog.h>
#endif
#include <cstdlib>

#include <signal.h>

#include <shmvncserver/Geometry.h>
#include <shmvncserver/Image.h>
#include <shmvncserver/XPixelBuffer.h>
#include <shmvncserver/PollingScheduler.h>
#include <shmvncserver/View.h>
#include <shmvncserver/messages.h>

// XXX Lynx/OS 2.3: protos for select(), bzero()
#ifdef Lynx
#include <sys/proto.h>
#endif

extern char buildtime[];

using namespace rfb;
using namespace network;

static LogWriter vlog("Main");

IntParameter pollingCycle("PollingCycle", "Milliseconds per one polling "
                          "cycle; actual interval may be dynamically "
                          "adjusted to satisfy MaxProcessorUsage setting", 30);
IntParameter maxProcessorUsage("MaxProcessorUsage", "Maximum percentage of "
                               "CPU time to be consumed", 35);
BoolParameter useShm("UseSHM", "Use MIT-SHM extension if available", true);
BoolParameter useOverlay("OverlayMode", "Use overlay mode under "
                         "IRIX or Solaris", true);
StringParameter displayname("display", "The X display", "");
IntParameter rfbport("rfbport", "TCP port to listen for RFB protocol",5900);
IntParameter queryConnectTimeout("QueryConnectTimeout",
                                 "Number of seconds to show the Accept Connection dialog before "
                                 "rejecting the connection",
                                 10);
StringParameter hostsFile("HostsFile", "File with IP access control rules", "");

//
// Allow the main loop terminate itself gracefully on receiving a signal.
//

static bool caughtSignal = false;

static void CleanupSignalHandler(int sig)
{
  caughtSignal = true;
}


#ifdef QUERY_CONNECT_DIALOG
class QueryConnHandler : public VNCServerST::QueryConnectionHandler,
                         public QueryResultCallback {
public:
  QueryConnHandler(Display* dpy, VNCServerST* vs)
    : display(dpy), server(vs), queryConnectDialog(0), queryConnectSock(0) {}
  ~QueryConnHandler() { delete queryConnectDialog; }

  // -=- VNCServerST::QueryConnectionHandler interface
  virtual VNCServerST::queryResult queryConnection(network::Socket* sock,
                                                   const char* userName,
                                                   char** reason) {
    if (queryConnectSock) {
      *reason = strDup("Another connection is currently being queried.");
      return VNCServerST::REJECT;
    }
    if (!userName) userName = "(anonymous)";
    queryConnectSock = sock;
    CharArray address(sock->getPeerAddress());
    delete queryConnectDialog;
    queryConnectDialog = new QueryConnectDialog(display, address.buf,
                                                userName, queryConnectTimeout,
                                                this);
    queryConnectDialog->map();
    return VNCServerST::PENDING;
  }

  // -=- QueryResultCallback interface
  virtual void queryApproved() {
    server->approveConnection(queryConnectSock, true, 0);
    queryConnectSock = 0;
  }
  virtual void queryRejected() {
    server->approveConnection(queryConnectSock, false,
                              "Connection rejected by local user");
    queryConnectSock = 0;
  }
private:
  Display* display;
  VNCServerST* server;
  QueryConnectDialog* queryConnectDialog;
  network::Socket* queryConnectSock;
};
#endif

class XDesktop : public SDesktop
{
public:
  XDesktop(View *view, Geometry *geometry_)
    : view(view), geometry(geometry_), pb(0), server(0),
      oldButtonMask(0),
      running(false)
  {
  }

  virtual ~XDesktop() {
    stop();
  }

  inline void poll() {
  }

  // -=- SDesktop interface

  virtual void start(VNCServer* vs) {
    // Create an ImageFactory instance for producing Image objects.
    ImageFactory factory((bool)useShm, (bool)useOverlay);

    // Create pixel buffer and provide it to the server object.
    pb = new XPixelBuffer(view, factory, geometry->getRect());
    vlog.info("Allocated %s", pb->getImage()->classDesc());

    server = (VNCServerST *)vs;
    server->setPixelBuffer(pb);

    running = true;
  }

  virtual void stop() {
    running = false;

    delete pb;
    pb = 0;
  }

  inline bool isRunning() {
    return running;
  }

  virtual void pointerEvent(const Point& pos, int buttonMask) {
    //TODO move this code away
    MouseEventMessage mouseMsg;
    if ((buttonMask == 1) && (oldButtonMask == 0)) {
        mouseMsg.msgType = 2;

    }else if ((buttonMask == 0) && (oldButtonMask == 1)) {
        mouseMsg.msgType = 3;

    } else {
        mouseMsg.msgType = 5;
    }
    mouseMsg.xPos = pos.x;
    mouseMsg.yPos = pos.y;
    mouseMsg.changedButton = oldButtonMask^buttonMask;
    mouseMsg.buttonsMask = buttonMask;
    write(view->ssocket, &mouseMsg, sizeof(MouseEventMessage));
    oldButtonMask = buttonMask;
  }

  virtual void keyEvent(rdr::U32 key, bool down) {
    //TODO move this code away
    KeyboardEventMessage keyMsg;
    if (down) {
        keyMsg.msgType = 6;
    } else {
        keyMsg.msgType = 7;
    }
    keyMsg.key = key;
    write(view->ssocket, &keyMsg, sizeof(KeyboardEventMessage));
  }

  virtual void clientCutText(const char* str, int len) {
  }

  virtual Point getFbSize() {
    return Point(pb->width(), pb->height());
  }

protected:
  View* view;
  Geometry* geometry;
  XPixelBuffer* pb;
  VNCServerST* server;
  int oldButtonMask;
  bool running;
};


class FileTcpFilter : public TcpFilter
{

public:

  FileTcpFilter(const char *fname)
    : TcpFilter("-"), fileName(NULL), lastModTime(0)
  {
    if (fname != NULL)
      fileName = strdup((char *)fname);
  }

  virtual ~FileTcpFilter()
  {
    if (fileName != NULL)
      free(fileName);
  }

  virtual bool verifyConnection(Socket* s)
  {
    if (!reloadRules()) {
      vlog.error("Could not read IP filtering rules: rejecting all clients");
      filter.clear();
      filter.push_back(parsePattern("-"));
      return false;
    }

    return TcpFilter::verifyConnection(s);
  }

protected:

  bool reloadRules()
  {
    if (fileName == NULL)
      return true;

    struct stat st;
    if (stat(fileName, &st) != 0)
      return false;

    if (st.st_mtime != lastModTime) {
      // Actually reload only if the file was modified
      FILE *fp = fopen(fileName, "r");
      if (fp == NULL)
        return false;

      // Remove all the rules from the parent class
      filter.clear();

      // Parse the file contents adding rules to the parent class
      char buf[32];
      while (readLine(buf, 32, fp)) {
        if (buf[0] && strchr("+-?", buf[0])) {
          filter.push_back(parsePattern(buf));
        }
      }

      fclose(fp);
      lastModTime = st.st_mtime;
    }
    return true;
  }

protected:

  char *fileName;
  time_t lastModTime;

private:

  //
  // NOTE: we silently truncate long lines in this function.
  //

  bool readLine(char *buf, int bufSize, FILE *fp)
  {
    if (fp == NULL || buf == NULL || bufSize == 0)
      return false;

    if (fgets(buf, bufSize, fp) == NULL)
      return false;

    char *ptr = strchr(buf, '\n');
    if (ptr != NULL) {
      *ptr = '\0';              // remove newline at the end
    } else {
      if (!feof(fp)) {
        int c;
        do {                    // skip the rest of a long line
          c = getc(fp);
        } while (c != '\n' && c != EOF);
      }
    }
    return true;
  }

};

char* programName;

static void printVersion(FILE *fp)
{
  fprintf(fp, "TigerVNC Server version %s, built %s\n",
          PACKAGE_VERSION, buildtime);
}

static void usage()
{
  printVersion(stderr);
  fprintf(stderr, "\nUsage: %s [<parameters>]\n", programName);
  fprintf(stderr, "       %s --version\n", programName);
  fprintf(stderr,"\n"
          "Parameters can be turned on with -<param> or off with -<param>=0\n"
          "Parameters which take a value can be specified as "
          "-<param> <value>\n"
          "Other valid forms are <param>=<value> -<param>=<value> "
          "--<param>=<value>\n"
          "Parameter names are case-insensitive.  The parameters are:\n\n");
  Configuration::listParams(79, 14);
  exit(1);
}

int main(int argc, char** argv)
{
  initStdIOLoggers();
  LogWriter::setLogParams("*:stderr:30");

  programName = argv[0];
  View *view;

  Configuration::enableServerParams();

  for (int i = 1; i < argc; i++) {
    if (Configuration::setParam(argv[i]))
      continue;

    if (argv[i][0] == '-') {
      if (i+1 < argc) {
        if (Configuration::setParam(&argv[i][1], argv[i+1])) {
          i++;
          continue;
        }
      }
      if (strcmp(argv[i], "-v") == 0 ||
          strcmp(argv[i], "-version") == 0 ||
          strcmp(argv[i], "--version") == 0) {
        printVersion(stdout);
        return 0;
      }
      usage();
    }

    usage();
  }

  view = new View();

  signal(SIGHUP, CleanupSignalHandler);
  signal(SIGINT, CleanupSignalHandler);
  signal(SIGTERM, CleanupSignalHandler);

  std::list<TcpListener> listeners;

  try {
    Geometry geo(view->width, view->height);
    if (geo.getRect().is_empty()) {
      vlog.error("Exiting with error");
      return 1;
    }
    XDesktop desktop(view, &geo);

    VNCServerST server("shmvncserver", &desktop);
#if QUERY_CONNECT_DIALOG
    QueryConnHandler qcHandler(dpy, &server);
    server.setQueryConnectionHandler(&qcHandler);
#endif

    createTcpListeners(&listeners, 0, (int)rfbport);
    vlog.info("Listening on port %d", (int)rfbport);

    const char *hostsData = hostsFile.getData();
    FileTcpFilter fileTcpFilter(hostsData);
    if (strlen(hostsData) != 0)
      for (std::list<TcpListener>::iterator i = listeners.begin();
           i != listeners.end();
           i++)
        (*i).setFilter(&fileTcpFilter);
    delete[] hostsData;

    PollingScheduler sched((int)pollingCycle, (int)maxProcessorUsage);

    while (!caughtSignal) {
      struct timeval tv;
      fd_set rfds;
      std::list<Socket*> sockets;
      std::list<Socket*>::iterator i;

      FD_ZERO(&rfds);
      for (std::list<TcpListener>::iterator i = listeners.begin();
           i != listeners.end();
           i++)
        FD_SET((*i).getFd(), &rfds);

      server.getSockets(&sockets);
      int clients_connected = 0;
      for (i = sockets.begin(); i != sockets.end(); i++) {
        if ((*i)->isShutdown()) {
          server.removeSocket(*i);
          delete (*i);
        } else {
          FD_SET((*i)->getFd(), &rfds);
          clients_connected++;
        }
      }

      FD_SET(view->ssocket, &rfds);

      if (!clients_connected)
        sched.reset();

      if (/*sched.isRunning()*/false) {
        int wait_ms = sched.millisRemaining();
        if (wait_ms > 500) {
          wait_ms = 500;
        }
        tv.tv_usec = wait_ms * 1000;
#ifdef DEBUG
        // fprintf(stderr, "[%d]\t", wait_ms);
#endif
      tv.tv_sec = 0;
      } else {
        tv.tv_usec = 0;
        tv.tv_sec = 2;
      }

      // Do the wait...
      sched.sleepStarted();
      int n = select(FD_SETSIZE, &rfds, 0, 0, &tv);
      sched.sleepFinished();

      if (n < 0) {
        if (errno == EINTR) {
          vlog.debug("Interrupted select() system call");
          continue;
        } else {
          throw rdr::SystemException("select", errno);
        }
      }

      // Accept new VNC connections
      for (std::list<TcpListener>::iterator i = listeners.begin();
           i != listeners.end();
           i++) {
        if (FD_ISSET((*i).getFd(), &rfds)) {
          Socket* sock = (*i).accept();
          if (sock) {
            server.addSocket(sock);
          } else {
            vlog.status("Client connection rejected");
          }
        }
      }

      Timer::checkTimeouts();
      server.checkTimeouts();

      // Client list could have been changed.
      server.getSockets(&sockets);

      if (FD_ISSET(view->ssocket, &rfds)) {
          int val = 0;
          if (recv(view->ssocket, &val, sizeof(int), 0) == sizeof(int)) {
          if (val == SCREEN_UPDATED_MSG) {
              Rect rect;
              rect.setXYWH(0, 0, view->width, view->height);
              server.add_changed(rect);
          }
          }
      }

      // Nothing more to do if there are no client connections.
      if (sockets.empty())
        continue;

      // Process events on existing VNC connections
      for (i = sockets.begin(); i != sockets.end(); i++) {
        if (FD_ISSET((*i)->getFd(), &rfds))
          server.processSocketEvent(*i);
      }

      if (desktop.isRunning() && sched.goodTimeToPoll()) {
        sched.newPass();
        desktop.poll();
      }
    }

  } catch (rdr::Exception &e) {
    vlog.error("%s", e.str());
    return 1;
  }

  vlog.info("Terminated");
  return 0;
}
