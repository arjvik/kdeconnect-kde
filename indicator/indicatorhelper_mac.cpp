/*
 * SPDX-FileCopyrightText: 2019 Weixuan XIAO <veyx.shaw@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QMessageBox>
#include <QStandardPaths>
#include <QThread>
#include <QDebug>

#include <KLocalizedString>

#include <dbushelper.h>

#include "indicatorhelper.h"

#include "serviceregister_mac.h"

#include <kdeconnectconfig.h>

IndicatorHelper::IndicatorHelper()
{
    registerServices();

    QIcon kdeconnectIcon = QIcon::fromTheme(QStringLiteral("kdeconnect"));
    QPixmap splashPixmap(kdeconnectIcon.pixmap(256, 256));

    m_splashScreen = new QSplashScreen(splashPixmap);

    m_splashScreen->showMessage(i18n("Launching") + QStringLiteral("\n"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
    m_splashScreen->show();
}

IndicatorHelper::~IndicatorHelper()
{
    if (m_splashScreen != nullptr) {
        delete m_splashScreen;
        m_splashScreen = nullptr;
    }
}

void IndicatorHelper::preInit() {}

void IndicatorHelper::postInit()
{
    m_splashScreen->finish(nullptr);
}

void IndicatorHelper::iconPathHook()
{
    const QString iconPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("kdeconnect-icons"), QStandardPaths::LocateDirectory);
    if (!iconPath.isNull()) {
        QStringList themeSearchPaths = QIcon::themeSearchPaths();
        themeSearchPaths << iconPath;
        QIcon::setThemeSearchPaths(themeSearchPaths);
    }
}

int IndicatorHelper::daemonHook(QProcess &kdeconnectd)
{
    // This flag marks whether a session DBus daemon is installed and run
    bool hasUsableSessionBus = true;
    // Use another bus instance for detecting, avoid session bus cache in Qt
    if (!QDBusConnection::connectToBus(QDBusConnection::SessionBus,
        QStringLiteral("kdeconnect-test-client")).isConnected()) {
        qDebug() << "Default session bus not detected, will use private D-Bus.";

        // Unset launchctl env and private dbus addr file, avoid block
        DBusHelper::macosUnsetLaunchctlEnv();
        QFile privateDBusAddressFile(KdeConnectConfig::instance().privateDBusAddressPath());
        if (privateDBusAddressFile.exists()) privateDBusAddressFile.resize(0);

        // Update session bus usability state
        hasUsableSessionBus = false;
    }

    // Start daemon
    m_splashScreen->showMessage(i18n("Launching daemon") + QStringLiteral("\n"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);

    // Here we will try to bring our private session D-Bus
    if (!hasUsableSessionBus) {
        qDebug() << "Launching private session D-Bus.";
        DBusHelper::macosUnsetLaunchctlEnv();
        DBusHelper::launchDBusDaemon();
        // Wait for dbus daemon env
        QProcess getLaunchdDBusEnv;
        m_splashScreen->showMessage(i18n("Waiting D-Bus") + QStringLiteral("\n"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
        int retry = 0;
        getLaunchdDBusEnv.setProgram(QStringLiteral("launchctl"));
        getLaunchdDBusEnv.setArguments({
            QStringLiteral("getenv"),
            QStringLiteral(KDECONNECT_SESSION_DBUS_LAUNCHD_ENV)
        });
        getLaunchdDBusEnv.start();
        getLaunchdDBusEnv.waitForFinished();

        QString launchdDBusEnv = QString::fromLocal8Bit(getLaunchdDBusEnv.readAllStandardOutput());

        if (!launchdDBusEnv.isEmpty() && QDBusConnection::sessionBus().isConnected()) {
            qDebug() << "Private D-Bus daemon launched and connected.";
            hasUsableSessionBus = true;
        } else if (!launchdDBusEnv.isEmpty()) {
            // Show a warning and exit
            qCritical() << "Invalid " << KDECONNECT_SESSION_DBUS_LAUNCHD_ENV << "env: \""
                        << launchdDBusEnv << "\"";

            QMessageBox::critical(nullptr, i18n("KDE Connect"),
                                  i18n("Cannot connect to DBus\n"
                                  "KDE Connect will quit"),
                                  QMessageBox::Abort,
                                  QMessageBox::Abort);
        } else {
            // Show a warning and exit
            qCritical() << "Fail to get launchctl" << KDECONNECT_SESSION_DBUS_LAUNCHD_ENV << "env";

            QMessageBox::critical(nullptr, i18n("KDE Connect"),
                                  i18n("Cannot connect to DBus\n"
                                  "KDE Connect will quit"),
                                  QMessageBox::Abort,
                                  QMessageBox::Abort);
            return -2;
        }
    }

    // Start kdeconnectd, the daemon will not duplicate when there is already one
    if (QFile::exists(QCoreApplication::applicationDirPath() + QStringLiteral("/kdeconnectd"))) {
        kdeconnectd.setProgram(QCoreApplication::applicationDirPath() + QStringLiteral("/kdeconnectd"));
    } else if (QFile::exists(QString::fromLatin1(qgetenv("craftRoot")) + QStringLiteral("/../lib/libexec/kdeconnectd"))) {
        kdeconnectd.setProgram(QString::fromLatin1(qgetenv("craftRoot")) + QStringLiteral("/../lib/libexec/kdeconnectd"));
    } else {
        QMessageBox::critical(nullptr, i18n("KDE Connect"),
                              i18n("Cannot find kdeconnectd"),
                              QMessageBox::Abort,
                              QMessageBox::Abort);
        return -1;
    }
    kdeconnectd.startDetached();

    m_splashScreen->showMessage(i18n("Loading modules") + QStringLiteral("\n"), Qt::AlignHCenter | Qt::AlignBottom, Qt::white);

    return 0;
}

#ifdef QSYSTRAY
void IndicatorHelper::systrayIconHook(QSystemTrayIcon &systray)
{
    Q_UNUSED(systray);
}
#else
void IndicatorHelper::systrayIconHook(KStatusNotifierItem &systray)
{
    const QString iconPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("kdeconnect-icons"), QStandardPaths::LocateDirectory);
    if (!iconPath.isNull()) {
        auto icon = QIcon::fromTheme(QStringLiteral("kdeconnectindicator"));
        icon.setIsMask(true); // Make icon adapt to menu bar color
        systray.setIconByPixmap(icon);
    } else {
        // We are in macOS dev env, just continue
        qWarning() << "Fail to find indicator icon, continue anyway";
    }
}
#endif
