
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include "vc.h"
#ifndef DWYCO_NO_UVSOCK
#ifndef VCUVSOCK_H
#define VCUVSOCK_H

#include "vcnil.h"
#include "vccomp.h"
#include "dwvp.h"
#include "uv.h"
#include "vcxstrm.h"
#include "dwstr.h"
#include "dwtree2.h"
#include "vcio.h"

class vc_uvsocket;
#define Ready_q (*Ready_q_p)

typedef DwTreeKazIter<vc_uvsocket *, long> UV_READY_Q_ITER;
class vc_uvsocket : public vc_composite
{
private:

    // note: used to be private inh, but that cause some problems
    // elsewhere. this isn't the best idea, just a hackaround so i
    // didn't have to do a bunch of forwarding by hand.
    struct list2 : public DwListA<vc>
    {
        vc_uvsocket *container;
        list2(vc_uvsocket *s) : container(s) {}

        void append(const vc& v) {
            if(num_elems() == 0)
            {
                vc_uvsocket::Ready_q_p->add(container->vp.cookie, container);
            }
            DwListA<vc>::append(v);

        }

        int remove_first() {
            if(num_elems() == 1)
            {
                vc_uvsocket::Ready_q_p->del(container->vp.cookie);
            }
            return DwListA<vc>::remove_first();
        }
        void clear() {
            if(num_elems() >= 1)
            {
                vc_uvsocket::Ready_q_p->del(container->vp.cookie);
            }
            DwListA<vc>::clear();
        }
        //DwListA<vc>::num_elems;
       // DwListA<vc>::get_first;
    };

protected:

public:
	DwVP vp;
	vc_uvsocket();
	~vc_uvsocket();

    int syntax;

    // for use by length-encoding syntax
    int state;
    int32_t len_to_read;

    int set_syntax(int);
    int parse_buffer(int once);

	static uv_loop_t *uvs_loop;
	static int init_uvs_loop();
	static int run_loop_once();
	// this monkey-biz with the ready-q and pointers is to avoid
	// problems related to destructors being called at exit time.
    static DwTreeKaz<vc_uvsocket *, long> *Ready_q_p;
    void add_error(vc v);

	uv_tcp_t *tcp_handle;
    list2 getq;
    list2 putq;
	int listening;
    // xstream we use to reconstruct objects as they come in
    vcxstream readx;
    DwString tailstr;
    const char *tmpbuf;
    int tmplen;

	// this is needed since errors in the UV socket like
	// "connection reset by peer" obliterate the peer information.
	vc cached_peer;
	
    // note: it would be possible to have these different syntaxes
    // interleaved on one stream, but it would require either some kind
    // of "meta encapsulation", or some other declaration of when to
    // switch between the different syntaxes. the length-encoded
    // functions are mostly here for compatibility with older
    // versions of the networking stuff. so for now, you cannot
    // interleave different syntaxes, stick with either all object
    // or all length-encoded on a single stream.

    // these are for i/o with vc xfer rep syntax
	vc socket_get_obj(int& avail, vc& addr_info);
    vc socket_put_obj(vc obj, const vc& to_addr, int syntax);

	vc socket_init(const vc& local_addr, int listen, int reuse, int syntax = 0);
    vc socket_close(int close_info);
    // graceful write shutdown
    vc socket_shutdown(int how);
	vc socket_connect(const vc& addr);
    vc socket_peer_addr();
    vc socket_local_addr();
    int socket_get_write_q_size();
    int socket_get_read_q_len();

    long overflow(vcxstream&, char *buf, long len);
    long underflow(vcxstream&, char *buf, long min, long max);
	virtual vc_default *do_copy() const {oopanic("copy uvsocket?"); return 0;}
	enum vc_type type() const {return VC_SOCKET;}
	int is_quoted() const {return TRUE;}
    void printOn(VcIO os);
	void stringrep(VcIO os) const {os << "uvsocket()";}
};

#endif
#endif

