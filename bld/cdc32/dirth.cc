
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#undef LOCAL_TEST
/*
 * $Header: g:/dwight/repo/cdc32/rcs/dirth.cc 1.22 1999/01/10 16:09:30 dwight Checkpoint $
 */
#include <string.h>
#include "doinit.h"
#include "usercnfg.h"
#include "cllaccpt.h"
#include "vidinput.h"
#include "dirth.h"
#include "vc.h"
#include "vcmap.h"
#include "lhboot.h"
#include "qdirth.h"
#include "qauth.h"
#include "qmsg.h"
#include "cdcver.h"
#include "zapadv.h"
#include "mmchan.h"
#include "senc.h"
#include "dwstr.h"
#ifndef LINUX
//#include "tbuy.h"
#endif
#include "gvchild.h"
#include "chatops.h"
#include "chatgrid.h"
#include "asshole.h"
#include "servass.h"
#include "aconn.h"
#include "fnmod.h"
#include "chatops.h"
#include "pgdll.h"
#include "xinfo.h"
#include "vcudh.h"
#include "dhsetup.h"
#include "prfcache.h"
#include "se.h"
#include "backsql.h"

//#undef NO_RTLOG
#include "dwrtlog.h"
#ifdef LINUX
#include <sys/utsname.h>
#include <arpa/inet.h>
#endif
#include "vcxstrm.h"
#include "ta.h"


int Inhibit_database_thread;

int Database_id = -1;
int Database_online;
int Database_starting;

extern DwycoEmergencyCallback dwyco_emergency_callback;
vc Server_list;
vc Database_list;
vc Xfer_list;
vc My_server_ip;
vc My_server_port;
vc My_server_name;
vc Pal_server_list;
vc STUN_server_list;
vc BW_server_list;
extern vc STUN_server;

vc KKG; // god mode pw

static int Inhibit_dir;

extern vc DH_public;
extern int No_database;
extern vc My_connection;

void exit_conf_mode();
vc set_get_uniq(int&);

#ifdef LEAK_CLEANUP
void
exit_dirth()
{
    Server_list = vcnil;
    Database_list = vcnil;
    Xfer_list = vcnil;
    My_server_ip = vcnil;
    My_server_port = vcnil;
    My_server_name = vcnil;
    Pal_server_list = vcnil;
}
#endif

vc
dwyco_get_version_string()
{
    extern vc Client_version;
    DwString a(IVERSION);
    a += "!";
    a += (const char *)Client_version;
    return a.c_str();
}

static void
got_sync(vc, void *, vc, ValidPtr)
{
    query_messages();
}

static void
got_inhibit(vc, void *, vc, ValidPtr)
{
    //end_directory_thread();
    stop_chat_thread();
    Inhibit_dir = 1;
}

static void
got_purge_outbox(vc, void *, vc, ValidPtr)
{
    qd_purge_outbox();
}

static void
reset_backups(vc, void *, vc, ValidPtr)
{
    dwyco::reset_msg_backup();
}

