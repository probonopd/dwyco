
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
// server assisted calling
// this exists solely because firewalls/NAT boxes are too
// complicated for consumers to figure out.
// manufacturers don't want to fix it, and uPNP doesn't
// work well enough in existing routers at this point.

#include "mmchan.h"
#include "qauth.h"
#include "dwrtlog.h"
#include "dwstr.h"
#include "servass.h"
#include "netvid.h"
#include "doinit.h"
#include "qdirth.h"
#include "gvchild.h"
#include "msgdisp.h"
#include "calllive.h"
#include "ta.h"

int Disable_SAC = 0;

static void
serv_recv_call_failed_last(MMChannel *mc, vc, void *, ValidPtr)
{
    GRTLOG("serv_recv call failed last", 0, 0);
}

void
serv_recv_online(MMChannel *mc, vc prox_info, void *, ValidPtr mcv)
{
    if(!mcv.is_valid())
        return;
    MMChannel *parent_mc = (MMChannel *)(void *)mcv;
    if(parent_mc == 0)
    {
        // special case, creating control channel
        mcv.invalidate();

        GRTLOG("serv_recv up", 0, 0);
        // here is where we do the switch-o-change-o
        // to allow the socket we just connected to the
        // proxy to become a channel/tube combo that
        // looks as if it were created by a normal
        // accept on the Listen_sock

        // build a channel
        // rest of channel is built after the
        // remote configuration is received
        // and verified.
        MMChannel *chan = new MMChannel;
        chan->proxy_info = prox_info; // for further proxy-based setups
        chan->tube = mc->tube;
        // this doesn't make sense in this case because
        // we are presumably blocked from accepting direct
        // connections anyways
        //chan->tube->init_listener();
        mc->tube = 0;
        chan->wait_for_crypto();
        chan->start_service();
        GRTLOG("new proxy %s", mc->get_string_id().c_str(), 0);
        // we may end up waiting awhile if the accept/rej box gets popped up,
        // so wait till later to get the STUN sockets set up.
        TRACK_ADD(CLR_control_established, 1);

        mc->schedule_destroy(MMChannel::HARD);
    }
    else
    {
        GRTLOG("serv_recv_aux up", 0, 0);
#if 0
        // creating a subchannel, so special switch-o-change-o for that
        // man this is NASTY
        MMTube *tube = parent_mc->tube;
        if(!tube)
            oopanic("no tube?");
        int c = tube->find_chan();
        tube->set_channel(mc->tube->ctrl_sock, 0, 0, c);
        //parent_mc->protos[c] = MMChannel::CHAN_GET_WHAT_TO_DO;
        mc->tube->ctrl_sock = 0;
        sproto *s = new sproto(c, recv_command, parent_mc->vp);
        parent_mc->simple_protos[c] = s;
        s->start();
#endif
        // this setup just transfers the ctrl_sock which was
        // set up into a subchannel on the same mmchan, and
        // sets up a protocol object to get the first commands
        // received. this makes it more like the firewall
        // friendly case so we can eliminate the code related
        // to the old non-firewall friendly case.
        int c = mc->tube->find_chan();
        mc->tube->set_channel(mc->tube->ctrl_sock, 0, 0, c);
        mc->tube->ctrl_sock = 0;
        sproto *s = new sproto(c, recv_command, mc->vp);
        mc->simple_protos[c] = s;
        s->start();

        TRACK_ADD(CLR_subchan_established, 1);
    }
    // DO NOT DO THIS, as we might have got here via a callback
    // and it might obliterate the context we are in now.
    //MMChannel::synchronous_destroy(mc->myid, MMChannel::HARD);

}

// this starts a connection to what is assumed to be
// some kinda proxy. we are ultimately to act as
// if we were the callee after the connection is completed.
static
void
track_connect(MMChannel *, vc what, void *, ValidPtr)
{
    if(what.is_nil())
    {
        TRACK_ADD(CLR_connect_failed, 1);
    }
    else
    {
        TRACK_ADD(CLR_connect_succeeded, 1);
    }
}

void
start_serv_recv_thread(vc ip, vc port, ValidPtr mcv)
{

    MMChannel *mc = MMChannel::start_server_channel(
                        MMChannel::BYADDR,
                        inet_addr(ip),
                        0,
                        port);
    if(!mc)
    {
        serv_recv_call_failed_last(0, vcnil, 0, ValidPtr());
        return;
    }
    mc->established_callback = serv_recv_online;
    vc prox_info(VC_VECTOR);
    prox_info[0] = ip;
    prox_info[1] = port;
    mc->ecb_arg1 = prox_info;
    mc->ecb_arg3 = mcv;
    mc->destroy_callback = serv_recv_call_failed_last;
    mc->low_level_connect_callback = track_connect;
    mc->set_string_id("Dwyco Server Assisted Call");

    GRTLOG("serv_recv start", 0, 0);
    GRTLOGVC(ip);
    GRTLOGVC(port);
    return;
}


// this is called in the callee when they receive the initial
// serv_r async msg from the msg server, this is the initial
// control channel setup

void
got_serv_r(vc m, void *, vc, ValidPtr)
{
    if(Disable_SAC)
        return;
    if(m[1].is_nil())
        return;
    vc ip = m[1][2];
    vc port = m[1][3];

    // note: might be better if we set up the proxy_info to be
    // the same in both caller and callee. in this case, the
    // callee is only going to have a vector(ip port) instead of
    // vector(ip-caller port-caller ip-callee port-callee)
    start_serv_recv_thread(ip, port, ValidPtr(0));
    TRACK_ADD(CL_got_serv_r, 1);
}

static void
servass_results(vc m, void *f, vc v, ValidPtr vp)
{
    vc to_uid = v[0];
    int media_select = (int)v[1];
    //vc user_args = v[2];
    MessageDisplay *md = (MessageDisplay *)f;
    if(m[1].is_nil())
    {
        stun_servass_failure("User not online.", to_uid, vp);
        if(md)
            md->show("User not online. Call failed.");
        TRACK_ADD(CL_server_assist_failed, 1);	;
        return;
    }
    // return is vector(prox-ip prox-port callee-prox-ip callee-prox-port)
#if 0
    vc hostlist(VC_VECTOR);
    vc portlist(VC_VECTOR);
    vc uid_list(VC_VECTOR);
    vc proxlist(VC_VECTOR);

    hostlist.append(m[1][0]);
    portlist.append(m[1][1]);
    uid_list.append(to_uid);
    proxlist.append(m[1]);
#endif
    stun_connect(m[1][0], m[1][1], m[1], to_uid, media_select, vp, md);
    //else
    //	conference_connect(hostlist, portlist, proxlist, uid_list);
}

void
start_server_assisted_call(vc uid, int media_select, ValidPtr vp, MessageDisplay *md)
{
    vc v(VC_VECTOR);
    v[0] = uid;
    v[1] = media_select;
    dirth_send_server_assist(My_UID, uid, QckDone(servass_results, md, v, vp));
    TRACK_ADD(CL_server_assist_request, 1);
}

// this is called in the callee when they recv the "aux_r" control msg
void
aux_channel_setup(MMChannel *mc, vc v)
{

    if(Disable_SAC)
        return;
    // this is where we start an aux setup by calling out to the
    // same proxy that set up the control channel
    // this is called on the callee's client in response to another
    // setup request from the caller.
    start_serv_recv_thread(mc->proxy_info[0], mc->proxy_info[1], mc->vp);

}

