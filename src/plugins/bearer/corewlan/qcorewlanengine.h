/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOREWLANENGINE_H
#define QCOREWLANENGINE_H

#include "../qbearerengine_impl.h"

#include <QMap>
#include <QTimer>
#include <SystemConfiguration/SystemConfiguration.h>
#include <QThread>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

class QNetworkConfigurationPrivate;
class QScanThread;

class QCoreWlanEngine : public QBearerEngineImpl
{
     friend class QScanThread;
    Q_OBJECT

public:
    QCoreWlanEngine(QObject *parent = 0);
    ~QCoreWlanEngine();

    QString getInterfaceFromId(const QString &id);
    bool hasIdentifier(const QString &id);

    QString bearerName(const QString &id);

    void connectToId(const QString &id);
    void disconnectFromId(const QString &id);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate();

    QNetworkSession::State sessionStateForId(const QString &id);

    QNetworkConfigurationManager::Capabilities capabilities() const;

    QNetworkSessionPrivate *createSessionBackend();

    QNetworkConfigurationPrivatePointer defaultConfiguration();

    bool requiresPolling() const;

private Q_SLOTS:
    void doRequestUpdate();
    void networksChanged();

private:
    bool isWifiReady(const QString &dev);
    QList<QNetworkConfigurationPrivate *> foundConfigurations;

    SCDynamicStoreRef storeSession;
    CFRunLoopSourceRef runloopSource;
    bool hasWifi;
    bool scanning;
    QScanThread *scanThread;

protected:
    void startNetworkChangeLoop();

};

class QScanThread : public QThread
{
    Q_OBJECT

public:
    QScanThread(QObject *parent = 0);
    ~QScanThread();

    void quit();
    QList<QNetworkConfigurationPrivate *> getConfigurations();
    QString interfaceName;
    QMap<QString, QString> configurationInterface;
    void getUserConfigurations();
    QString getNetworkNameFromSsid(const QString &ssid);
    QString getSsidFromNetworkName(const QString &name);
    bool isKnownSsid(const QString &ssid);
    QMap<QString, QMap<QString,QString> > userProfiles;

signals:
    void networksChanged();

protected:
    void run();

private:
    QList<QNetworkConfigurationPrivate *> fetchedConfigurations;
    QMutex mutex;
    QStringList foundNetwork(const QString &id, const QString &ssid, const QNetworkConfiguration::StateFlags state, const QString &interfaceName, const QNetworkConfiguration::Purpose purpose);

};

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT

#endif
