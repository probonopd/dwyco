
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#ifndef FNMOD_H
#define FNMOD_H
#include "vc.h"
#include "dwstr.h"

namespace dwyco {
int filename_modify(const DwString& fn, DwString& fn_out);
DwString newfn(const DwString& fn);
DwString newfn(const char *);
DwString newfn_userpfx(const char *);
DwString prepend_pfx(const char *subdir, const char *fn);

DwString dwbasename(const char *name);
int is_attachment(vc fn);
int is_attachment(const DwString& fn);
DwString fn_extension(const DwString& fn);
DwString fn_extension(vc fn);
DwString fn_base_wo_extension(const DwString& fn);

void
set_fn_prefixes(
    const char *sys_pfx,
    const char *user_pfx,
    const char *tmp_pfx
);
void
get_fn_prefixes(
    DwString& sys_pfx,
    DwString& user_pfx,
    DwString& tmp_pfx
);
}
using namespace dwyco;

#endif

