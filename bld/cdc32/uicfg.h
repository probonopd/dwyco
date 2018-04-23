
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#ifndef UICFG_H
#define UICFG_H

// this macros are used to transfer data back
// and forth between whatever UI is out there
// and the internal structs used by the core.
// the structs were originally generated by
// OWL, but they are generic enough that there
// should be no problem getting stuff from other
// user interface frameworks into them.
//
// there is also a generic tprofile replacement
// that doesn't use ini files. this avoids
// having to redo all the load/save stuff that
// was written for owl.

// these macros generate a get* and set*
// member function for each member in the
// struct. the cdc core uses the members
// directly (that way we don't have to change
// all the code that currently uses the structs).
// but other user interfaces should only use
// the get* and set* members to change the values.
//

#define DWUIDECLVAR(name) \
	SyncVar dwvc##name;

#define DWUIDECL_BEGIN \
	SyncMap *syncmap; \
	DWUITYPELESS_GETVAL \
	DWUITYPELESS_SETVAL

#define DWUIDECL_END

#define DWUIINIT_CTOR_BEGIN \
    : syncmap(new SyncMap(31))

#define DWUIINIT_CTOR_VAL(name) \
	dwvc##name(#name, syncmap)

#define DWUIINIT_CTOR_END

#define DWUISETVAL(type, name) \
	void set_##name(type val) { \
		dwvc##name = val; \
	}

#if 0
if(strcmp(#type, "char *") == 0) \
    { \
      strcpy((char *)name, (char *)val); \
    } \
    else \
        *(type *)&name = val;
#endif

#define DWUITYPELESS_SETVAL \
	void set_vc(const char *nm, vc val) { \
		syncmap->replace(nm, val); \
	}

#define DWUIGETVAL(type, name) \
	type get_##name() { \
		return (type)dwvc##name; \
	}

#define DWUITYPELESS_GETVAL \
	vc get_vc(const char *nm) { \
		vc ret; \
		if(!syncmap->find(nm, ret)) \
			return vcnil; \
		return ret; \
	}


// type must be an integral type, but the value
// is transferred in as a char
#define DWUISETVAL_char_int(type, name) \
	void set_##name(type val) { \
		dwvc##name = val; \
	}
//		itoa(val, name, 10);

#define DWUIGETVAL_char_int(type, name) \
	type get_##name() { \
		return (type)dwvc##name; \
	}
//return atoi(name);

#define DWUIDECLVAL(type, name) \
	DWUIDECLVAR(name) \
	DWUISETVAL(type, name) \
	DWUIGETVAL(type, name)

#define DWUIDECLVAL_char_int(type, name) \
	DWUIDECLVAR(name) \
	DWUISETVAL(type, name) \
	DWUISETVAL_char_int(int, name) \
	DWUIGETVAL_char_int(int, name)


#define TProfile DwTProfile
#include "vc.h"
#include "syncvar.h"
void load_syncmap(SyncMap *, const char *);
void save_syncmap(SyncMap *, const char *);

#define BF_CHECKED 1
#define BF_UNCHECKED 0
#define TYPESAFE_DOWNCAST(ptr, class) (dynamic_cast<class *>(ptr))

class DwTProfile
{
public:
    DwTProfile(const char *section, const char *filename, SyncMap * = 0);

    void WriteInt(const char *name, int value);
    void WriteString(const char *name, const char *value);
    void GetString(const char *name, char *buf, int buflen, const char *df);
    int GetInt(const char *name, int df);

    vc map;
    char section[255];
    SyncMap *sm;
};

#endif

