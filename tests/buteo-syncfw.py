#!/usr/bin/python3

'''buteo syncfw mock template

This creates the expected methods and properties of the main
com.meego.msyncd object. You can specify D-BUS property values
'''

# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.  See http://www.gnu.org/copyleft/lgpl.html for the full text
# of the license.

__author__ = 'Renato Araujo Oliveira Filho'
__email__ = 'renatofilho@canonical.com'
__copyright__ = '(c) 2015 Canonical Ltd.'
__license__ = 'LGPL 3+'

import dbus
from gi.repository import GObject

import dbus
import dbus.service
import dbus.mainloop.glib

BUS_NAME = 'com.meego.msyncd'
MAIN_OBJ = '/synchronizer'
MAIN_IFACE = 'com.meego.msyncd'
SYSTEM_BUS = False

class ButeoSyncFw(dbus.service.Object):
    PROFILES = [
"""<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<profile type=\"sync\" name=\"test-profile\">
    <key value=\"45\" name=\"accountid\"/>
    <key value=\"contacts\" name=\"category\"/>
    <key value=\"google-contacts-ubuntu@gmail.com\" name=\"displayname\"/>
    <key value=\"true\" name=\"enabled\"/>
    <key value=\"google-contacts\" name=\"remote_service_name\"/>
    <key value=\"true\" name=\"hidden\"/>
    <key value=\"30\" name=\"sync_since_days_past\"/>
    <key value=\"true\" name=\"use_accounts\"/>
    <profile type=\"client\" name=\"googlecontacts\">
        <key value=\"two-way\" name=\"Sync Direction\"/>
    </profile>
    <schedule time=\"05:00:00\" days=\"4,5,2,3,1,6,7\" syncconfiguredtime=\"\" interval=\"0\" enabled=\"true\">
        <rush end=\"\" externalsync=\"false\" days=\"\" interval=\"15\" begin=\"\" enabled=\"false\"/>
    </schedule>
</profile>
"""]

    def __init__(self, object_path):
        dbus.service.Object.__init__(self, dbus.SessionBus(), object_path)
        self._activeSync = []
        self._profiles = ButeoSyncFw.PROFILES

    @dbus.service.method(dbus_interface=MAIN_IFACE,
                         in_signature='s', out_signature='')
    def abortSync(self, profileId):
        self.syncStatus(profileId, 5, 'aborted by the user', 0)

    @dbus.service.method(dbus_interface=MAIN_IFACE,
                         in_signature='s', out_signature='b')
    def startSync(self, profileId):
        GObject.timeout_add(200, self.notifySyncQueued, profileId)
        GObject.timeout_add(400, self.notifySyncStarted, profileId)
        GObject.timeout_add(600, self.notifySyncProgress, profileId)
        GObject.timeout_add(800, self.notifySyncFinished, profileId)

    @dbus.service.method(dbus_interface=MAIN_IFACE,
                         in_signature='', out_signature='as')
    def runningSyncs(self):
        return self._activeSync

    @dbus.service.method(dbus_interface=MAIN_IFACE,
                         in_signature='', out_signature='s')
    def syncProfile(self, profileId):
        return self._profiles[0]

    @dbus.service.signal(dbus_interface=MAIN_IFACE,
                         signature='sisi')
    def syncStatus(self, profileId, status, message, statusDetails):
        print("SyncStatus called", profileId, status, message, statusDetails)

    def notifySyncQueued(self, profileId):
        #QUEUED(0)
        self.syncStatus(profileId, 0, "", 0)
        return False

    def notifySyncStarted(self, profileId):
        #RUNNING(1)
        self.syncStatus(profileId, 1, "", 10)
        return False

    def notifySyncProgress(self, profileId):
        #PROGRESS(2)
        self.syncStatus(profileId, 2, "", 20)
        return False

    def notifySyncFinished(self, profileId):
        #DONE(4)
        self.syncStatus(profileId, 4, "", 100)
        return False


if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    name = dbus.service.BusName(BUS_NAME)
    mainloop = GObject.MainLoop()
    buteo = ButeoSyncFw(MAIN_OBJ)
    mainloop.run()

