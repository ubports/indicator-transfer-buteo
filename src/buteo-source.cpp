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

#include "buteo-source.h"
#include "buteo-transfer.h"

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtDBus/QDBusReply>
#include <QtXml/QDomDocument>

#define BUTEO_SERVICE_NAME  "com.meego.msyncd"
#define BUTEO_OBJECT_PATH   "/synchronizer"
#define BUTEO_DBUS_INTEFACE  "com.meego.msyncd"

using namespace unity::indicator::transfer;

ButeoSource::ButeoSource()
    : m_cancellable(g_cancellable_new()),
      m_model(std::make_shared<MutableModel>()),
      m_bus(nullptr)
{
    g_bus_get(G_BUS_TYPE_SESSION, m_cancellable,
              (GAsyncReadyCallback) onBusReady, this);
}

ButeoSource::~ButeoSource()
{
    g_cancellable_cancel(m_cancellable);
    g_clear_object(&m_cancellable);
    setBus(nullptr);
    g_clear_object(&m_bus);
}

void ButeoSource::open(const Transfer::Id &id)
{
    open_app(id);
}

void ButeoSource::start(const Transfer::Id &id)
{
    QString profile = profileId(id);
    GError *gError = nullptr;
    GVariant *reply = g_dbus_connection_call_sync(m_bus,
                                                  BUTEO_SERVICE_NAME,
                                                  BUTEO_OBJECT_PATH,
                                                  BUTEO_DBUS_INTEFACE,
                                                  "startSync",
                                                  g_variant_new("(s)", profile.toUtf8().data()),
                                                   G_VARIANT_TYPE("(b)"),
                                                  G_DBUS_CALL_FLAGS_NONE,
                                                  -1,
                                                  m_cancellable,
                                                  &gError);
    if (gError) {
        qWarning() << "Fail to start sync" << gError->message;
        g_error_free(gError);
    }
    gboolean result = FALSE;
    g_variant_get_child(reply, 0, "b", &result);
    g_variant_unref(reply);

    if (!result) {
        qWarning() << "Fail to start sync for profile" << profile;
    }
}

void ButeoSource::pause(const Transfer::Id &id)
{
    qWarning() << "Buteo plugin does not support pause";
}

void ButeoSource::resume(const Transfer::Id &id)
{
    qWarning() << "Buteo plugin does not support resume";
}

void ButeoSource::cancel(const Transfer::Id &id)
{
    QString profile = profileId(id);
    GError *gError = nullptr;
    GVariant *reply = g_dbus_connection_call_sync(m_bus,
                                                  BUTEO_SERVICE_NAME,
                                                  BUTEO_OBJECT_PATH,
                                                  BUTEO_DBUS_INTEFACE,
                                                  "abortSync",
                                                  g_variant_new("(s)", profile.toUtf8().data()),
                                                  nullptr,
                                                  G_DBUS_CALL_FLAGS_NONE,
                                                  -1,
                                                  m_cancellable,
                                                  &gError);
    if (gError) {
        qWarning() << "Fail to about sync" << gError->message;
        g_error_free(gError);
    }
    g_variant_unref(reply);
}

void ButeoSource::open_app(const Transfer::Id &id)
{
    std::shared_ptr<Transfer> transfer = m_model->get(id);
    std::static_pointer_cast<ButeoTransfer>(transfer)->launchApp();
}

std::shared_ptr<MutableModel> ButeoSource::get_model()
{
    return m_model;
}

