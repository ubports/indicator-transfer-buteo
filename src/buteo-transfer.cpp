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

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

#include <Accounts/Manager>
#include <Accounts/Account>
#include <Accounts/Application>

#include <url-dispatcher.h>
#include <indicator-transfer/transfer/transfer.h>

#include <glib/gi18n-lib.h>

using namespace unity::indicator::transfer;

ButeoTransfer::ButeoTransfer(const QString &profileId,
                             const QVariantMap &fields)
{
    id = profileId.toStdString();
    state = Transfer::QUEUED;
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

void ButeoTransfer::launchApp() const
{
    qDebug() << "application url" << m_appUrl;
    url_dispatch_send(m_appUrl.toUtf8().data(), NULL, NULL);
}

void ButeoTransfer::updateStatus(int status, const QString &message, int moreDetails)
{
    /*  status
      0 (QUEUED): Sync request has been queued or was already in the
          queue when sync start was requested.
      1 (STARTED): Sync session has been started.
      2 (PROGRESS): Sync session is progressing.
      3 (ERROR): Sync session has encountered an error and has been stopped,
          or the session could not be started at all.
      4 (DONE): Sync session was successfully completed.
      5 (ABORTED): Sync session was aborted.
    */
    switch(status) {
    case 0:
        state = Transfer::QUEUED;
        // reset transfer in case it be an old transfer
        reset();
        break;
    case 1:
    case 2:
        state = Transfer::RUNNING;
        updateProgress(moreDetails);
        break;
    case 3:
        state = Transfer::ERROR;
        error_string = message.toStdString();
        break;
    case 4:
        state = Transfer::FINISHED;
        break;
    case 5:
        state = Transfer::CANCELED;
        break;
    }

    if (state == Transfer::RUNNING) {
        custom_state = _("Syncing");
    } else {
        custom_state = "";
    }
}

void ButeoTransfer::reset()
{
    m_state = 0;
    progress = 0.0;
    error_string = "";
}

void ButeoTransfer::updateProgress(int progress)
{
    // sync moreDetails
    //SYNC_PROGRESS_INITIALISING = 201,
    //SYNC_PROGRESS_SENDING_ITEMS ,
    //SYNC_PROGRESS_RECEIVING_ITEMS,
    //SYNC_PROGRESS_FINALISING

    qreal realProgress = progress;
    // the state can be a sync state or a sync progress
    // any value bigger than 200 is a sync state
    if (progress >= 200) {
        m_state = progress;
        realProgress = 0;
    }

    // the progress is a combination of sync state and the progress
    switch(m_state) {
    case 201: //SYNC_PROGRESS_INITIALISING
        realProgress = 1.0;
        break;
    case 203: //SYNC_PROGRESS_RECEIVING_ITEMS [0..100]
        break;
    case 202: //SYNC_PROGRESS_SENDING_ITEMS [100..200]
        realProgress += 100.0;
        break;
    case 204: //SYNC_PROGRESS_FINALISING
        realProgress = 200.0;
        break;
    }

    if (realProgress > 0) {
        this->progress = (realProgress / 200.0);
    } else {
        this->progress = 0.0;
    }
}

bool ButeoTransfer::can_start() const
{
    switch(state) {
    case Transfer::ERROR:
    case Transfer::CANCELED:
        return true;
    default:
        return false;
    }
}

bool ButeoTransfer::can_pause() const
{
    return false;
}
