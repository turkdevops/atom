// Copyright (c) 2011 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <gtk/gtk.h>
#include <iostream>
#include <unistd.h>
#include <string>
#include "atom.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "client_handler.h"
#include "onig_regexp_extension.h"
#include "io_utils.h"

char* szWorkingDir; // The current working directory

const char* szPath; // The folder the application is in

const char* szPathToOpen; // The file to open

CefRefPtr<ClientHandler> g_handler;

void AppGetSettings(CefSettings& settings, CefRefPtr<CefApp>& app) {
  CefString(&settings.cache_path) = "";
  CefString(&settings.user_agent) = "";
  CefString(&settings.product_version) = "";
  CefString(&settings.locale) = "";
  CefString(&settings.log_file) = "";
  CefString(&settings.javascript_flags) = "";

  settings.log_severity = LOGSEVERITY_ERROR;
  settings.local_storage_quota = 0;
  settings.session_storage_quota = 0;
}

void destroy(void) {
  CefQuitMessageLoop();
}

void TerminationSignalHandler(int signatl) {
  destroy();
}

// WebViewDelegate::TakeFocus in the test webview delegate.
static gboolean HandleFocus(GtkWidget* widget, GdkEventFocus* focus) {
  if (g_handler.get() && g_handler->GetBrowserHwnd()) {
    // Give focus to the browser window.
    g_handler->GetBrowser()->SetFocus(true);
  }

  return TRUE;
}

int main(int argc, char *argv[]) {
  szWorkingDir = get_current_dir_name();
  if (szWorkingDir == NULL)
    return -1;

  std::string fullPath = argv[0];
  fullPath = fullPath.substr(0, fullPath.length() - 5);
  szPath = fullPath.c_str();

  std::string pathToOpen;
  if (argc >= 2) {
    if (argv[1][0] != '/') {
      pathToOpen.append(szWorkingDir);
      pathToOpen.append("/");
      pathToOpen.append(argv[1]);
    } else
      pathToOpen.append(argv[1]);
  } else
    pathToOpen.append(szWorkingDir);
  szPathToOpen = pathToOpen.c_str();

  GtkWidget* window;

  gtk_init(&argc, &argv);

  CefSettings settings;
  CefRefPtr<CefApp> app;

  AppGetSettings(settings, app);
  CefInitialize(settings, app);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "atom");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
  gtk_window_maximize(GTK_WINDOW(window));

  g_signal_connect(window, "focus", G_CALLBACK(&HandleFocus), NULL);

  GtkWidget* vbox = gtk_vbox_new(FALSE, 0);

  g_signal_connect(G_OBJECT(window), "destroy",
      G_CALLBACK(gtk_widget_destroyed), &window);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

  // Create the handler.
  g_handler = new ClientHandler();
  g_handler->SetMainHwnd(vbox);
  g_handler->SetWindow(window);

  new OnigRegexpExtension();

  // Create the browser view.
  CefWindowInfo window_info;
  CefBrowserSettings browserSettings;

  window_info.SetAsChild(vbox);

  std::string path = io_utils_real_app_path("/../index.html");
  if (path.empty())
    return -1;

  std::string resolved("file://");
  resolved.append(path);

  CefBrowser::CreateBrowserSync(window_info,
      static_cast<CefRefPtr<CefClient> >(g_handler), resolved, browserSettings);

  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_widget_show_all(GTK_WIDGET(window));

  GdkPixbuf *pixbuf;
  GError *error = NULL;
  std::string iconPath;
  iconPath.append(szPath);
  iconPath.append("/atom.png");
  pixbuf = gdk_pixbuf_new_from_file(iconPath.c_str(), &error);
  if (pixbuf)
    gtk_window_set_icon(GTK_WINDOW(window), pixbuf);

  // Install an signal handler so we clean up after ourselves.
  signal(SIGINT, TerminationSignalHandler);
  signal(SIGTERM, TerminationSignalHandler);

  CefRunMessageLoop();

  CefShutdown();

  return 0;
}

// Global functions

std::string AppGetWorkingDirectory() {
  return szWorkingDir;
}

std::string AppPath() {
  return szPath;
}

std::string PathToOpen() {
  return szPathToOpen;
}
