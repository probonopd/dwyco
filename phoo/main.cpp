
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
;
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QSettings>
#include <QHostInfo>
#include <QQuickStyle>
#include <QDebug>
#ifdef ANDROID
#include "notificationclient.h"
#include <QAndroidJniObject>
#endif

QQmlApplicationEngine *TheEngine;

void dwyco_register_qml(QQmlContext *root);
#ifdef ANDROID
NotificationClient *notificationClient;
#endif
static
void
myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
#if 1
    if(msg.contains("Timers cannot be stopped from another thread"))
        ::abort();
    if(msg.contains("Timers can only be used with threads started with QThread"))
        ::abort();
#endif

}

int main(int argc, char *argv[])
{
#if 0 && defined(DWYCO_RELEASE)
    qInstallMessageHandler(myMessageOutput);
#endif

#ifdef ANDROID
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif


    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Material");
    QQuickStyle::setFallbackStyle("Material");

    qDebug() << QQuickStyle::availableStyles();

    // note: qt seems to use some of these names in constructing
    // file names. this can be a problem if different FS's with different
    // naming conventions are being used. this manifests itself with
    // file names conflicting, and problems copying existing files from
    // different machines. it gets worse if you are using file syncing
    // things like dropbox and btsync.
    QCoreApplication::setOrganizationName("dwyco");
    QCoreApplication::setOrganizationDomain("dwyco.com");
    // if we run one copy of a cdc-x install on multiple machines,
    // identify the settings for the machine by local hostname.
    // this allows for differences in devices and stuff on that host.
    QString LocalHostName = QHostInfo::localHostName();
    QCoreApplication::setApplicationName(QString("phoo") + LocalHostName);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    // note: need to set the path to the right place, same as fn_pfx for dll
    //QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, FPATH);

    QQmlApplicationEngine engine;
    TheEngine = &engine;


#ifdef ANDROID
    notificationClient = new NotificationClient(&engine);
    engine.rootContext()->setContextProperty(QLatin1String("notificationClient"),
            notificationClient);
    notificationClient->set_allow_notification(0);
#endif

    dwyco_register_qml(engine.rootContext());
    // this screen resolution stuff was stolen from
    // qt world summit talk 2015 by vplay guy
    QScreen *screen = app.primaryScreen();

    qreal dpi;
#if defined(Q_OS_WIN)
    dpi = screen->logicalDotsPerInch() * app.devicePixelRatio();
#elif defined(Q_OS_ANDROID)
    QAndroidJniObject qtActivity =
        QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative",
                "activity", "()Landroid/app/Activity;");
    QAndroidJniObject resources = qtActivity.callObjectMethod("getResources",
                                  "()Landroid/content/res/Resources;");
    QAndroidJniObject displayMetrics = resources.callObjectMethod("getDisplayMetrics",
                                       "()Landroid/util/DisplayMetrics;");
    int density = displayMetrics.getField<int>("densityDpi");
    dpi = density;
#else
    dpi = screen->physicalDotsPerInch() * app.devicePixelRatio();
#endif


    engine.rootContext()->setContextProperty("screenDpi", dpi);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