void ButeoSource::onSyncStatus(GDBusConnection* connection,
                               const gchar* senderName,
                               const gchar* objectPath,
                               const gchar* interfaceName,
                               const gchar* signalName,
                               GVariant* parameters,
                               ButeoSource* self)
{
    const gchar *profileName = nullptr;
    g_variant_get_child(parameters, 0, "&s", &profileName);

    gint status = -1;
    g_variant_get_child(parameters, 1, "i", &status);

    const gchar *message = nullptr;
    g_variant_get_child(parameters, 2, "&s", &message);

    gint moreDetails = -1;
    g_variant_get_child(parameters, 3, "i", &moreDetails);

    qDebug() << "Profile" << profileName
             << "Status" << status
             << "Message" << message
             << "Details" << moreDetails;

    QString qProfileName = QString::fromUtf8(profileName);
    std::shared_ptr<Transfer> t = self->m_transfers.value(qProfileName, 0);
    if (!t) {
        QMap<QString, QVariant> fields = self->profileFields(qProfileName);
        t = std::shared_ptr<Transfer>(new ButeoTransfer(qProfileName, fields));
        self->m_transfers.insert(qProfileName, t);
        self->m_model->add(t);
        qDebug() << "Add new profile" << qProfileName << t->title;
    }

    /*
    *      0 (QUEUED): Sync request has been queued or was already in the
    *          queue when sync start was requested.
    *      1 (STARTED): Sync session has been started.
    *      2 (PROGRESS): Sync session is progressing.
    *      3 (ERROR): Sync session has encountered an error and has been stopped,
    *          or the session could not be started at all.
    *      4 (DONE): Sync session was successfully completed.
    *      5 (ABORTED): Sync session was aborted.
    */
    switch(status) {
    case 0:
        t->state = Transfer::QUEUED;
        break;
    case 1:
    case 2:
        t->state = Transfer::RUNNING;
        t->progress = moreDetails > 0.0 ?  moreDetails / 100.0 : -1.0;
        qDebug() << "Update transfer progress" << t->progress;
        break;
    case 3:
        t->state = Transfer::ERROR;
        t->error_string = std::string(message);
        break;
    case 4:
        t->state = Transfer::FINISHED;
        break;
    case 5:
        t->state = Transfer::CANCELED;
        break;
    }
    self->m_model->emit_changed(t->id);
}


void ButeoSource::setBus(GDBusConnection *bus)
{
    if (m_bus == bus) {
        return;
    }

    if (m_bus) {
        g_dbus_connection_signal_unsubscribe(m_bus, m_syncStatusId);
        m_syncStatusId = -1;

        Q_FOREACH(const std::shared_ptr<Transfer> &t, m_transfers.values()) {
            m_model->remove(t->id);
        }
        m_transfers.clear();
        g_object_unref(m_bus);
        m_bus = nullptr;
    }

    if (bus != nullptr) {
        m_bus = G_DBUS_CONNECTION(g_object_ref(bus));;
        m_syncStatusId = g_dbus_connection_signal_subscribe(m_bus,
                                                            BUTEO_SERVICE_NAME,
                                                            BUTEO_DBUS_INTEFACE,
                                                            "syncStatus",
                                                            BUTEO_OBJECT_PATH,
                                                            NULL,
                                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                                            (GDBusSignalCallback) onSyncStatus,
                                                            this,
                                                            nullptr);
    }
}

void ButeoSource::onBusReady(GObject *object, GAsyncResult *res, ButeoSource *self)
{
    Q_UNUSED(object);
    GError* error = nullptr;
    GDBusConnection *bus = g_bus_get_finish(res, &error);

    if (bus) {
        self->setBus(bus);
        g_object_unref(bus);
    } else {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning("Could not get session bus: %s", error->message);
        }
        g_error_free(error);
    }
}


QMap<QString, QVariant> ButeoSource::profileFields(const QString &profileId) const
{
    QMap<QString, QVariant> result;

    GError *gError = nullptr;
    GVariant *reply = g_dbus_connection_call_sync(m_bus,
                                                  BUTEO_SERVICE_NAME,
                                                  BUTEO_OBJECT_PATH,
                                                  BUTEO_DBUS_INTEFACE,
                                                  "syncProfile",
                                                  g_variant_new("(s)", profileId.toUtf8().data()),
                                                  G_VARIANT_TYPE("(s)"),
                                                  G_DBUS_CALL_FLAGS_NONE,
                                                  -1,
                                                  m_cancellable,
                                                  &gError);

    if (gError) {
        qWarning() << "Failt to retrieve profile" << profileId << gError->message;
        g_error_free(gError);
        return result;
    }

    const gchar* profileXml = nullptr;
    g_variant_get_child(reply, 0, "&s", &profileXml);

    // parse Xml
    QDomDocument doc;
    if (doc.setContent(QString::fromUtf8(profileXml))) {
        QDomNodeList keys = doc.elementsByTagName("key");
        for (int i = 0; i < keys.size(); i++) {
            QDomElement element = keys.item(i).toElement();
            result.insert(element.attribute("name"), element.attribute("value"));
        }
    }

    g_variant_unref(reply);
    return result;
}

QString ButeoSource::profileId(const Transfer::Id &id) const
{
    std::shared_ptr<Transfer> transfer = m_model->get(id);
    return std::static_pointer_cast<ButeoTransfer>(transfer)->profileId();
}
