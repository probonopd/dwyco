
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
;
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include <QList>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include <QCamera>
#include <QCameraInfo>
#include <QVideoFrame>
#include <QVideoProbe>
#include <QQmlApplicationEngine>
#include <QtConcurrent>
#include <QFuture>
#include <QThread>

#include "vgqt.h"
#include "vidaq.h"
#include "pgm.h"

#include <stdio.h>
#include <time.h>
#ifdef _Windows
#include <WinSock2.h>
#endif
#ifdef LINUX
#include <unistd.h>
#endif



#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE   STBIR_FILTER_BOX
#define STBIR_SATURATE_INT
#include "stb_image_resize.h"
#undef TEST_THREAD
#ifdef TEST_THREAD
#include <pthread.h>
#endif

extern QQmlApplicationEngine *TheEngine;

#define NB_BUFFER 4
static QVector<unsigned long> y_bufs(NB_BUFFER);
static QVector<unsigned int> lens(NB_BUFFER);
static QVector<QVideoFrame> vbufs(NB_BUFFER);
static int next_ibuf;
static int next_buf;
static int debug = 1;
static QMutex mutex;
static int Orientation;


struct finished
{
    finished() {
        r = c = bytes = fmt = -1;
        planes[0] = 0;
        planes[1] = 0;
        planes[2] = 0;
        captime = 0;
    }

    void flush() {
        pgm_freearray(planes[0], r);
        pgm_freearray(planes[1], r / 2);
        pgm_freearray(planes[2], r / 2);
    }

    gray **planes[3];
    int r;
    int c;
    int bytes;
    int fmt;
    unsigned long captime;
};
static QVector<QFuture<finished> > futs(NB_BUFFER);
static QVector<finished> conv_buf(NB_BUFFER);
static int next_cb;
static int next_icb;

void oopanic(const char *);

class probe_handler : public QObject
{
    Q_OBJECT
public:
    probe_handler(QObject *parent = 0) : QObject(parent) {}

    QVideoProbe probe;
public slots:
    void handleFrame(const QVideoFrame&);
    //void flush_frames();

};
#include "vgqt.moc"

static probe_handler *Probe_handler;
static int stop_thread;
#ifdef TEST_THREAD
static pthread_t thread;
#endif

