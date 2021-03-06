
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
//
// this file provides some simple Unified DH stuff
// it provides both the ephemeral and static key agreement stuff,
// and a "store and forward" form of DH (that uses just the static
// keys). the latter is used where interactive key agreement
// isn't needed (or isn't possible.)
//
#include "vc.h"
#include "vcmap.h"
#ifndef NO_VCCRYPTO
#include <time.h>
#ifdef LINUX
#include <unistd.h>
#include <fcntl.h>
#else
#include <io.h>
#include <fcntl.h>
#endif
#include "dh.h"
#include "rng.h"
#include "dh2.h"
#include "files.h"
#include "hex.h"
#include "blowfish.h"
#include "vcudh.h"
#include "randpool.h"

// this was just for debugging
#ifdef CDCDLL
#include "qauth.h"
#include "dwrtlog.h"
#else
#define GRTLOG(a, b, c)
#define GRTLOGVC(a)
#endif

using namespace CryptoPP;

static DH *EphDH;
static DH2 *UDH;

//static AutoSeededRandomPool *Rng;
static RandomPool *Rng;

// XXX this really needs to be beefed up on windows... if we have access
// to our little entropy pool, we should use it.
static void
init_rng(vc entropy)
{
	// seed, don't bother with it, just use
	// whatever rubbish is on the stack.
	// this bothers valgrind, so init it.
#define N 8
	byte a[N];
	byte k[N];
	memset(a, 0, sizeof(a));
	memset(k, 0, sizeof(k));
	int fd = open("/dev/urandom", O_RDONLY);
	if(fd == -1 ||
		read(fd, k, sizeof(k)) != sizeof(k) ||
		read(fd, a, sizeof(a)) != sizeof(a)
		)
	{
		if(entropy.type() == VC_STRING && entropy.len() >= sizeof(a) + sizeof(k))
		{
			const char *e = (const char *)entropy;
			for(size_t i = 0; i < sizeof(a); ++i)
			{
				a[i] = e[i];
				k[i] = e[sizeof(a) + i];
			}
			
		}
		else
		{
			srand(time(0));
			for(size_t i = 0; i < sizeof(a); ++i)
			{
				a[i] = rand();
				k[i] = rand();
			}
		}
	}
	else
	{
		close(fd);
	}
	//Rng = new X917RNG(new BlowfishEncryption(k, sizeof(k)), a);
	Rng = new RandomPool;
    Rng->IncorporateEntropy(a, sizeof(a));
    Rng->IncorporateEntropy(k, sizeof(k));
}

static vc
sha(vc s)
{
	if(s.type() != VC_STRING)
		return vcnil;
		
	SHA md;
	SecByteBlock b(md.DigestSize());
	md.Update((const byte *)(const char *)s, s.len());
	md.Final(b);
	vc ret(VC_BSTRING, (const char *)b.data(), (long)md.DigestSize());
	return ret;
}

vc
udh_init(vc entropy)
{
	//StringSource f2("3081870281810087CD83F01B8F1FD92BD5DD51A3D519C9675211F4364EB62278E2A711B830E2AEF8EE4C0B81A02CEBAD095F55A83B492B8C8A9F0EB22B3E298001D3F2070297D8EB54E806884BC7D8F2E00720C71DF4B30B18CBA6ABDFA202EAF3344D024BBD22E7416D255DAB105E55440712B436C60377A4BC231B4C1EA186BA003B0E1EBA2B020103", 1, new HexDecoder);
	// 2048 bit dh parameters, ca 2014
	StringSource f2(
	"3082020C0282010100F094119216DBC6E5333B45DC0AFD528F251D1910D39A947B69678C01C811C29B0FC0A5D15BBC127F3E1EE6CB1610792446C82CA4874FA24C988C9D206002F70ACD864C6402E40BB6050CCCB0558C0C5102235C6AC824DB31432D1AC64E73AC2F8EB3990AFEA0BF1915092EB3E45C603F41BDA14E6041589502AC3243A0E76E443787BDE21964D2F002C38C7CAE3AEF35003AE7E63076850FC143EEAC865964C34018A279A694889AE30F879579537231CDB6BCC9FD553A2F0E8EFF4BFE29BC6BB0BF7D4C80238DDA25AF481F3CB7EFC4E98B376FF0E56A413C07B5190FA6B611920B2C355827B2ED5FB18683FB65A44DA566A286AFDE08D018254CA25C33D56302820100784A08C90B6DE372999DA2EE057EA947928E8C8869CD4A3DB4B3C600E408E14D87E052E8ADDE093F9F0F73658B083C922364165243A7D1264C464E9030017B8566C32632017205DB028666582AC606288111AE3564126D98A1968D632739D617C759CC857F505F8C8A849759F22E301FA0DED0A73020AC4A81561921D073B7221BC3DEF10CB269780161C63E571D779A801D73F3183B4287E0A1F756432CB261A00C513CD34A444D7187C3CABCA9B918E6DB5E64FEAA9D1787477FA5FF14DE35D85FBEA64011C6ED12D7A40F9E5BF7E274C59BB7F872B5209E03DA8C87D35B08C905961AAC13D976AFD8C341FDB2D226D2B3514357EF04680C12A6512E19EAB1020103"
	, 1, new HexDecoder);
	EphDH = new DH(f2);
    UDH = new DH2(*EphDH);
	init_rng(entropy);
	return vctrue;
}

