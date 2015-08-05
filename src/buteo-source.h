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

#include <memory>
#include <indicator-transfer/transfer/source.h>

#include <QtCore/QMap>
#include <QtCore/QCoreApplication>
#include <QtCore/QScopedPointer>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusServiceWatcher>

namespace unity {
namespace indicator {
namespace transfer {

class ButeoTransfer;

class ButeoSource : public Source
{
public:
    ButeoSource();
    ~ButeoSource();

    bool connected() const;
    void open(const Transfer::Id& id) override;
    void start(const Transfer::Id& id) override;
    void pause(const Transfer::Id& id) override;
    void resume(const Transfer::Id& id) override;
    void cancel(const Transfer::Id& id) override;
    void open_app(const Transfer::Id& id) override;

    std::shared_ptr<MutableModel> get_model() override;

private:
    GCancellable *m_cancellable;
    GDBusConnection *m_bus;
    guint m_syncStatusId;

    std::shared_ptr<MutableModel> m_model;

    void setBus(GDBusConnection *bus);

    QMap<QString, QVariant> profileFields(const QString &profileId) const;

    static void onBusReady(GObject *object, GAsyncResult *res, ButeoSource *self);
    static void onSyncStatus(GDBusConnection* connection,
                             const gchar* senderName,
                             const gchar* objectPath,
                             const gchar* interfaceName,
                             const gchar* signalName,
                             GVariant* parameters,
                             ButeoSource* self);
};

} // namespace transfer
} // namespace indicator
} // namespace unity