void
update_server_list(vc m, void *, vc, ValidPtr)
{
    // note: if we get this far, even if it is a failure,
    // we may still be able to get to a proper database
    // server using whatever server list exists currently
    // on the disk. so we give that a try right away.


    if(m[1].is_nil())
    {
        if(Database_id == -1)
            start_database_thread();
        return;
    }
    if(m[1].type() != VC_VECTOR)
    {
        // something is wrong, just go back to the
        // built-in defaults just in case
        DeleteFile(newfn("servers2").c_str());
        if(Database_id == -1)
            start_database_thread();
        return;
    }
    save_info(m[1], "servers2");
    // if nothing other than IP's have changed, then
    // incorporate the change directly rather than
    // having to wait until the next restart
    vc nsl = m[1][0];

    for(int i = 0; i < Server_list.num_elems(); ++i)
    {
        int j;
        vc d = Server_list[i];
        for(j = 0; j < nsl.num_elems(); ++j)
        {
            if(d[SL_SERVER_NAME] == nsl[j][SL_SERVER_NAME] &&
                    d[SL_SERVER_RATING] == nsl[j][SL_SERVER_RATING])
            {
                Server_list[i] = nsl[j];
                break;
            }
        }
    }

    // note: probably want to do the same thing for
    // the database list and xfer list

    vc ndbl = m[1][1];
    vc nxfer = m[1][2];
    vc npal = m[1][3];
    vc nstun = m[1][4];
    vc nbw = m[1][5];

    // check the database list, if only IP's have changed, then
    // update to use the new database list. note that we should
    // terminate all database connections if any ip has changed.
    // if any of the other mapping data in the database list has
    // changed, then we are in a pickle. instead of using it,
    // we just save it, and defer using it until next startup.
    // the main reason for this is that lots of stuff depends on
    // what server is your "home" server, and it is not designed to
    // change in mid stream.
    //
    // the xfer list, on the other hand, can be updated immediately
    // without any problems.
    // note: kluge, we quit and restart if any of the
    // database server info has changed...

    int terminate_servers = 0;
    int exit_advisory = 0;
    if(ndbl.num_elems() == Database_list.num_elems())
    {
        int n = ndbl.num_elems();
        int i;
        for(i = 0; i < n; ++i)
        {
            vc nrec = ndbl[i];
            vc orec = Database_list[i];
            if(nrec[0] != orec[0] || nrec[1] != orec[1] ||
                    nrec[2] != orec[2] || nrec[3] != orec[3])
                break;
            if(nrec[4] != orec[4] || nrec[5] != orec[5] || nrec[6] != orec[6])
            {
                terminate_servers = 1;
                exit_advisory = 1;
                Database_list[i] = ndbl[i];
            }
        }
        if(i != n)
        {
            terminate_servers = 1;
            exit_advisory = 1;
        }
    }
    else
    {
        exit_advisory = 1;
        terminate_servers = 1;
    }

    if(terminate_servers)
    {
        MMChannel *mc;
        while(mc = MMChannel::get_server_channel())
            mc->schedule_destroy(MMChannel::HARD);
        while(mc = MMChannel::get_secondary_server_channel())
            mc->schedule_destroy(MMChannel::HARD);
    }

    if(exit_advisory)
    {
        // DO SOMETHING HERE

        if(dwyco_emergency_callback)
        {
            (*dwyco_emergency_callback)(DWYCO_EMERGENCY_DB_CHANGE, 1, "The messages servers have changed location, you must quit and restart now.");
        }
        else
        {
#ifdef _Windows
            ::MessageBox(0, "db servers changed location, must quit now.", "quit", MB_OK);
#endif
            dwyco_exit();
            exit(0);
        }
    }

    Xfer_list = nxfer;
    Pal_server_list = npal;
    STUN_server_list = nstun;
    BW_server_list = nbw;


    if(Database_id == -1)
        start_database_thread();
}

vc
get_server_by_uid(vc uid)
{
    GRTLOG("server by uid", 0, 0);
    GRTLOGVC(to_hex(uid));
    GRTLOGVC(Database_list);
    if(Database_list.is_nil())
        return vcnil;
    for(int i = 0; i < Database_list.num_elems(); ++i)
    {
        int start = Database_list[i][0];
        int len = Database_list[i][1];
        int mask = Database_list[i][2];
        int val = Database_list[i][3];

        if(start >= uid.len())
            continue;
        if(start + len > uid.len())
            continue;

        unsigned int uv = 0;
        for(int j = start; j < start + len; ++j)
        {
            uv <<= 8;
            uv |= (unsigned char)((const char *)uid)[j];
        }
        uv &= mask;
        if(uv == val)
            return Database_list[i];
    }
    return vcnil;
}

vc
get_server_ip_by_uid(vc uid, vc& port)
{
    vc v = get_server_by_uid(uid);
    if(v.is_nil())
        return vcnil;
    port = v[6];
    return v[5];
}

vc
get_server_name_by_uid(vc uid, vc& port)
{
    vc v = get_server_by_uid(uid);
    if(v.is_nil())
        return vcnil;
    port = v[6];
    return v[4];
}