vc
udh_new_static(vc entropy)
{
	// create a public static DH that can be stored in our profile
	// future connection setups can use to authenticate us.

    SecByteBlock static_priv(UDH->StaticPrivateKeyLength());
    SecByteBlock static_pub(UDH->StaticPublicKeyLength());
    if(entropy.type() == VC_STRING)
    {
        Rng->IncorporateEntropy((const byte *)(const char *)entropy, entropy.len());
    }
    UDH->GenerateStaticKeyPair(*Rng, static_priv, static_pub);
	vc DH_static = vc(VC_VECTOR);
    DH_static[DH_STATIC_PUBLIC] = vc(VC_BSTRING, (const char *)static_pub.BytePtr(), static_pub.SizeInBytes());
	DH_static[DH_STATIC_PRIVATE] = vc(VC_BSTRING, (const char *)static_priv.BytePtr(), static_priv.SizeInBytes());
	return DH_static;
}

vc
udh_gen_keys(vc DH_static, vc entropy)
{
    SecByteBlock eph_priv(UDH->EphemeralPrivateKeyLength());
    SecByteBlock eph_pub(UDH->EphemeralPublicKeyLength());
    if(entropy.type() == VC_STRING)
    {
        Rng->IncorporateEntropy((const byte *)(const char *)entropy, entropy.len());
    }

    UDH->GenerateEphemeralKeyPair(*Rng, eph_priv, eph_pub);

    vc ret(VC_VECTOR);
    ret[0] = vc(VC_BSTRING, (const char *)eph_priv.BytePtr(), eph_priv.SizeInBytes());
    ret[1] = vc(VC_BSTRING, (const char *)eph_pub.BytePtr(), eph_pub.SizeInBytes());
    ret[2] = DH_static[DH_STATIC_PRIVATE];
    ret[3] = DH_static[DH_STATIC_PUBLIC];

    return ret;
}

vc
udh_just_publics(vc keys)
{
	vc ret(VC_VECTOR);
	ret[0] = vcnil;
	ret[1] = keys[1];
	ret[2] = vcnil;
	ret[3] = keys[3];
	return ret;
}

vc
udh_agree_auth(vc our_material, vc other_publics)
{
    // eventually make sure to check all the sizes on the keys with
    // what is setup in the current set of crypto objects, since some of these
    // presumably came from out in the wilderness

    SecByteBlock agreed(UDH->AgreedValueLength());
    GRTLOG("dh agree ours (priv static, priv eph)", 0, 0)
            GRTLOGVC(to_hex(our_material[2]))
            GRTLOGVC(to_hex(our_material[0]))

    GRTLOG("dh agree ours (pub static, pub eph)", 0, 0)
            GRTLOGVC(to_hex(our_material[3]))
            GRTLOGVC(to_hex(our_material[1]))

            GRTLOG("dh agree theirs (pub static, pub eph)", 0, 0)
            GRTLOGVC(to_hex(other_publics[3]))
            GRTLOGVC(to_hex(other_publics[1]))
    if(UDH->Agree(agreed.BytePtr(),
                  (const byte *)(const char *)our_material[2],
                  (const byte *)(const char *)our_material[0],
                  (const byte *)(const char *)other_publics[3], // note: this should be filled in from profile from server
                  (const byte *)(const char *)other_publics[1]
                  ))
    {
        vc key(VC_BSTRING, (const char *)agreed.BytePtr(), agreed.SizeInBytes());
        GRTLOG("DH key %s", (const char *)to_hex(key), 0)
        return key;

    }
    return vcnil;

}

// this is used to generate the key that is used by all servers for this
// session.
// it returns the public package of info the recipient can use to
// decrypt the key. it also returns the key itself

