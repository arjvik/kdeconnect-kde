/**
 * SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef PRESENTERPLUGIN_H
#define PRESENTERPLUGIN_H

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_PRESENTER QStringLiteral("kdeconnect.presenter")

class PresenterView;

class Q_DECL_EXPORT PresenterPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.presenter")

public:
    explicit PresenterPlugin(QObject* parent, const QVariantList& args);
    ~PresenterPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override {}

private:
};

#endif