vc
get_random_xfer_server_ip(vc& port)
{
    int n = Xfer_list.num_elems();
    if(n <= 0)
        return vcnil;
    int p = abs(gen_random_int());
    if(p <= 0)
        p = 0;
    GRTLOG("XFER to", 0, 0);
    GRTLOGVC(Xfer_list[p%n]);
    port = Xfer_list[p % n][2];
    return Xfer_list[p % n][1];
}

// server_list should be a vector of vectors of the form:
// vector(hostname ip port)
vc
get_random_server_ip(vc server_list, vc& port)
{
    if(server_list.type() != VC_VECTOR)
        return vcnil;
    int n = server_list.num_elems();
    if(n <= 0)
        return vcnil;
    int p = abs(gen_random_int());
    if(p <= 0)
        p = 0;
    GRTLOG("RNDSERV to", 0, 0);
    GRTLOGVC(server_list[p%n]);
    if(server_list[p%n].type() != VC_VECTOR)
        return vcnil;
    port = server_list[p % n][2];
    return server_list[p % n][1];
}

static
void
invalidate_profile(vc m, void *, vc, ValidPtr)
{
    if(m[1].type() != VC_STRING)
        return;
    GRTLOG("msgb invalidate profile %s", (const char *)to_hex(m[1]), 0);
    prf_invalidate(m[1]);
    se_emit(SE_USER_PROFILE_INVALIDATE, m[1]);

}
extern DwVec<QckDone> Waitq;
void update_server_list(vc, void *, vc, ValidPtr);
void ignoring_you_update(vc, void *, vc, ValidPtr);
void background_check_for_update_done(vc m, void *, vc, ValidPtr p);
void async_pal(vc, void *, vc, ValidPtr);

void
init_dirth()
{
    Waitq.append(QckDone(got_sync, 0, vcnil, ValidPtr(0), "sync", 0, 1));
    Waitq.append(QckDone(got_serv_r, 0, vcnil, ValidPtr(0), "serv_r", 0, 1));
    Waitq.append(QckDone(got_inhibit, 0, vcnil, ValidPtr(0), "inhibit", 0, 1));
    Waitq.append(QckDone(got_purge_outbox, 0, vcnil, ValidPtr(0), "purge_outbox", 0, 1));
    //void got_mailbox_full(vc m, void *, vc, ValidPtr);
    //Waitq.append(QckDone(got_mailbox_full, 0, vcnil, ValidPtr(0), "mailbox_full", 0, 1));
    Waitq.append(QckDone(update_server_list, 0, vcnil, ValidPtr(0), "nsl", 0, 1));
    Waitq.append(QckDone(ignoring_you_update, 0, vcnil, ValidPtr(0), "iy", 0, 1));
    Waitq.append(QckDone(background_check_for_update_done, 0, vcnil, ValidPtr(0), "serv-check-update", 0, 1));

#ifndef DWYCO_UDP_PAL
    Waitq.append(QckDone(async_pal, 0, vcnil, ValidPtr(0), "async-pal", 0, 1));
#endif
    Waitq.append(QckDone(invalidate_profile, 0, vcnil, ValidPtr(0), "invalidate-profile", 0, 1));
    Waitq.append(QckDone(reset_backups, 0, vcnil, ValidPtr(0), "reset-backups", 0, 1));
    //get the server list set up
    if(!load_info(Server_list, "servers2") || Server_list.type() != VC_VECTOR ||
            Server_list.num_elems() < 1)
    {
        vc v("090160901409017020212s1.dwyco.org020215173.255.230.19201054350002011a02015Adult02015Adult09013020217s6.blindchimp.com02021366.135.42.14701054350009017020212s1.dwyco.org020215173.255.230.19201054350102011a02017Couples02015Adult09013020217s6.blindchimp.com02021366.135.42.14701054350109017020212s2.dwyco.org02021250.116.26.1201054350202011a020210Clean Chat02015Adult09013020212s5.dwyco.com02021469.164.210.23701054350209017020212s2.dwyco.org02021250.116.26.1201054350302011a02013Gay02015Adult09013020217s6.blindchimp.com02021366.135.42.147010543503090140901701010010110101301010020212s1.dwyco.org020215173.255.230.1920105405000901701010010110101301011020212s1.dwyco.org020215173.255.230.1920105405010901701010010110101301012020212s2.dwyco.org02021250.116.26.120105405020901701010010110101301013020212s2.dwyco.org02021250.116.26.120105405030901209013020212s1.dwyco.org020215173.255.230.1920104690209013020212s2.dwyco.org02021250.116.26.12010469020901109013020212s1.dwyco.org020215173.255.230.1920105100960901109013020212s5.dwyco.com02021469.164.210.237010434340901209013020212s1.dwyco.org020215173.255.230.1920104343709013020212s2.dwyco.org02021250.116.26.1201043437");
        vcxstream vcx((const char *)v, v.len(), vcxstream::FIXED);

        vc item;
        long len;
        if(!vcx.open(vcxstream::READABLE))
        {
            oopanic("servers2 file is corrupt, contact tech support.");
        }
        if((len = item.xfer_in(vcx)) < 0)
        {
            oopanic("servers2 file is corrupt, contact tech support.");
        }
        Server_list = item;
    }
    // in this version, the server list
    // has 3 lists in it: a directory list (what the
    // old Server list was) a database list, and
    // an xfer-server list.
    Database_list = Server_list[1];
    Xfer_list = Server_list[2];
    Pal_server_list = Server_list[3];
    STUN_server_list = Server_list[4];
    BW_server_list = Server_list[5];
    vc tmp = Server_list[0];
    Server_list = tmp;
}

