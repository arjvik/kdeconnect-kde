/**
 * SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "presenterplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QCursor>
#include <QDebug>
#include <QDBusConnection>
#include <QQuickView>
#include <QQmlError>
#include <QQuickItem>
#include <QtGlobal>

#include <core/device.h>
#include <core/daemon.h>
#include <QScreen>
#include "plugin_presenter_debug.h"

K_PLUGIN_CLASS_WITH_JSON(PresenterPlugin, "kdeconnect_presenter.json")

PresenterPlugin::PresenterPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{

}

PresenterPlugin::~PresenterPlugin()
{

}

bool PresenterPlugin::receivePacket(const NetworkPacket& np)
{
    float dx = np.get<float>(QStringLiteral("dx"), 0) * 1920;
    float dy = np.get<float>(QStringLiteral("dy"), 0) * 1080;
    // qDebug() << "dx=" << dx << " dy=" << dy;
    QPoint point = QCursor::pos();
    QCursor::setPos(point.x() + (int)dx, point.y() + (int)dy);
    return true;
}

#include "presenterplugin.moc"

