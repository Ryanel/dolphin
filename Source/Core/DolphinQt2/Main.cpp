// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <QAbstractEventDispatcher>
#include <QApplication>
#include <QMessageBox>
#include <QObject>

#include "Core/Analytics.h"
#include "Core/BootManager.h"
#include "Core/Core.h"
#include "DolphinQt2/Host.h"
#include "DolphinQt2/InDevelopmentWarning.h"
#include "DolphinQt2/MainWindow.h"
#include "DolphinQt2/Resources.h"
#include "DolphinQt2/Settings.h"
#include "UICommon/UICommon.h"

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);

  UICommon::SetUserDirectory("");
  UICommon::CreateDirectories();
  UICommon::Init();
  Resources::Init();

  // Whenever the event loop is about to go to sleep, dispatch the jobs
  // queued in the Core first.
  QObject::connect(QAbstractEventDispatcher::instance(), &QAbstractEventDispatcher::aboutToBlock,
                   &app, &Core::HostDispatchJobs);

  int retval = 0;
  if (Settings().IsInDevelopmentWarningEnabled())
  {
    InDevelopmentWarning warning_box;
    retval = warning_box.exec() == QDialog::Rejected;
  }
  if (!retval)
  {
    DolphinAnalytics::Instance()->ReportDolphinStart("qt");

    MainWindow win;
    win.show();

#if defined(USE_ANALYTICS) && USE_ANALYTICS
    if (!Settings().HasAskedForAnalyticsPermission())
    {
      QMessageBox analytics_prompt(&win);

      analytics_prompt.setIcon(QMessageBox::Question);
      analytics_prompt.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
      analytics_prompt.setText(QObject::tr(
          "Do you authorize Dolphin to report this information to Dolphin's developers?"));
      analytics_prompt.setInformativeText(
          QObject::tr("If authorized, Dolphin can collect data on its performance, "
                      "feature usage, and configuration, as well as data on your system's "
                      "hardware and operating system.\n\n"
                      "No private data is ever collected. This data helps us understand "
                      "how people and emulated games use Dolphin and prioritize our "
                      "efforts. It also helps us identify rare configurations that are "
                      "causing bugs, performance and stability issues.\n"
                      "This authorization can be revoked at any time through Dolphin's "
                      "settings.\n\n"));

      const int answer = analytics_prompt.exec();

      Settings().SetAskedForAnalyticsPermission(true);
      Settings().SetAnalyticsEnabled(answer == QMessageBox::Yes);
      Settings().Save();

      DolphinAnalytics::Instance()->ReloadConfig();
    }
#endif

    retval = app.exec();
  }

  BootManager::Stop();
  Core::Shutdown();
  UICommon::Shutdown();
  Host::GetInstance()->deleteLater();

  return retval;
}