// only call this *after* the server list is
// initialized *and* the account id has been
// determined.

void
init_home_server()
{
    vc port;
    My_server_ip = get_server_ip_by_uid(My_UID, port);
    My_server_name = get_server_name_by_uid(My_UID, port);
    My_server_port = port;
}

long
get_disk_serial()
{
#ifdef _Windows
    DWORD serial = 0;
    DWORD dum;
    GetVolumeInformation("c:\\", 0, 0, &serial, &dum, &dum, 0, 0);
    return serial;
#endif
    return 0;
}

vc
system_info()
{
    vc v(VC_VECTOR);

#ifdef _Windows
    extern RECT ScreenSize;
    vc screen(VC_VECTOR);
    screen.append(ScreenSize.left);
    screen.append(ScreenSize.top);
    screen.append(ScreenSize.right);
    screen.append(ScreenSize.bottom);

    OSVERSIONINFO o;
    memset(&o, 0, sizeof(0));
    o.dwOSVersionInfoSize = sizeof(o);
    GetVersionEx(&o);

    vc osv(VC_VECTOR);
    osv.append((long)o.dwMajorVersion);
    osv.append((long)o.dwMinorVersion);
    osv.append((long)o.dwBuildNumber);
    osv.append((long)o.dwPlatformId);
    osv.append(o.szCSDVersion);

    v.append(screen);
    v.append(osv);

    v.append((int)get_disk_serial());
#endif
#ifdef LINUX
    vc screen(VC_VECTOR);
    screen.append(0);
    screen.append(0);
    screen.append(0);
    screen.append(0);

    struct utsname buf;
    uname(&buf);

    vc osv(VC_VECTOR);
    osv.append(buf.version);
    osv.append(buf.release);
    osv.append(buf.sysname);
    osv.append(buf.machine);
    osv.append(buf.nodename);

    v.append(screen);
    v.append(osv);

    v.append((int)get_disk_serial());
#endif
    v.append(""); // was user-entered regcode

    extern int Crashed_last_time;
    v.append(Crashed_last_time);

//	v.append(StackDump);
//	StackDump = vcnil;

//	v.append(Transmit_stats);
//	Transmit_stats = vcnil;
    // lets not piggy back debug reporting here anymore
    v.append(vcnil);
    v.append(vcnil);

    static vc MID;
    static int MID_method = -2;

    if(MID.is_nil())
        MID = set_get_uniq(MID_method);
    if(MID.is_nil())
        MID = vc(VC_BSTRING, "\x45\x72\x89\x00\x3a\x98\x50\x3b\xf8\x39", 10);
    My_MID = MID;
    v.append(MID);
    v.append(My_connection);
    v.append(MID_method);

    return v;


}

