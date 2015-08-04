#include "buteo-source.h"

#include <QObject>
#include <QTest>

using namespace unity::indicator::transfer;

class TstButeoTransferPlugin : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tst_createPlugin()
    {
        QScopedPointer<ButeoSource> plugin(new ButeoSource);
    }
};

QTEST_MAIN(TstButeoTransferPlugin)

#include "tst-transfer-plugin.moc"

