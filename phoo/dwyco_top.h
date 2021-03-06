
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
;
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#ifndef DWYCO_TOP_H
#define DWYCO_TOP_H
// This class implements an encapsulation of the Dwyco API using
// Qt/QML.

#include <QObject>
#include <QVariant>
#include <QUrl>
#include "dlli.h"
#include "QQmlVarPropertyHelpers.h"
#include <QAbstractListModel>
#ifndef NO_BUILDTIME
#include "buildtime.h"
#else
#define BUILDTIME "debug"
#endif
class DwycoCore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString client_name READ client_name WRITE setClient_name NOTIFY client_nameChanged)
    QML_READONLY_VAR_PROPERTY(int, unread_count)
    QML_READONLY_VAR_PROPERTY(QString, buildtime)
    QML_READONLY_VAR_PROPERTY(QString, user_dir)
    QML_READONLY_VAR_PROPERTY(QString, tmp_dir)

public:
    DwycoCore(QObject *parent = 0) : QObject(parent) {
        m_unread_count = 0;
        m_client_name = "";
        m_buildtime = BUILDTIME;
        m_user_dir = ".";
        m_tmp_dir = ".";
    }
    static QByteArray My_uid;


    enum System_event {
        USER_STATUS_CHANGE = DWYCO_SE_USER_STATUS_CHANGE,
        USER_ADD = DWYCO_SE_USER_ADD,
        USER_DEL = DWYCO_SE_USER_DEL,
        SERVER_CONNECTING = DWYCO_SE_SERVER_CONNECTING,
        SERVER_CONNECTION_SUCCESSFUL = DWYCO_SE_SERVER_CONNECTION_SUCCESSFUL,
        SERVER_DISCONNECT = DWYCO_SE_SERVER_DISCONNECT,
        SERVER_LOGIN = DWYCO_SE_SERVER_LOGIN,
        SERVER_LOGIN_FAILED = DWYCO_SE_SERVER_LOGIN_FAILED,
        USER_MSG_RECEIVED = DWYCO_SE_USER_MSG_RECEIVED,
        USER_UID_RESOLVED = DWYCO_SE_USER_UID_RESOLVED,
        USER_PROFILE_INVALIDATE = DWYCO_SE_USER_PROFILE_INVALIDATE,
        USER_MSG_IDX_UPDATED = DWYCO_SE_USER_MSG_IDX_UPDATED,
        USER_MSG_IDX_UPDATED_PREPEND = DWYCO_SE_USER_MSG_IDX_UPDATED_PREPEND,
        MSG_SEND_START = DWYCO_SE_MSG_SEND_START,
        MSG_SEND_FAIL = DWYCO_SE_MSG_SEND_FAIL,
        MSG_SEND_SUCCESS = DWYCO_SE_MSG_SEND_SUCCESS,
        MSG_SEND_STATUS = DWYCO_SE_MSG_SEND_STATUS,
        MSG_SEND_DELIVERY_SUCCESS = DWYCO_SE_MSG_SEND_DELIVERY_SUCCESS,
        MSG_SEND_CANCELED = DWYCO_SE_MSG_SEND_CANCELED
    };

    enum Profile_info {
        HANDLE,
        EMAIL,
        LOCATION,
        DESCRIPTION,
        REVIEWED,
        REGULAR
    };

    enum Chat_event {
        CHAT_CTX_NEW = DWYCO_CHAT_CTX_NEW,
        CHAT_CTX_DEL = DWYCO_CHAT_CTX_DEL,
        CHAT_CTX_ADD_USER = DWYCO_CHAT_CTX_ADD_USER,
        CHAT_CTX_DEL_USER = DWYCO_CHAT_CTX_DEL_USER,
        CHAT_CTX_UPDATE_AH = DWYCO_CHAT_CTX_UPDATE_AH,
        CHAT_CTX_START_UPDATE = DWYCO_CHAT_CTX_START_UPDATE,
        CHAT_CTX_END_UPDATE = DWYCO_CHAT_CTX_END_UPDATE,
        CHAT_CTX_Q_ADD = DWYCO_CHAT_CTX_Q_ADD,
        CHAT_CTX_Q_DEL = DWYCO_CHAT_CTX_Q_DEL,
        CHAT_CTX_Q_GRANT = DWYCO_CHAT_CTX_Q_GRANT,
        CHAT_CTX_Q_DATA = DWYCO_CHAT_CTX_Q_DATA,
        CHAT_CTX_SYS_ATTR = DWYCO_CHAT_CTX_SYS_ATTR,
        CHAT_CTX_UPDATE_ATTR = DWYCO_CHAT_CTX_UPDATE_ATTR,
        CHAT_CTX_ADD_LOBBY = DWYCO_CHAT_CTX_ADD_LOBBY,
        CHAT_CTX_DEL_LOBBY = DWYCO_CHAT_CTX_DEL_LOBBY,
        CHAT_CTX_ADD_GOD = DWYCO_CHAT_CTX_ADD_GOD,
        CHAT_CTX_DEL_GOD = DWYCO_CHAT_CTX_DEL_GOD,
        CHAT_CTX_RECV_DATA = DWYCO_CHAT_CTX_RECV_DATA
    };

    Q_ENUM(System_event)
    Q_ENUM(Profile_info)
    Q_ENUM(Chat_event)

    Q_INVOKABLE void init();
    Q_INVOKABLE int service_channels();
    Q_INVOKABLE void exit() {
        //dwyco_empty_trash();
        //dwyco_power_clean_safe();
        dwyco_exit();
    }

    Q_INVOKABLE void power_clean() {
        dwyco_power_clean_safe();
    }

    Q_INVOKABLE QString get_my_uid() {
        return My_uid.toHex();
    }

    Q_INVOKABLE int database_online();

    Q_INVOKABLE QUrl get_simple_directory_url();
    Q_INVOKABLE QUrl get_simple_xml_url();
    Q_INVOKABLE QString get_msg_count_url();
    Q_INVOKABLE QString url_to_filename(QUrl);
    Q_INVOKABLE int simple_send(QString recipient, QString msg);
    Q_INVOKABLE int simple_send_file(QString recipient, QString msg, QString filename);
    Q_INVOKABLE int simple_send_url(QString recipient, QString msg, QUrl filename);
    Q_INVOKABLE int send_simple_cam_pic(QString recipient, QString msg, QString filename);
    Q_INVOKABLE int is_file_zap(int compid) {
        return dwyco_is_file_zap(compid);
    }

    Q_INVOKABLE int send_forward(QString recipient, QString add_text, QString uid_folder, QString mid_to_forward, int save_sent);
    Q_INVOKABLE int flim(QString uid_folder, QString mid_to_forward);

    Q_INVOKABLE void send_chat(QString text);

    Q_INVOKABLE QString uid_to_name(QString uid);
    Q_INVOKABLE QString uid_to_profile_info(QString uid, enum Profile_info field);
    Q_INVOKABLE bool uid_profile_regular(QString uid);
    Q_INVOKABLE QUrl uid_to_profile_preview(QString uid);
    Q_INVOKABLE QUrl uid_to_http_profile_preview(QString uid);
    Q_INVOKABLE QUrl uid_to_profile_view(QString uid);
    Q_INVOKABLE QString uid_to_profile_image_filename(QString uid);

    Q_INVOKABLE int set_setting(QString name, QString value);
    Q_INVOKABLE QVariant get_setting(QString name);

    Q_INVOKABLE void set_local_setting(QString name, QString value);
    Q_INVOKABLE QString get_local_setting(QString name);

    Q_INVOKABLE void bootstrap(QString name, QString email);
    Q_INVOKABLE int create_new_account();
    Q_INVOKABLE QString random_string(int len);

    Q_INVOKABLE int set_simple_profile(QString handle, QString email, QString desc, QString img_fn);

    Q_INVOKABLE void set_pal(QString uid, int is_pal);
    Q_INVOKABLE int get_pal(QString uid);

    Q_INVOKABLE void set_ignore(QString uid, int is_ignored);
    Q_INVOKABLE int get_ignore(QString uid);
    Q_INVOKABLE void clear_ignore_list();

    Q_INVOKABLE void reset_unviewed_msgs(QString uid);

    Q_INVOKABLE int make_zap_view(QString uid, QString mid);
    Q_INVOKABLE int make_zap_view_file(QString fn);
    Q_INVOKABLE int delete_zap_view(int view_id) {
        return dwyco_delete_zap_view(view_id);
    }
    Q_INVOKABLE int play_zap_preview(int view_id) {
        return 0;
    }
    Q_INVOKABLE int play_zap_view(int view_id);
    Q_INVOKABLE int stop_zap_view(int view_id) {
        return dwyco_zap_stop_view(view_id);
    }

    // dwyco video record
    Q_INVOKABLE int make_zap_composition();
    Q_INVOKABLE int start_zap_record(int zid);
    Q_INVOKABLE int start_zap_record_pic(int zid);
    Q_INVOKABLE int stop_zap_record(int zid);
    Q_INVOKABLE int play_zap(int zid);
    Q_INVOKABLE int stop_zap(int zid);
    Q_INVOKABLE int cancel_zap(int zid);
    Q_INVOKABLE int send_zap(int zid, QString recipient, int save_sent);

    Q_INVOKABLE int delete_message(QString uid, QString mid);
    Q_INVOKABLE int clear_messages(QString uid);
    Q_INVOKABLE int clear_messages_unfav(QString uid);
    Q_INVOKABLE int delete_user(QString uid);
    Q_INVOKABLE int get_fav_message(QString mid);
    Q_INVOKABLE void set_fav_message(QString uid, QString mid, int val);

    Q_INVOKABLE void uid_keyboard_input(QString uid);
    Q_INVOKABLE int get_rem_keyboard_state(QString uid);
    Q_INVOKABLE void create_call_context(QString uid);
    Q_INVOKABLE void delete_call_context(QString uid);
    Q_INVOKABLE void try_connect(QString uid);
    Q_INVOKABLE int get_established_state(QString uid);

    Q_INVOKABLE void delete_file(QString fn);

    // chat server related
    Q_INVOKABLE void switch_to_chat_server(int);
    Q_INVOKABLE void disconnect_chat_server();
    Q_INVOKABLE void set_invisible_state(int);

    // contact list related functions
    Q_INVOKABLE void load_contacts();
    Q_INVOKABLE QUrl get_cq_results_url();
    Q_INVOKABLE void delete_cq_results();

    // control for auto/manual fetching
    Q_INVOKABLE int retry_auto_fetch(QString mid);

    Q_INVOKABLE bool get_cc_property(QString uid, QString button_name, QString property_name);
    Q_INVOKABLE void set_cc_property(QString uid, QString button_name, QString property_name, bool val);
    //Q_INVOKABLE void invoke_button(QString uid, QString buton_name, QString);
    Q_INVOKABLE QObject *get_button_model(QString uid);

    // dwyco video camera api
    Q_INVOKABLE void select_vid_dev(int i);
    Q_INVOKABLE void enable_video_capture_preview(int i);