vc
make_fw_setup()
{
    vc v(VC_VECTOR);
    if(DwNetConfigData.get_advertise_nat_ports())
    {
        v.append(DwNetConfigData.get_nat_primary_port());
        v.append(DwNetConfigData.get_nat_secondary_port());
        v.append(DwNetConfigData.get_nat_pal_port());
    }
    else
    {
        v.append(DwNetConfigData.get_primary_port());
        v.append(DwNetConfigData.get_secondary_port());
        v.append(DwNetConfigData.get_pal_port());
    }
    return v;
}

vc
make_local_ports()
{
    vc v(VC_VECTOR);
    v.append(DwNetConfigData.get_primary_port());
    v.append(DwNetConfigData.get_secondary_port());
    v.append(DwNetConfigData.get_pal_port());
    return v;
}

vc
build_directory_entry()
{
    vc v(VC_VECTOR);
    v.append(Myhostname);
    v.append(UserConfigData.get_username());
    v.append(UserConfigData.get_description());
    CallAcceptanceXfer& c = CallAcceptanceData;
    vc v2(VC_VECTOR);
    v2.append(c.get_max_audio());
    v2.append(VidInputData.get_no_video() ? 0 : c.get_max_video());
    v2.append(c.get_max_chat());
    v2.append(c.get_max_audio_recv());
    v2.append(c.get_max_video_recv());
    v.append(v2);
    v.append(1); // "registered"
    v.append(dwyco_get_version_string());
    v.append(UserConfigData.get_email());
    v.append(My_UID);
    v.append(DH_public);
    v.append(vcnil); // was "rating"
    v.append(system_info());
    v.append(ZapAdvData.get_always_server() ? vcnil : vctrue); // can do direct msgs
    // note: no more invisible
    v.append(vcnil);
    //v.append(ShowDirectoryData.get_invisible() ? vctrue : vcnil);

    // really the location
// XXXXXX note: this needs to be FIXED, it is always mucked up
// after an integrate.
// 13
    v.append(UserConfigData.get_location());
    v.append(vcnil);
    // note: to support old crappy clients, this element
    // must be nil (this is ui-speced-peer in server, and the
    // server filled it in.
    v.append(vcnil); // ui-speced-peer
    v.append(make_fw_setup());
    extern vc Auto_update_hash;
    v.append(Auto_update_hash); //17
#if 0
    extern vc Never_visible; //18
    if(!Never_visible.is_nil() && Never_visible.num_elems() > 0)
        v.append(Never_visible);
    else
        v.append(vcnil);
#else
    v.append(vcnil);
#endif

    v.append(KKG);
    v.append(get_asshole_factor());

    GRTLOG("dir entry", 0, 0);
    GRTLOGVC(v);
    return v;
}

vc
build_directory_entry2()
{
    vc v(VC_VECTOR);
    v.append(Myhostname);
    v.append(1); // was "registered"
    v.append(dwyco_get_version_string());
    v.append(system_info());
    v.append(ZapAdvData.get_always_server() ? vcnil : vctrue);
    v.append(make_fw_setup());

    v.append(KKG);

    GRTLOG("dir entry2", 0, 0);
    GRTLOGVC(v);
    return v;
}


extern int Reauthorize;

static void
db_offline(MMChannel *, vc, void *, ValidPtr)
{
    Database_id = -1;
    Database_online = 0;
    Reauthorize = 1;
    GRTLOG("db offline", 0, 0);
    TRACK_ADD(DB_offline, 1);
}