vc
dh_store_and_forward_material(vc other_pub, vc& session_key_out)
{
    // generate a key from their static public material
    SecByteBlock privk(EphDH->PrivateKeyLength());
    SecByteBlock pubk(EphDH->PublicKeyLength());

    EphDH->GenerateKeyPair(*Rng, privk, pubk);
    SecByteBlock akey(EphDH->AgreedValueLength());
    if(!EphDH->Agree(akey, privk, (const byte *)(const char *)other_pub[DH_STATIC_PUBLIC]))
        return vcnil;
    vc key(VC_BSTRING, (const char *)akey.data(), akey.SizeInBytes());
    vc our_public(VC_BSTRING, (const char *)pubk.data(), pubk.SizeInBytes());

    // normalize to 160 bits
    key = sha(key);
	// 128 bit symmetric key
    SecByteBlock skey(16);
    Rng->GenerateBlock(skey, skey.SizeInBytes());
    vc session_key(VC_BSTRING, (const char *)(const byte *)skey, skey.SizeInBytes());

    const byte *k = (const byte *)(const char *)key;
    for(size_t i = 0; i < skey.SizeInBytes(); ++i)
        skey[i] ^= k[i];
    vc session_key_enc(VC_BSTRING, (const char *)(const byte *)skey, skey.SizeInBytes());

    vc ret(VC_VECTOR);
    // encrypted output is
    // 0: session key encrypted with public key
    // 1: public DH value to decrypt
    ret[0] = session_key_enc;
    ret[1] = our_public;
    session_key_out = session_key;

    return ret;
}

vc
vclh_sf_material(vc other_pub, vc key_out)
{
	if(key_out.type() != VC_STRING)
	{
		USER_BOMB("second arg to SF-material must be a string to bind to", vcnil);
	}
	vc session_key_out;
	vc ret = dh_store_and_forward_material(other_pub, session_key_out);
	Vcmap->local_add(key_out, session_key_out);
	return ret;
}


// sfpack is the package of info created by dh_store_and_forward_material, presumably
// created by the sender. here is where we do the agreement and recover the session key.
vc
dh_store_and_forward_get_key(vc sfpack, vc our_material)
{
    SecByteBlock akey(EphDH->AgreedValueLength());
    if(!EphDH->Agree(akey, (const byte *)(const char *)our_material[DH_STATIC_PRIVATE],
                 (const byte *)(const char *)sfpack[1]))
        return vcnil;

    vc kdk(VC_BSTRING, (const char *)akey.data(), akey.SizeInBytes());

    kdk = sha(kdk);

    vc sk_enc = sfpack[0];
    if(!(sk_enc.type() == VC_STRING && sk_enc.len() <= kdk.len()))
    {
        return vcnil;
    }

    const byte *k = (const byte *)(const char *)sk_enc;
    SecByteBlock sk(sk_enc.len());
    const byte *k2 = (const byte *)(const char *)kdk;

    for(int i = 0; i < sk_enc.len(); ++i)
    {
        sk[i] = k[i] ^ k2[i];
    }

    vc ret(VC_BSTRING, (const char *)sk.BytePtr(), sk.SizeInBytes());
    return ret;
}


#undef TEST
#ifdef TEST
// g++ -g -fpermissive -I../../ins/dbg/inc -I./winemu -I. dhsetup.cpp -DVCCFG_FILE -DSTANDALONE xinfo.cpp fnmod.cc ../../ins/dbg/lib/crypto5.a ../../ins/dbg/lib/vc.a ../../ins/dbg/lib/dwcls.a ../../ins/dbg/lib/kazlib.a ../../ins/dbg/lib/jenkins.a

class DwAllocator;
DwAllocator *Default_alloc;

void
oopanic(const char *)
{
	::abort();
}

int
main(int, char **)
{
	dh_init();

	// pretend this is client
	dh_load_account("a1/dh.dif");
    vc client_dh = DH_static;

    // server
    dh_load_account("a2/dh.dif");
    vc server_dh = DH_static;

    vc session_key;
    vc pub_material = dh_store_and_forward_material(server_dh, session_key);

    vc("\n----\n").print_top(VcOutput);
    pub_material.print_top(VcOutput);
    vc("\n----\n").print_top(VcOutput);
    session_key.print_top(VcOutput);
    vc("\n----\n").print_top(VcOutput);

    vc sfkey = dh_store_and_forward_get_key(pub_material, server_dh);

    vc("\n----\n").print_top(VcOutput);
    sfkey.print_top(VcOutput);
    vc("\n----\n").print_top(VcOutput);

    exit(0);

#if 0
	dh_new_account();
#else
	dh_load_account("a1/dh.dif");
	vc k1 = dh_gen_keys();

	vc pubs1(VC_VECTOR);
	pubs1[0] = k1[1];
	pubs1[1] = k1[3];

	save_info(pubs1, "xmit1");

	dh_load_account("a2/dh.dif");

	vc k2 = dh_gen_keys();

	vc pubs2(VC_VECTOR);
	pubs2[0] = k2[1];
	pubs2[1] = k2[3];

	save_info(pubs2, "xmit2");

	vc ag1 = dh_agree_auth(k2, dh_just_publics(k1));

	vc ag2 = dh_agree_auth(k1, dh_just_publics(k2));

	ag1.print_top(VcOutput);
	vc("\n----\n").print_top(VcOutput);
	ag2.print_top(VcOutput);
	vc("\n----\n").print_top(VcOutput);

	if(ag1 != ag2)
		::abort();


#endif

}

#endif

#endif
