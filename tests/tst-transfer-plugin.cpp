#include "buteo-source.h"
#include "buteo-transfer.h"

#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QtDBus/QDBusMessage>
#include <QTest>

#define BUTEO_SERVICE_NAME  "com.meego.msyncd"
#define BUTEO_OBJECT_PATH   "/synchronizer"
#define BUTEO_DBUS_INTEFACE  "com.meego.msyncd"

using namespace unity::indicator::transfer;

class TstButeoTransferPlugin : public QObject
{
    Q_OBJECT
private:
    struct Event
    {
        typedef enum { ADDED, CHANGED, REMOVED } Type;
        Type type;
        QString profileId;

        Event(Type t, const QString &s)
            : type(t), profileId(s)
        {}

        Event(const Event &other)
            : type(other.type),
              profileId(other.profileId)
        {}

        bool operator==(const Event& that) const
        {
            return type==that.type && profileId==that.profileId;
        }
    };

private Q_SLOTS:
    void tst_singleSync()
    {
        QScopedPointer<ButeoSource> plugin(new ButeoSource);
        QQueue<Event> events;

        plugin->get_model()->added().connect([&events, &plugin](const Transfer::Id& id){
            events.append(Event(Event::ADDED, QString::fromStdString(id)));
        });

        plugin->get_model()->changed().connect([&events, &plugin](const Transfer::Id& id){
            events.append(Event(Event::CHANGED, QString::fromStdString(id)));
        });

        // start sync
        QTRY_VERIFY(plugin->connected());
        plugin->start(QString("profile-123").toStdString());
        QTRY_COMPARE(events.size(), 5);

        // start event
        Event e = events.takeFirst();
        QCOMPARE(e.type, Event::ADDED);
        QCOMPARE(e.profileId, QStringLiteral("profile-123"));

        // changed(QUEUED)
        e = events.takeFirst();
        QCOMPARE(e.type, Event::CHANGED);
        QCOMPARE(e.profileId, QStringLiteral("profile-123"));

        // changed(STARTED)
        e = events.takeFirst();
        QCOMPARE(e.type, Event::CHANGED);
        QCOMPARE(e.profileId, QStringLiteral("profile-123"));

        // changed(RUNNING)
        e = events.takeFirst();
        QCOMPARE(e.type, Event::CHANGED);
        QCOMPARE(e.profileId, QStringLiteral("profile-123"));

        // changed(FINISHED)
        e = events.takeFirst();
        QCOMPARE(e.type, Event::CHANGED);
        QCOMPARE(e.profileId, QStringLiteral("profile-123"));
    }
};

QTEST_MAIN(TstButeoTransferPlugin)

#include "tst-transfer-plugin.moc"