static void
start_encryption(vc m, void *, vc , ValidPtr vp)
{
    MMChannel *mc = vp.is_valid() ? (MMChannel *)(void *)vp : 0;
    Database_online = 1;
    Database_starting = 0;
    if(m[1].is_nil())
    {
        if(mc)
            mc->agreed_key = vcnil;
        TRACK_ADD(DB_crypto_fail, 1);
        return;
    }
    mc->send_decrypt();
    mc->tube->set_key_iv(mc->agreed_key, 0);
    vc key = mc->agreed_key;
    // truncating to 80 bits is a compat hack
    if(mc->agreed_key.len() > 10)
        key = vc(VC_BSTRING, (const char *)mc->agreed_key, 10);
    Current_session_key = key;
    TRACK_ADD(DB_crypto_up, 1);
    if(Reauthorize)
    {
        GRTLOG("other public reauthorize", 0, 0);
        TRACK_ADD(DB_crypto_up_reinit, 1);
        init_qauth();
        Reauthorize = 0;
    }
}

void
db_online(MMChannel *mc, vc, void *, ValidPtr)
{
    mc->destroy_callback = db_offline;
    vc material = dh_store_and_forward_material(Server_publics, mc->agreed_key);
    dirth_send_setup_session_key(My_UID, material, QckDone(start_encryption, 0, vcnil, mc->vp));

    GRTLOG("db up", 0, 0);
    TRACK_ADD(DB_up, 1);
}

void
db_call_failed_last(MMChannel *mc, vc, void *, ValidPtr)
{
    Database_id = -1;
    Database_starting = 0;
    GRTLOG("db call failed last", 0, 0);
    TRACK_ADD(DB_hard_fail, 1);
}

void
db_call_failed(MMChannel *mc, vc, void *, ValidPtr)
{
    Database_id = -1;
    //if(mc->resolve_failed)
    {
        MMChannel *mc = MMChannel::start_server_channel(
                            MMChannel::BYNAME,
                            0,
#if 0
                            inet_addr("204.75.209.40"),
                            0,
                            6703);
#endif

        My_server_name,
        My_server_port);

        if(!mc)
    {
        Database_id = -1;
        return;
    }
    mc->established_callback = db_online;
    mc->destroy_callback = db_call_failed_last;
    mc->set_string_id("Dwyco Database");
    Database_id = mc->myid;
    Database_online = 0;
    //GRTLOG("db resolve failed", 0, 0);
    //return;
}
GRTLOG("db call failed", 0, 0);
}


void
start_database_thread()
{
    if(Inhibit_database_thread)
        return;
#if 0 && defined(CDC32_REGISTERED)
    if(!reg_is_registered())
        return;
#endif
    if(Database_id != -1)
        return;
    MMChannel *mc = MMChannel::start_server_channel(
                        MMChannel::BYADDR,
                        inet_addr(My_server_ip),
                        0,
                        My_server_port);
    if(!mc)
    {
        // maybe DNS hosed right off the bat, so
        // go straight to next stage
        Database_id = -1;
        db_call_failed(0, vcnil, 0, ValidPtr());
        return;
    }
    mc->established_callback = db_online;
    mc->destroy_callback = db_call_failed;
    mc->set_string_id("Dwyco Database");
    Database_id = mc->myid;
    Database_online = 0;
    Database_starting = 1;
    GRTLOG("db start", 0, 0);
    GRTLOGVC(My_server_name);
    GRTLOGVC(My_server_port);
    return;
}

void
end_database_thread()
{
    MMChannel *mm = MMChannel::channel_by_id(Database_id);
    if(mm)
    {
        mm->destroy_callback = 0;
        mm->established_callback = 0;
        MMChannel::synchronous_destroy(Database_id);
    }
    Database_id = -1;
    Database_online = 0;
    Reauthorize = 1;
    GRTLOG("db offline", 0, 0);

}

int
dirth_switch_to_chat_server(int n, const char *pw, StatusCallback scb)
{
    if(n < 0 || n >= Server_list.num_elems())
        return 0;
    stop_chat_thread();
    vc d = Server_list[n];
    GRTLOG("switch to chat", 0, 0);
    GRTLOGVC(d);

    vc ip = d[SL_SERVER_IP];
    vc port = (int)d[SL_SERVER_PORT] + 1000;

    if(!start_chat_thread2(ip, port, pw, scb))
        return 0;
    return 1;
}

