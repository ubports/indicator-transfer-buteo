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
#include <url-dispatcher.h>
#include <indicator-transfer/transfer/transfer.h>

using namespace unity::indicator::transfer;
static const QString iconPath("/usr/share/icons/suru/apps/scalable");


ButeoTransfer::ButeoTransfer(const QString &profileId,
                             const QMap<QString, QVariant> &fields)
    : m_profileId(profileId),
      m_state(0)
{
    id = QUuid::createUuid().toString().toStdString();
    m_category = fields.value("category", "contacts").toString();
    QString displayName = fields.value("displayname", "Account").toString();
    QString serviceName = fields.value("remote_service_name", "").toString();
    title = displayName.mid(serviceName.length() + 1).toStdString();
    // FIXME: load icons from profile or theme
    if (m_category == "contacts") {
        app_icon = QString("%1/address-book-app-symbolic.svg").arg(iconPath).toStdString();
    } else {
        app_icon = QString("%1/calendar-app-symbolic.svg").arg(iconPath).toStdString();
    }
}

QString ButeoTransfer::profileId() const
{
    return m_profileId;
}

QString ButeoTransfer::launchApp() const
{
    QString url;
    if (m_category == "contacts") {
        url = QString("application:///address-book-app");
    } else if (m_category == "calendar") {
        url = QString("application:///calendar-app");
    }
    url_dispatch_send(url.toUtf8().data(), NULL, NULL);
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
        break;
    case 203: //SYNC_PROGRESS_RECEIVING_ITEMS
        realProgress += 100.0;
        break;
    case 202: //SYNC_PROGRESS_SENDING_ITEMS
        realProgress += 200.0;
        break;
    case 204: //SYNC_PROGRESS_FINALISING
        realProgress = 300.0;
        break;
    }
    return (realProgress / 300.0);
}

bool ButeoTransfer::can_resume() const
{
    return false;
}

bool ButeoTransfer::can_pause() const
{
    return false;
}
