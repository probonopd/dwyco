
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#ifdef DWYCO_DO_USER_LOG
/*
 * $Header: g:/dwight/repo/cdc32/rcs/dwlog.cc 1.4 1997/11/25 20:41:03 dwight Stable095 $
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dwdate.h"
#include "dwlog.h"
#include "vcio.h"
using namespace dwyco;

DwLog::DwLog(const char *f)
{
    filename = strdup(f);
}

DwLog::~DwLog()
{
    free(filename);
}

void
DwLog::make_entry(const char *s, const char *s2, const char *s3)
{
#ifdef DWLOG
    FILE *f = fopen(filename, "a");
    if(!f)
        return;
    VcIOHack o(f);
    o << DwTime().AsString().c_str() << " ";
    if(s)
        o << s;
    if(s2)
        o << s2;
    if(s3)
        o << s3;
#ifdef _Windows
    o << "\r\n";
#else
    o << "\n";
#endif
    fclose(f);
#endif
}

void
DwLog::make_entry(vc v)
{
#ifdef DWLOG
    FILE *f = fopen(filename, "a");
    if(!f)
        return;
    VcIOHack o(f);
    o << DwTime().AsString().c_str() << " ";
    v.print_top(o);
#ifdef _Windows
    o << "\r\n";
#else
    o << '\n';
#endif
    fclose(f);
#endif
}
#endif
