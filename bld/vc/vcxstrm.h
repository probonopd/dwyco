
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#ifndef VCXSTRM_H
#define VCXSTRM_H
//
// VC xfer stream
// This class provides a number of services to allow easy
// transfer of info between a client of this class and
// an underlying device.
// To use one of these streams, you create a stream object
// and specify the device to use. To xfer data to the device
// you call the in_want/out_want member functions. The
// stream calls the overflow and underflow member functions of the
// device object when physical I/O is necessary.
// There are a variety of styles of I/O & devices supported
// via both constructor args and args to specific I/O calls.
//
//
// $Header: g:/dwight/repo/vc/rcs/vcxstrm.h 1.49 1998/12/09 05:11:28 dwight Exp $

#include "dwvec.h"
#include "dwvecp.h"
#include "vc.h"
#include "dwgrows.h"
#include "vcdeflt.h"

class vcxstream;

typedef DwVecP<vc_default> ChitTable;

typedef long (*VCXUNDERFUN)(vcxstream&, char *, long min, long max);
typedef long (*VCXOVERFUN)(vcxstream&, char *, long howmuch);
typedef int VCXCHIT;

#define NB_ERROR 0
#define NB_RETRY -1
#define NB_DONE 1

class vcxstream
{
friend vc lh_bf_xfer_enc(vc);
friend vc vclh_bf_xfer_enc_ctx(vc, vc);
friend vc vclh_compress_xfer(vc, vc);
friend vc vclh_serialize(vc);
friend vc serialize(vc);
friend int deserialize(vc, vc&);
friend vc vclh_encdec_xfer_enc_ctx(vc, vc);
friend class vc_uvsocket;
private:
public:
    enum how_close {
        // for readable streams, any unread data in the buffer is discarded.
        // warning: you don't always have control over how much data is
        // in the buffer, so be careful about flushing readable streams.
        // for writable streams, issue whatever i/o operation is
        // necessary to write remaining data out.
        FLUSH,

        // for readable streams, do nothing.
        // for writable streams, make buffer empty without issuing final write.
        DISCARD,

        // only valid for atomic i/o-style (see below.)
        // on readables, pushes back any data that has been read since
        // the last open. leaves the stream in a state such that the pushed
        // back data will be returned by subsequent in_want calls, and then
		// i/o operations will continue if more data is available after
        // the old data is read.
        //
        // for writables,
        RETRY,

        // just reset the chit table, but don't fiddle with the buffers.
        // this should allow multiple open-close pairs on a single
        // stream connected to a device. inhibits the "devclose"
        // operation.
        CONTINUE
    };

    enum status {NOT_OPEN, OPEN, CLOSED, READABLE, WRITEABLE, RETRYING,
                WRITE_RETRY, READ_RETRY};

    enum style {
        // buffer is fixed length, and cannot be changed
        // for readable streams, an i/o operation is performed
		// at open time to try and fill the buffer immediately.
        FIXED,

        // buffer can be extended by up to req length
        CONTINUOUS,

        // buf can be extended by req length and more
        CONTINUOUS_READAHEAD
    };

    enum e_iostyle {
        // for reading, i/o ops are not logged (useful only for blocking devices)
        // for writing, i/o ops are done piecemeal as buffer overflows
        MULTIPLE,

        // for reading, i/o ops are logged, and retry is allowed (on close, data is
        //  reinserted into the stream to be re-read on next in_want)
        // for writing, i/o ops are inhibited and results accumulated in a buffer
        //  which is then written in one i/o op at close time
        ATOMIC
    };

	// use this when there is no device, just a fixed buffer
	// 
	//vcxstream(char *buf = 0, long len = 2048, enum style = FIXED);
	vcxstream(const char *buf = 0, long len = 2048, enum style = FIXED);
	
	// not working yet, but this can be used with a
	// disembodied device.
	vcxstream(VCXUNDERFUN underflow, VCXOVERFUN overflow, vc_default *vcp, 
		char *buf = 0, long len = 2048, enum style = CONTINUOUS);
	
	// use this one when you know the lifetime of the device
	// is short (you can pass a pointer to the device without the
	// need to encapsulate it...)
	vcxstream(vc_default *, char *buf = 0, long len = 2048, enum style = CONTINUOUS);
	
	// use this one one if the device and stream are long-lived, or
	// it isn't possible to get at the underlying pointer object.
	vcxstream(vc, char *buf = 0, long len = 2048, enum style = CONTINUOUS);
	
	~vcxstream();

    // open/close pairs is how the xstream object calculates
    // how to record references in structures that contain
    // object that are referenced more than once.
    // as streaming proceeds, the serializer can use the chit
    // functions to represent references to objects it has seen more
    // than once. closing a stream deletes the chit table.
	int open(enum status, enum e_iostyle = MULTIPLE);
	int close(enum how_close how = FLUSH);
    int open2(enum status, enum e_iostyle = MULTIPLE);
    int close2(enum how_close how);

    char *in_want(long);
	char *out_want(long);
    int retry();
	void put_back(const char *, long);
	int flush();
	int flushnb();
	enum status get_status();
	int has_input();

	vc_default *chit_get(VCXCHIT);
	void chit_append(vc_default *);
	VCXCHIT chit_find(vc_default *);
	void chit_new_table();
	void commit();
    int check_status(enum status);

    // these are some arbitrary limits you can set and the
    // deserializer will check them and fail if they are
    // exceeded. this is useful if you know the character of
    // the objects that are going to be deserialized, and
    // the input may be from an unknown source.
    // it might be better to provide some means of hooking the
    // memory allocator and terminating after a certain amount of
    // memory had been allocated, but that is environment dependent
    // and would probably require some exception handling.
    // this should be adequate to allow limiting mischief for simple
    // outward facing protocols where the form is fairly restricted.
    long max_elements;
    long max_element_len;
    long max_depth;

private:
	ChitTable *chit_table;
        void chit_destroy_table();
	
	// tells which device to use
	enum devtype {INDEPENDENT, VCPTR, VC, NONE} dtype;
	VCXUNDERFUN uflow;
	VCXOVERFUN oflow;
	vc_default *manager;	// objects whose under/overflow functions to invoke
	vc mgr;

	int read_only;
	
	char *buf;		// pointer to beginning of buffer
	char *cur;		// current read/write pointer
	char *eob;		// one character past end of buf
    char *eod;		// one character past end of valid data for reading
    long len;		// allocated size pointed to by buf
	enum status stat;
	enum style styl;
	enum e_iostyle iostyle;
	int own_buf;	// 1 means we can alloc/dealloc buffer

	DwGrowingString log;
	
	long do_overflow(char *buf, long len);
	long do_underflow(char *buf, long min, long max);
	long do_devopen(int style, int stat);
	long do_devclose(int style);

};

#endif
