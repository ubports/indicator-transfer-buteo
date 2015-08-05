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

#include <indicator-transfer/transfer/transfer.h>
#include <memory>

#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QMap>

namespace unity {
namespace indicator {
namespace transfer {

class ButeoTransfer : public Transfer
{
public:
    ButeoTransfer(const QString &profileId,
                  const QMap<QString, QVariant> &fields);
    void launchApp() const;
    void setState(int state);
    void reset();

    bool can_resume() const override;
    bool can_pause() const override;

private:
    QString m_category;
    QString m_appUrl;
    int m_state;

    qreal syncProgress(int progress) const;
};

} // namespace transfer
} // namespace indicator
} // namespace unity