// this is needed because on older versions of
// ios (like 9), it doesn't have clock_gettime, and
// in addition to that, it weak-links it, so the program
// just crashes despite compiling and deploying ok.
//
#if defined(DWYCO_IOS) || defined(MACOSX)
#include <sys/time.h>
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0
static int
clock_gettime(int, struct timespec *t)
{
    struct timeval now;
    int rv = gettimeofday(&now, 0);
    if(rv) return rv;
    t->tv_sec = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#endif

#ifdef TEST_THREAD
static void
add_frame(QVideoFrame frm)
{
    QMutexLocker ml(&mutex);

    if(next_buf == (next_ibuf + 1) % NB_BUFFER)
    {
        // drop it for now, maybe something more complicated
        // like overwriting next frame would look better
        // in some cases, but not worth it at this point.
        return;
    }
    struct timeval tm;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    tm.tv_sec = ts.tv_sec;
    tm.tv_usec = ts.tv_nsec / 1000;

    vbufs[next_ibuf] = frm;
    y_bufs[next_ibuf] = ((tm.tv_sec * 1000000) + tm.tv_usec) / 1000; // turn into msecs
    next_ibuf = (next_ibuf + 1) % NB_BUFFER;
}

static void *
test_thread(void *)
{
    int t = 0;
    Orientation = 270;
    while(1)
    {
        if(stop_thread)
            return 0;
        QVideoFrame f(320*240+2*(160 * 120), QSize(320, 240), 320, QVideoFrame::Format_NV21);
        f.map(QAbstractVideoBuffer::ReadWrite);
        uchar *bits = f.bits();
        memset(bits, t, 320 * 240);
        memset(bits + 320*60, 0, 120);
        memset(bits + 320*61, 0, 120);
        memset(bits + 320*62, 255, 120);
        memset(bits + 320*63, 255, 120);
        memset(bits + 320*240, 0, 2 * 160 * 120);
        ++t;
        f.unmap();
        add_frame(f);
        QThread::msleep(30);
    }
    return 0;
}
#endif

static
gray **
pgm_rot(gray **inp, int& cols, int& rows, int rot)
{
    if(rot == 0)
        return inp;
    gray *s = inp[0]; //&inp[0][0];
    gray **dst;
    if(rot == 180)
    {
        dst = pgm_allocarray(cols, rows);
        for(int r = 0; r < rows; ++r)
        {
            for(int c = 0; c < cols; ++c)
            {
                dst[rows - 1 - r][cols - 1 - c] = *s++;
            }
        }
    }
    else
    {
        int tmp = cols;
        cols = rows;
        rows = tmp;
        dst = pgm_allocarray(cols, rows);
        if(rot == 90)
        {
            for(int c = cols - 1; c >= 0; --c)
            {
                for(int r = 0; r < rows; ++r)
                {
                    dst[r][c] = *s++;
                }
            }
        }
        else if(rot == 270)
        {
            for(int c = 0; c < cols; ++c)
            {
                for(int r = rows - 1; r >= 0; --r)
                {
                    dst[r][c] = *s++;
                }
            }

        }
    }
    return dst;

}

// this is for nv21
// c should point to the first sample in the block of
// interleaved chroma samples
static void
get_interleaved_chroma_planes(int ccols, int crows, unsigned char *c, gray**& vu_out, gray**& vv_out, int subsample)
{
    int ch_cols = ccols / subsample;
    int ch_rows = crows / subsample;
    int autoconfig = 0;
    int upside_down = 0;

    gray **u_out = pgm_allocarray(ch_cols, ch_rows);
    gray **v_out = pgm_allocarray(ch_cols, ch_rows);
    vu_out = u_out;
    vv_out = v_out;

    //if((autoconfig && !upside_down) || (!autoconfig && upside_down))
    {
        gray *ut = &u_out[0][0];
        gray *vt = &v_out[0][0];
        gray *us = c;
        gray *vs = c + 1;
        for(int i = 0; i < ch_rows; ++i)
        {
            for(int j = 0; j < ch_cols; ++j)
            {
                *ut++ = *us;
                us += 2;
                *vt++ = *vs;
                vs += 2;
            }
        }
    }
//    else
//    {
//        gray *us = c;
//        gray *vs = c + 1;
//        for(int i = ch_rows - 1; i >= 0; --i)
//        {
//            gray *ut = &u_out[i][0];
//            gray *vt = &v_out[i][0];
//            for(int j = 0; j < ch_cols; ++j)
//            {
//                *ut++ = *us;
//                us += 2;
//                *vt++ = *vs;
//                vs += 2;
//            }
//        }
//    }
}

char **
DWYCOEXPORT
vgqt_get_video_devices()
{
    char **r = new char *[2];
    r[0] = new char [strlen("Camera") + 1];
    strcpy(r[0], "Camera");
    r[1] = 0;
    return r;
}

void
DWYCOEXPORT
vgqt_free_video_devices(char **d)
{
    char **tmp = d;
    while(*d)
    {
        delete [] *d;
        ++d;
    }
    delete [] tmp;
}

// if the video cap device is in use, we
// turn off the old one, select the new one, and
// restart capturing.
void
DWYCOEXPORT
vgqt_set_video_device(int idx)
{
    if(!vgqt_init(0, 0))
    {

    }
}

void
DWYCOEXPORT
vgqt_show_source_dialog()
{
}


void DWYCOEXPORT
vgqt_set_appdata(void *u1)
{
}

void
DWYCOEXPORT
vgqt_preview_on(void *display_window)
{
}

void
DWYCOEXPORT
vgqt_preview_off()
{
}

// this should shutdown and release the device.
void
DWYCOEXPORT
vgqt_stop_video_device()
{
    if(Probe_handler)
    {
        vgqt_stop(0);
        vgqt_pass(0);
        delete Probe_handler;
        Probe_handler = 0;
    }
}



void
DWYCOEXPORT
vgqt_new(void *aqext)
{
    if(Probe_handler)
        return;
    Probe_handler = new probe_handler;
#ifdef TEST_THREAD
    stop_thread = 0;
    pthread_create(&thread, 0, test_thread, 0);
#endif
}

void
DWYCOEXPORT
vgqt_del(void *aqext)
{
    if(Probe_handler)
    {
        vgqt_stop(0);
        vgqt_pass(0);

    }
    delete Probe_handler;
    Probe_handler = 0;
    stop_thread = 1;
#ifdef TEST_THREAD
    pthread_join(thread, 0);
#endif
}

int
DWYCOEXPORT
vgqt_init(void *aqext, int frame_rate)
{
    if(!Probe_handler)
        Probe_handler = new probe_handler;
#ifdef TEST_THREAD
    return 1;
#endif
    QList<QObject *> ro = TheEngine->rootObjects();
    for(int i = 0; i < ro.count(); ++i)
    {
        QObject *qmlCamera = ro[i]->findChild<QObject*>("qrCameraQML");
        if(!qmlCamera)
        {
            continue;
        }
        QCamera *camera_ = qvariant_cast<QCamera*>(qmlCamera->property("mediaObject"));
        if(!camera_)
            return 0;
#ifdef DWYCO_IOS
        QCameraViewfinderSettings vfs;
        vfs = camera_->viewfinderSettings();
        vfs.setPixelFormat(QVideoFrame::Format_NV12);
        camera_->setViewfinderSettings(vfs);
#elif !defined(ANDROID)
        QCameraViewfinderSettings vfs;
        vfs = camera_->viewfinderSettings();
        vfs.setPixelFormat(QVideoFrame::Format_YV12);
        camera_->setViewfinderSettings(vfs);
#endif
        QCameraInfo caminfo(*camera_);
        Orientation = caminfo.orientation();
        QObject::connect(&Probe_handler->probe, SIGNAL(videoFrameProbed(QVideoFrame)),
                         Probe_handler, SLOT(handleFrame(QVideoFrame)), Qt::UniqueConnection);

        if(Probe_handler->probe.setSource(camera_))
            return 1;
    }
    return 0;
}

int
DWYCOEXPORT
vgqt_has_data(void *aqext)
{
    QMutexLocker ml(&mutex);

    if(next_cb == next_icb)
    {
        return 0;
    }
    if(futs[next_icb].isFinished())
        return 1;
    return 0;
}

void
DWYCOEXPORT
vgqt_pass(void *aqext)
{
    QMutexLocker ml(&mutex);
    while(next_buf != next_ibuf)
    {
        vbufs[next_buf] = QVideoFrame();
        //GRTLOG("chuck %d", (int)y_bufs[next_buf], 0);
        next_buf = (next_buf + 1) % NB_BUFFER;
    }
}

void
DWYCOEXPORT
vgqt_stop(void *aqext)
{
    // XXX need to make sure thread is dead and then
    // see about the capture device too
    if(!Probe_handler)
        return;
    Probe_handler->probe.setSource((QMediaObject *)0);
}

void *
DWYCOEXPORT
vgqt_get_data(
    void *aqext,
    int *c_out, int *r_out,
    int *bytes_out, int *fmt_out, unsigned long *captime_out)
{
    QMutexLocker ml(&mutex);

    finished *f;
    if(!futs[next_icb].isFinished())
        oopanic("bad get data");
    f = new finished;
    *f = futs[next_icb].result();
    *c_out = f->c;
    *r_out = f->r;
    *bytes_out = f->bytes;
    *fmt_out = f->fmt;
    *captime_out = f->captime;

    next_icb = (next_icb + 1) % NB_BUFFER;

    // this is ugly, should fix this, but can't be bothered
    // until some testing is done
    return f;
}

static
struct finished
conv_data()
{

    QMutexLocker ml(&mutex);

    struct finished f;

    if(next_buf != next_ibuf)
    {
        int nb = next_buf;
        int cols, rows;
        QVideoFrame vf = vbufs[nb];
        f.captime = y_bufs[nb];
        vbufs[nb] = QVideoFrame();
        next_buf = (next_buf + 1) % NB_BUFFER;
        ml.unlock();

        if(!vf.map(QAbstractVideoBuffer::ReadOnly))
        {
            oopanic("can't map");
        }
        f.r = rows = vf.height();
        f.c = cols = vf.width();

        int fmt = 0;
        int swap = 0;
        switch(vf.pixelFormat())
        {
        case QVideoFrame::Format_RGB24:
            fmt = AQ_RGB24;
            break;
        case QVideoFrame::Format_RGB555:
            fmt = AQ_RGB555;
            break;
        case QVideoFrame::Format_YUV420P:
            fmt = AQ_YUV12;
            swap = 1;
            break;
        case QVideoFrame::Format_YV12:
            fmt = AQ_YUV12;
            break;
        case QVideoFrame::Format_UYVY:
            fmt = AQ_UYVY;
            break;
        case QVideoFrame::Format_YUYV:
            fmt = AQ_YUY2;
            break;
        // note: NV12 seems to be the closest thing to
        // NV21 provided by the qt ios driver. this isn't
        // in the documentation, but was gleaned from reading
        // the source for 5.6.2. it *appears* to work, as long
        // as you set the format explicitly in the camera setup.
        // though it needs more testing because sometimes it doesn't
        // appear to get setup properly, and we still end up with
        // ARGB32 in here.
        case QVideoFrame::Format_NV12:
            swap = 1;
        // FALL THRU
        case QVideoFrame::Format_NV21:
            fmt = AQ_NV21;
            // for now, if the stride isn't the same as
            // the dimensions, bail
            if(vf.bytesPerLine() != vf.width())
                ::abort();
            break;
        default:
            // just a guess so we don't crash
            fmt = AQ_RGB24;
        }

        // note for android, nv21 is standard, and we convert to
        // yuv12
        f.fmt = (AQ_COLOR|AQ_YUV12);
#ifndef ANDROID
        if(fmt & AQ_YUV12)
        {
            unsigned char *c = (unsigned char *)vf.bits();
            gray **g = pgm_allocarray(cols, rows);
            memcpy(&g[0][0], c, cols * rows);
            c += cols * rows;
            f.planes[0] = g;

            g = pgm_allocarray(cols / 2, rows / 2);
            memcpy(&g[0][0], c, (cols * rows) / 4);
            f.planes[1] = g;
            c += (cols * rows) / 4;

            g = pgm_allocarray(cols / 2, rows / 2);
            memcpy(&g[0][0], c, (cols * rows) / 4);
            f.planes[2] = g;

            if(swap)
            {
                gray **tmp = f.planes[1];
                f.planes[1] = f.planes[2];
                f.planes[2] = tmp;
            }

            vf.unmap();
            vf = QVideoFrame();
            return f;


        }
#endif

        unsigned char *c = (unsigned char *)vf.bits();
#define SSCOLS 320
#define SSROWS (calcrows)
        int calcrows = (float)rows / ((float)cols / SSCOLS);
        if(calcrows % 2 != 0) ++calcrows;

        gray **g = pgm_allocarray(SSCOLS, SSROWS);
        int ncols = SSCOLS;
        int nrows = SSROWS;
        stbir_resize_uint8(c, cols, rows, 0, &g[0][0], SSCOLS, SSROWS, 0, 1);

        //
        if(Orientation != 0)
        {
            gray **rg = pgm_rot(g, ncols, nrows, Orientation);
            pgm_freearray(g, SSROWS);
            g = rg;
        }

        c += f.c * f.r;
        // note: 2 channels, cb and cr
        gray **gc = pgm_allocarray(SSCOLS, SSROWS / 2);
        stbir_resize_uint8(c, cols / 2, rows / 2, 0, &gc[0][0], SSCOLS / 2, SSROWS / 2, 0, 2);

        gray **cr;
        gray **cb;

        get_interleaved_chroma_planes(SSCOLS / 2, SSROWS / 2, &gc[0][0], cr, cb, 1);
        if(swap)
        {
            gray **tmp = cr;
            cr = cb;
            cb = tmp;
        }
        pgm_freearray(gc, SSROWS / 2);

        if(Orientation != 0)
        {
            int c = SSCOLS / 2;
            int r = SSROWS / 2;
            gray **rcr = pgm_rot(cr, c, r, Orientation);
            pgm_freearray(cr, SSROWS / 2);
            cr = rcr;

            c = SSCOLS / 2;
            r = SSROWS / 2;
            gray **rcb = pgm_rot(cb, c, r, Orientation);
            pgm_freearray(cb, SSROWS / 2);
            cb = rcb;
        }

        //cols = SSCOLS;
        //rows = SSROWS;
        f.c = ncols;
        f.r = nrows;

        f.planes[0] = g;
        f.planes[1] = cb;
        f.planes[2] = cr;
        // note: f.bytes isn't really used
        // on android, and it doesn't make
        // a lot of sense since we are returning
        // discontiguous blocks of memory for the
        // planes.

        vf.unmap();
        vf = QVideoFrame();
        return f;
    }

    oopanic("aqvfw get no data");
    // not reached
    return finished();
}

void
DWYCOEXPORT
vgqt_free_data(void *data)
{
    delete (finished *)data;
}

void
DWYCOEXPORT
vgqt_need(void *aqext)
{
    QMutexLocker ml(&mutex);
    if(next_ibuf == next_buf)
    {
        // no raw frames to process
        return;
    }
    if(next_cb == (next_icb + 1) % NB_BUFFER)
    {
        // no room to start another conversion
        return;
    }
    futs[next_cb] = QtConcurrent::run(conv_data);
    next_cb = (next_cb + 1) % NB_BUFFER;
}


void
probe_handler::handleFrame(const QVideoFrame& frm)
{
    QMutexLocker ml(&mutex);

    if(next_buf == (next_ibuf + 1) % NB_BUFFER)
    {
        // drop it for now, maybe something more complicated
        // like overwriting next frame would look better
        // in some cases, but not worth it at this point.
        return;
    }
#ifdef __WIN32__
    y_bufs[next_ibuf] = timeGetTime();
#else
    struct timeval tm;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    tm.tv_sec = ts.tv_sec;
    tm.tv_usec = ts.tv_nsec / 1000;
    y_bufs[next_ibuf] = ((tm.tv_sec * 1000000) + tm.tv_usec) / 1000; // turn into msecs
#endif

    vbufs[next_ibuf] = frm;

    next_ibuf = (next_ibuf + 1) % NB_BUFFER;
}


