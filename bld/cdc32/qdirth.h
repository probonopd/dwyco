
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
// $Header: g:/dwight/repo/cdc32/rcs/qdirth.h 1.9 1999/01/10 16:11:00 dwight Checkpoint $
#ifndef QDIRTH_H
#define QDIRTH_H
#include <time.h>
#include "vc.h"
#include "pval.h"


struct QckMsg
{
    vc v;
    QckMsg() : v(VC_VECTOR) {}

    vc& operator[](int a) {
        return v[a];
    }

};

typedef void (*QDFUNCP)(vc, void *, vc, ValidPtr);

struct ReqType
{
    vc name;
    int serial;

    ReqType() {
        serial = 0;
    }
    ReqType(vc n, int s) {
        name = n;
        serial = s;
    }

    int operator==(const ReqType& rt) const {
        if(name.is_nil())
            return rt.serial == serial;
        else
            return name == rt.name && rt.serial == serial;
    }
    operator char *() {
        static char a[1000];
        sprintf(a, "%s %d", (const char *)name, serial);
        return a;
    }
    vc response_type() {
        vc v(VC_VECTOR);
        v.append(name);
        v.append(serial);
        return v;
    }
};

struct QckDone
{
    ReqType type;
    QDFUNCP callback;
    void *arg1;
    vc arg2;
    int permanent;
    ValidPtr vp;
    // this is used to mark callbacks with the channel
    // the response that would trigger it should come from.
    // this is so when the channel dies, we can clean up
    // callback associated with it (fire them with generic errors)
    int channel; // channel from which response should appear


    QckDone() {
        callback = 0;
        arg1 = 0;
        permanent = 0;
        channel = -1;
        timeout = -1;
    }
    QckDone(QDFUNCP cb, void *user_arg,
            vc user_arg2 = vcnil, ValidPtr v = ValidPtr(), vc t = vcnil, int s = 0, int p = 0, int chan = -1, int tmout = -1)
        : vp(v), type(t, s) {
        callback = cb;
        arg1 = user_arg;
        permanent = p;
        arg2 = user_arg2;
        channel = chan;
        if(tmout != -1)
            this->timeout = tmout + time(0);
        else
            this->timeout = -1;
    }

    // note: we don't check arg1 and arg2 because they are user supplied
    // and we don't know what "==" means for them in general.
    int operator==(const QckDone& m) const {
        return callback == m.callback &&
               type == m.type &&
               vp == m.vp;
    }

    void done(vc v) {
        if(callback) (*callback)(v, arg1, arg2, vp);
    }
    int expired() {
        if(timeout == -1)
            return 0;
        else
            return time(0) > timeout;
    }
    void set_timeout(time_t tm) {
        timeout = time(0) + tm;
    }
private:
    time_t timeout;
};

int dirth_poll_response();
void dirth_poll_timeouts();

//void dirth_send_new3(vc id, vc handle, vc email, vc icq, vc first, vc last, vc info,
//	vc pw, vc rating, vc force_rating, vc pal_auth, QckDone);
void dirth_send_new4(vc id, vc handle, vc email, vc user_spec_id, vc pw, vc pal_auth, QckDone d);

void dirth_send_ack_get(vc id, vc mid, QckDone d);
void dirth_send_query(vc id, QckDone d);
void dirth_send_store(vc id, vc recipients, vc msg, QckDone d);
void dirth_send_get(vc id, vc which, QckDone d);
void dirth_send_ignore(vc id, vc uid, QckDone d);
void dirth_send_unignore(vc id, vc uid, QckDone d);
void dirth_send_get_ignore(vc id, QckDone d);
void dirth_send_ignore_count(vc id, vc uid, vc delta, QckDone d);
void dirth_cancel_callbacks(QDFUNCP f, void *arg1, const ReqType& type);
void dirth_cancel_callbacks(QDFUNCP f, ValidPtr vp, const ReqType& type);
int dirth_pending_callbacks(QDFUNCP f, void *arg1, const ReqType& type, vc arg2 = vcnil);
void dirth_simulate_error_response(const QckDone& q);
void dirth_dead_channel_cleanup(int chan);
void dirth_send_ack_all(vc id, QckDone d);
void dirth_send_clear_ignore(vc id, QckDone d);
void dirth_q_local_action(vc response_vector, QckDone d);
void dirth_send_misc_info(vc id, vc mi, QckDone d);
void dirth_send_get_server_list2(vc id, QckDone d);
void dirth_send_setup_session_key(vc id, vc dh_public, QckDone d);
void dirth_send_set_interest_list(vc id, vc list, QckDone d);
void dirth_send_recommend2(vc id, vc from, vc to, QckDone d);
void dirth_send_set_state(vc id, vc state, QckDone d);
void dirth_send_check_for_update(vc id, QckDone d);
//void dirth_send_set_profile(vc id, vc info, QckDone d);
//void dirth_send_get_profile(vc id, vc uid, vc cache_check, QckDone d);
void dirth_send_get_info(vc id, vc uid, vc cache_check, QckDone d);
void dirth_send_set_info(vc id, vc info, QckDone d);
void dirth_send_get_pal_auth_state(vc id, vc who, QckDone d);
void dirth_send_set_pal_auth_state(vc id, vc state, QckDone d);
void dirth_send_server_assist(vc id, vc to_uid, QckDone d);
void dirth_send_create_user_lobby(vc id, vc dispname, vc category, vc sub_god_uid, vc pw, vc user_limit, QckDone d);
void dirth_send_remove_user_lobby(vc id, vc lobby_id, QckDone d);
void dirth_send_get_pk(vc id, vc uid, QckDone d);
void dirth_send_debug(vc id, vc crashed, vc stack, vc field_track, QckDone d);
void dirth_send_set_token(vc id, vc token, QckDone d);
// used internally
QckMsg dirth_get_setup_session_key_cmd(vc id, vc sf_material, QckDone& d);

#define QTYPE 0
#define QFROM 1
#define QHANDLE 2
#define QEMAIL 3
#define QUSER_SPECED_ID 4
#define QFIRST 5
#define QLAST 6
#define QMESSAGES 7
#define QUSER_INFO 8
#define QPW  9
#define QPLAINTEXT 10
#define QRATING 11
#define QDHPUBLIC 12
#define QFORCE_RATING 13
#define QSTUFF 14
#define QRECIPIENTS 2
//#define QID 1
//#define QID_LIST 2
#define QMSG 4
#define QPAL_AUTH 15
#define QIGNORE_LIST 16
#define QSTATIC_PUBLIC 17

// vector positions for info response
#define QIR_FROM 0
#define QIR_HANDLE 1
#define QIR_EMAIL 2
#define QIR_USER_SPECED_ID 3
#define QIR_FIRST 4
#define QIR_LAST 5
#define QIR_DESCRIPTION 6
#define QIR_LOCATION 7

#endif
