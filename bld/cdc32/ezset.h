
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#ifndef EZSET_H
#define EZSET_H

#include "vc.h"

vc get_settings_value(const char *name);
int set_settings_value(const char *name, const char *value);
#endif
