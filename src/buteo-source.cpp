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

bool ButeoSource::connected() const
{
    return (m_bus != nullptr);
}

void ButeoSource::open(const Transfer::Id &id)
{
    qDebug() << "Buteo open" << QString::fromStdString(id);
    open_app(id);
}

void ButeoSource::start(const Transfer::Id &id)
{
    qDebug() << "start" << QString::fromStdString(id);
    GError *gError = nullptr;
    GVariant *reply = g_dbus_connection_call_sync(m_bus,
                                                  BUTEO_SERVICE_NAME,
                                                  BUTEO_OBJECT_PATH,
                                                  BUTEO_DBUS_INTEFACE,
                                                  "startSync",
                                                  g_variant_new("(s)", id.c_str()),
                                                   G_VARIANT_TYPE("(b)"),
                                                  G_DBUS_CALL_FLAGS_NONE,
                                                  -1,
                                                  m_cancellable,
                                                  &gError);
    gboolean result = FALSE;
    if (gError) {
        qWarning() << "Fail to start sync" << gError->message;
        g_error_free(gError);
    } else {
        g_variant_get_child(reply, 0, "b", &result);
    }
    g_variant_unref(reply);

    if (!result) {
        qWarning() << "Fail to start sync for profile" <<  QString::fromStdString(id);
    }
}

void ButeoSource::pause(const Transfer::Id &id)
{
    Q_UNUSED(id);
    qWarning() << "Buteo plugin does not support pause";
}

void ButeoSource::resume(const Transfer::Id &id)
{
    Q_UNUSED(id);
    qWarning() << "Buteo plugin does not support resume";
}

void ButeoSource::cancel(const Transfer::Id &id)
{
    GError *gError = nullptr;
    GVariant *reply = g_dbus_connection_call_sync(m_bus,
                                                  BUTEO_SERVICE_NAME,
                                                  BUTEO_OBJECT_PATH,
                                                  BUTEO_DBUS_INTEFACE,
                                                  "abortSync",
                                                  g_variant_new("(s)", id.c_str()),
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
    Q_UNUSED(connection);
    Q_UNUSED(senderName);
    Q_UNUSED(objectPath);
    Q_UNUSED(interfaceName);
    Q_UNUSED(signalName);

    const gchar *profileId = nullptr;
    g_variant_get_child(parameters, 0, "&s", &profileId);

    gint status = -1;
    g_variant_get_child(parameters, 1, "i", &status);

    const gchar *message = nullptr;
    g_variant_get_child(parameters, 2, "&s", &message);

    gint moreDetails = -1;
    g_variant_get_child(parameters, 3, "i", &moreDetails);

    qDebug() << "Profile" << profileId << "\n"
             << "\tStatus" << status << "\n"
             << "\tMessage" << message << "\n"
             << "\tDetails" << moreDetails;

    std::shared_ptr<Transfer> transfer = self->m_model->get(profileId);
    if (!transfer) {
        QMap<QString, QVariant> fields = self->profileFields(profileId);
        transfer = std::shared_ptr<Transfer>(new ButeoTransfer(profileId, fields));
        self->m_model->add(transfer);
        qDebug() << "Add new profile"
                 << QString::fromUtf8(profileId)
                 << QString::fromStdString(transfer->title);
    }

    std::static_pointer_cast<ButeoTransfer>(transfer)->updateStatus(status, message, moreDetails);
    self->m_model->emit_changed(transfer->id);

    if (transfer->state == Transfer::CANCELED) {
        self->m_model->remove(transfer->id);
    }
}

void ButeoSource::setBus(GDBusConnection *bus)
{
    if (m_bus == bus) {
        return;
    }

    if (m_bus) {
        g_dbus_connection_signal_unsubscribe(m_bus, m_syncStatusId);
        m_syncStatusId = -1;
        m_model.reset();
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
