/*
 * Copyright 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Renato Araujo Oliveira Filho <renato.filho@canonical.com>
 */

#include "buteo-transfer.h"

#include <QtCore/QUuid>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Application>

#include <url-dispatcher.h>
#include <indicator-transfer/transfer/transfer.h>

using namespace unity::indicator::transfer;

ButeoTransfer::ButeoTransfer(const QString &profileId,
                             const QMap<QString, QVariant> &fields)
    : m_profileId(profileId),
      m_state(0)
{
    id = QUuid::createUuid().toString().toStdString();
    m_category = fields.value("category", "contacts").toString();

    // retrieve account
    int accountId = fields.value("accountid", 0).toInt();
    QString serviceName = fields.value("remote_service_name", "").toString();
    if (accountId > 0) {
        Accounts::Manager manager;
        Accounts::Account *account = manager.account(accountId);
        if (account) {
            title = account->displayName().toStdString();
            delete account;
        } else {
            qWarning() << "Account not found" << accountId;
        }

        Accounts::Service service = manager.service(serviceName);
        if (service.isValid()) {
            Accounts::ApplicationList apps = manager.applicationList(service);
            if (!apps.isEmpty()) {
                // we only consider the first app for now
                // TODO: check if we need care about a list of apps
                Accounts::Application app = apps.first();
                app_icon = app.iconName().toStdString();

                if (app.desktopFilePath().isEmpty()) {
                    m_appUrl = QString("%1://").arg(app.name());
                } else {
                    QFileInfo desktopIfon(app.desktopFilePath());
                    m_appUrl = QString("application:///%1").arg(desktopIfon.fileName());
                }
            } else {
                qWarning() << "No application found for service" << serviceName;
            }
        } else {
            qWarning() << "Service not found" << serviceName;
        }
    }
}

QString ButeoTransfer::profileId() const
{
    return m_profileId;
}

void ButeoTransfer::launchApp() const
{
    qDebug() << "application url" << m_appUrl;
    url_dispatch_send(m_appUrl.toUtf8().data(), NULL, NULL);
}

void ButeoTransfer::setState(int state)
{
    // sync states
    //SYNC_PROGRESS_INITIALISING = 201,
    //SYNC_PROGRESS_SENDING_ITEMS ,
    //SYNC_PROGRESS_RECEIVING_ITEMS,
    //SYNC_PROGRESS_FINALISING

    // the state can be a sync state or a sync progress
    // any value bigger than 200 is a sync state
    if (state >= 200) {
        m_state = state;
        progress = syncProgress(0);
    } else {
        progress = syncProgress(state);
    }

}

qreal ButeoTransfer::syncProgress(int progress) const
{
    // the progress is a combination of sync state and the progress
    qreal realProgress = progress;
    switch(m_state) {
    case 201: //SYNC_PROGRESS_INITIALISING
        realProgress = 0.1;
        break;
    case 203: //SYNC_PROGRESS_RECEIVING_ITEMS
        break;
    case 202: //SYNC_PROGRESS_SENDING_ITEMS
        realProgress += 100.0;
        break;
    case 204: //SYNC_PROGRESS_FINALISING
        realProgress = 200.0;
        break;
    }
    return (realProgress / 200.0);
}

bool ButeoTransfer::can_resume() const
{
    return false;
}

bool ButeoTransfer::can_pause() const
{
    return false;
}