public:
    void setClient_name(const QString& a) {
        if(a != m_client_name)
        {
            m_client_name = a;
            QByteArray b = a.toLatin1();
            dwyco_set_client_version(b.constBegin(), b.length());
            emit client_nameChanged();
        }
    }

    QString client_name() const {
        return m_client_name;
    }


public slots:
    void app_state_change(Qt::ApplicationState);

signals:
    void client_nameChanged();
    void server_login(const QString& msg, int what);
    void chat_event(int cmd, int sid, const QString& huid, const QString &sname, QVariant vdata, int qid, int extra_arg);
    void new_msg(const QString& from_uid, const QString& txt, const QString& mid);
    void msg_send_status(int status, const QString& recipient, const QString& pers_id);
    void msg_progress(const QString& pers_id, const QString& recipient, const QString& msg, int percent_done);
    void sys_invalidate_profile(const QString& uid);
    void sys_msg_idx_updated(const QString& uid);
    void sys_uid_resolved(const QString& uid);
    void profile_update(int success);
    void pal_event(const QString& uid);
    void ignore_event(const QString& uid);
    void video_display(int ui_id, int frame_number, QString img_path);
    void video_capture_preview(QString img_path);
// this is used internally, should not fiddle with it via QML
    void user_control(int, QByteArray, QByteArray);
    void decorate_user(const QString& uid);
    void sys_chat_server_status(int id, int status);
    void qt_app_state_change(int app_state);
    void rem_keyboard_active(const QString& uid, int active);
    void image_picked(const QString& fn);
    void established_active(const QString& uid, int active);
    void cq_results_received(int succ);
    void msg_recv_state(int cmd, const QString& mid);
    void msg_recv_progress(const QString& mid, int percent);
    // dwyco video camera signals
    void camera_change(int);
    // zap composition record/play stopped
    void zap_stopped(int zid);

private:
    QString m_client_name;
    static void DWYCOCALLCONV dwyco_chat_ctx_callback(int cmd, int id, const char *uid, int len_uid, const char *name, int len_name, int type, const char *val, int len_val, int qid, int extra_arg);

};

#endif
