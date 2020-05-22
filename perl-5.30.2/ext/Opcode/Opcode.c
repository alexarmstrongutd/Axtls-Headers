/*
 * This file was generated automatically by ExtUtils::ParseXS version 3.40 from the
 * contents of Opcode.xs. Do not edit this file, edit Opcode.xs instead.
 *
 *    ANY CHANGES MADE HERE WILL BE LOST!
 *
 */

#line 1 "Opcode.xs"
#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/* PL_maxo shouldn't differ from MAXO but leave room anyway (see BOOT:)	*/
#define OP_MASK_BUF_SIZE (MAXO + 100)

/* XXX op_named_bits and opset_all are never freed */
#define MY_CXT_KEY "Opcode::_guts" XS_VERSION

typedef struct {
    HV *	x_op_named_bits;	/* cache shared for whole process */
    SV *	x_opset_all;		/* mask with all bits set	*/
    IV		x_opset_len;		/* length of opmasks in bytes	*/
#ifdef OPCODE_DEBUG
    int		x_opcode_debug;		/* unused warn() emitting debugging code */
#endif
} my_cxt_t;

START_MY_CXT

#define op_named_bits		(MY_CXT.x_op_named_bits)
#define opset_all		(MY_CXT.x_opset_all)
#define opset_len		(MY_CXT.x_opset_len)
#ifdef OPCODE_DEBUG
#  define opcode_debug		(MY_CXT.x_opcode_debug)
#else
 /* no API to turn this on at runtime, so constant fold the code away */
#  define opcode_debug		0
#endif

static SV  *new_opset (pTHX_ SV *old_opset);
static int  verify_opset (pTHX_ SV *opset, int fatal);
static void set_opset_bits (pTHX_ char *bitmap, SV *bitspec, int on, const char *opname);
static void put_op_bitspec (pTHX_ const char *optag,  STRLEN len, SV *opset);
static SV  *get_op_bitspec (pTHX_ const char *opname, STRLEN len, int fatal);


/* Initialise our private op_named_bits HV.
 * It is first loaded with the name and number of each perl operator.
 * Then the builtin tags :none and :all are added.
 * Opcode.pm loads the standard optags from __DATA__
 * XXX leak-alert: data allocated here is never freed, call this
 *     at most once
 */

static void
op_names_init(pTHX)
{
    int i;
    STRLEN len;
    char **op_names;
    U8 *bitmap;
    dMY_CXT;

    op_named_bits = newHV();
    op_names = get_op_names();
    for(i=0; i < PL_maxo; ++i) {
	SV * const sv = newSViv(i);
	SvREADONLY_on(sv);
	(void) hv_store(op_named_bits, op_names[i], strlen(op_names[i]), sv, 0);
    }

    put_op_bitspec(aTHX_ STR_WITH_LEN(":none"), sv_2mortal(new_opset(aTHX_ Nullsv)));

    opset_all = new_opset(aTHX_ Nullsv);
    bitmap = (U8*)SvPV(opset_all, len);
    memset(bitmap, 0xFF, len-1); /* deal with last byte specially, see below */
    /* Take care to set the right number of bits in the last byte */
    bitmap[len-1] = (PL_maxo & 0x07) ? ((~(0xFF << (PL_maxo & 0x07))) & 0xFF)
                                     : 0xFF;
    put_op_bitspec(aTHX_ STR_WITH_LEN(":all"), opset_all); /* don't mortalise */
}


/* Store a new tag definition. Always a mask.
 * The tag must not already be defined.
 * SV *mask is copied not referenced.
 */

static void
put_op_bitspec(pTHX_ const char *optag, STRLEN len, SV *mask)
{
    SV **svp;
    dMY_CXT;

    verify_opset(aTHX_ mask,1);
    svp = hv_fetch(op_named_bits, optag, len, 1);
    if (SvOK(*svp))
	croak("Opcode tag \"%s\" already defined", optag);
    sv_setsv(*svp, mask);
    SvREADONLY_on(*svp);
}



/* Fetch a 'bits' entry for an opname or optag (IV/PV).
 * Note that we return the actual entry for speed.
 * Always sv_mortalcopy() if returning it to user code.
 */

static SV *
get_op_bitspec(pTHX_ const char *opname, STRLEN len, int fatal)
{
    SV **svp;
    dMY_CXT;

    svp = hv_fetch(op_named_bits, opname, len, 0);
    if (!svp || !SvOK(*svp)) {
	if (!fatal)
	    return Nullsv;
	if (*opname == ':')
	    croak("Unknown operator tag \"%s\"", opname);
	if (*opname == '!')	/* XXX here later, or elsewhere? */
	    croak("Can't negate operators here (\"%s\")", opname);
	if (isALPHA(*opname))
	    croak("Unknown operator name \"%s\"", opname);
	croak("Unknown operator prefix \"%s\"", opname);
    }
    return *svp;
}



static SV *
new_opset(pTHX_ SV *old_opset)
{
    SV *opset;
    dMY_CXT;

    if (old_opset) {
	verify_opset(aTHX_ old_opset,1);
	opset = newSVsv(old_opset);
    }
    else {
	opset = newSV(opset_len);
	Zero(SvPVX_const(opset), opset_len + 1, char);
	SvCUR_set(opset, opset_len);
	(void)SvPOK_only(opset);
    }
    /* not mortalised here */
    return opset;
}


static int
verify_opset(pTHX_ SV *opset, int fatal)
{
    const char *err = NULL;
    dMY_CXT;

    if      (!SvOK(opset))              err = "undefined";
    else if (!SvPOK(opset))             err = "wrong type";
    else if (SvCUR(opset) != (STRLEN)opset_len) err = "wrong size";
    if (err && fatal) {
	croak("Invalid opset: %s", err);
    }
    return !err;
}


static void
set_opset_bits(pTHX_ char *bitmap, SV *bitspec, int on, const char *opname)
{
    dMY_CXT;

    if (SvIOK(bitspec)) {
	const int myopcode = SvIV(bitspec);
	const int offset = myopcode >> 3;
	const int bit    = myopcode & 0x07;
	if (myopcode >= PL_maxo || myopcode < 0)
	    croak("panic: opcode \"%s\" value %d is invalid", opname, myopcode);
	if (opcode_debug >= 2)
	    warn("set_opset_bits bit %2d (off=%d, bit=%d) %s %s\n",
			myopcode, offset, bit, opname, (on)?"on":"off");
	if (on)
	    bitmap[offset] |= 1 << bit;
	else
	    bitmap[offset] &= ~(1 << bit);
    }
    else if (SvPOK(bitspec) && SvCUR(bitspec) == (STRLEN)opset_len) {

	STRLEN len;
	const char * const specbits = SvPV(bitspec, len);
	if (opcode_debug >= 2)
	    warn("set_opset_bits opset %s %s\n", opname, (on)?"on":"off");
	if (on) 
	    while(len-- > 0) bitmap[len] |=  specbits[len];
	else
	    while(len-- > 0) bitmap[len] &= ~specbits[len];
    }
    else
	croak("panic: invalid bitspec for \"%s\" (type %u)",
		opname, (unsigned)SvTYPE(bitspec));
}


static void
opmask_add(pTHX_ SV *opset)	/* THE ONLY FUNCTION TO EDIT PL_op_mask ITSELF	*/
{
    int i,j;
    char *bitmask;
    STRLEN len;
    int myopcode = 0;
    dMY_CXT;

    verify_opset(aTHX_ opset,1);		/* croaks on bad opset	*/

    if (!PL_op_mask)		/* caller must ensure PL_op_mask exists	*/
	croak("Can't add to uninitialised PL_op_mask");

    /* OPCODES ALREADY MASKED ARE NEVER UNMASKED. See opmask_addlocal()	*/

    bitmask = SvPV(opset, len);
    for (i=0; i < opset_len; i++) {
	const U16 bits = bitmask[i];
	if (!bits) {	/* optimise for sparse masks */
	    myopcode += 8;
	    continue;
	}
	for (j=0; j < 8 && myopcode < PL_maxo; )
	    PL_op_mask[myopcode++] |= bits & (1 << j++);
    }
}

static void
opmask_addlocal(pTHX_ SV *opset, char *op_mask_buf) /* Localise PL_op_mask then opmask_add() */
{
    char *orig_op_mask = PL_op_mask;
#ifdef OPCODE_DEBUG
    dMY_CXT;
#endif

    SAVEVPTR(PL_op_mask);
    /* XXX casting to an ordinary function ptr from a member function ptr
     * is disallowed by Borland
     */
    if (opcode_debug >= 2)
	SAVEDESTRUCTOR((void(*)(void*))Perl_warn,"PL_op_mask restored");
    PL_op_mask = &op_mask_buf[0];
    if (orig_op_mask)
	Copy(orig_op_mask, PL_op_mask, PL_maxo, char);
    else
	Zero(PL_op_mask, PL_maxo, char);
    opmask_add(aTHX_ opset);
}



#line 261 "Opcode.c"
#ifndef PERL_UNUSED_VAR
#  define PERL_UNUSED_VAR(var) if (0) var = var
#endif

#ifndef dVAR
#  define dVAR		dNOOP
#endif


/* This stuff is not part of the API! You have been warned. */
#ifndef PERL_VERSION_DECIMAL
#  define PERL_VERSION_DECIMAL(r,v,s) (r*1000000 + v*1000 + s)
#endif
#ifndef PERL_DECIMAL_VERSION
#  define PERL_DECIMAL_VERSION \
	  PERL_VERSION_DECIMAL(PERL_REVISION,PERL_VERSION,PERL_SUBVERSION)
#endif
#ifndef PERL_VERSION_GE
#  define PERL_VERSION_GE(r,v,s) \
	  (PERL_DECIMAL_VERSION >= PERL_VERSION_DECIMAL(r,v,s))
#endif
#ifndef PERL_VERSION_LE
#  define PERL_VERSION_LE(r,v,s) \
	  (PERL_DECIMAL_VERSION <= PERL_VERSION_DECIMAL(r,v,s))
#endif

/* XS_INTERNAL is the explicit static-linkage variant of the default
 * XS macro.
 *
 * XS_EXTERNAL is the same as XS_INTERNAL except it does not include
 * "STATIC", ie. it exports XSUB symbols. You probably don't want that
 * for anything but the BOOT XSUB.
 *
 * See XSUB.h in core!
 */


/* TODO: This might be compatible further back than 5.10.0. */
#if PERL_VERSION_GE(5, 10, 0) && PERL_VERSION_LE(5, 15, 1)
#  undef XS_EXTERNAL
#  undef XS_INTERNAL
#  if defined(__CYGWIN__) && defined(USE_DYNAMIC_LOADING)
#    define XS_EXTERNAL(name) __declspec(dllexport) XSPROTO(name)
#    define XS_INTERNAL(name) STATIC XSPROTO(name)
#  endif
#  if defined(__SYMBIAN32__)
#    define XS_EXTERNAL(name) EXPORT_C XSPROTO(name)
#    define XS_INTERNAL(name) EXPORT_C STATIC XSPROTO(name)
#  endif
#  ifndef XS_EXTERNAL
#    if defined(HASATTRIBUTE_UNUSED) && !defined(__cplusplus)
#      define XS_EXTERNAL(name) void name(pTHX_ CV* cv __attribute__unused__)
#      define XS_INTERNAL(name) STATIC void name(pTHX_ CV* cv __attribute__unused__)
#    else
#      ifdef __cplusplus
#        define XS_EXTERNAL(name) extern "C" XSPROTO(name)
#        define XS_INTERNAL(name) static XSPROTO(name)
#      else
#        define XS_EXTERNAL(name) XSPROTO(name)
#        define XS_INTERNAL(name) STATIC XSPROTO(name)
#      endif
#    endif
#  endif
#endif

/* perl >= 5.10.0 && perl <= 5.15.1 */


/* The XS_EXTERNAL macro is used for functions that must not be static
 * like the boot XSUB of a module. If perl didn't have an XS_EXTERNAL
 * macro defined, the best we can do is assume XS is the same.
 * Dito for XS_INTERNAL.
 */
#ifndef XS_EXTERNAL
#  define XS_EXTERNAL(name) XS(name)
#endif
#ifndef XS_INTERNAL
#  define XS_INTERNAL(name) XS(name)
#endif

/* Now, finally, after all this mess, we want an ExtUtils::ParseXS
 * internal macro that we're free to redefine for varying linkage due
 * to the EXPORT_XSUB_SYMBOLS XS keyword. This is internal, use
 * XS_EXTERNAL(name) or XS_INTERNAL(name) in your code if you need to!
 */

#undef XS_EUPXS
#if defined(PERL_EUPXS_ALWAYS_EXPORT)
#  define XS_EUPXS(name) XS_EXTERNAL(name)
#else
   /* default to internal */
#  define XS_EUPXS(name) XS_INTERNAL(name)
#endif

#ifndef PERL_ARGS_ASSERT_CROAK_XS_USAGE
#define PERL_ARGS_ASSERT_CROAK_XS_USAGE assert(cv); assert(params)

/* prototype to pass -Wmissing-prototypes */
STATIC void
S_croak_xs_usage(const CV *const cv, const char *const params);

STATIC void
S_croak_xs_usage(const CV *const cv, const char *const params)
{
    const GV *const gv = CvGV(cv);

    PERL_ARGS_ASSERT_CROAK_XS_USAGE;

    if (gv) {
        const char *const gvname = GvNAME(gv);
        const HV *const stash = GvSTASH(gv);
        const char *const hvname = stash ? HvNAME(stash) : NULL;

        if (hvname)
	    Perl_croak_nocontext("Usage: %s::%s(%s)", hvname, gvname, params);
        else
	    Perl_croak_nocontext("Usage: %s(%s)", gvname, params);
    } else {
        /* Pants. I don't think that it should be possible to get here. */
	Perl_croak_nocontext("Usage: CODE(0x%" UVxf ")(%s)", PTR2UV(cv), params);
    }
}
#undef  PERL_ARGS_ASSERT_CROAK_XS_USAGE

#define croak_xs_usage        S_croak_xs_usage

#endif

/* NOTE: the prototype of newXSproto() is different in versions of perls,
 * so we define a portable version of newXSproto()
 */
#ifdef newXS_flags
#define newXSproto_portable(name, c_impl, file, proto) newXS_flags(name, c_impl, file, proto, 0)
#else
#define newXSproto_portable(name, c_impl, file, proto) (PL_Sv=(SV*)newXS(name, c_impl, file), sv_setpv(PL_Sv, proto), (CV*)PL_Sv)
#endif /* !defined(newXS_flags) */

#if PERL_VERSION_LE(5, 21, 5)
#  define newXS_deffile(a,b) Perl_newXS(aTHX_ a,b,file)
#else
#  define newXS_deffile(a,b) Perl_newXS_deffile(aTHX_ a,b)
#endif

#line 405 "Opcode.c"

XS_EUPXS(XS_Opcode__safe_pkg_prep); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode__safe_pkg_prep)
{
    dVAR; dXSARGS;
    if (items != 1)
       croak_xs_usage(cv,  "Package");
    PERL_UNUSED_VAR(ax); /* -Wall */
    SP -= items;
    {
	SV *	Package = ST(0)
;
#line 269 "Opcode.xs"
    HV *hv; 
    char *hvname;
    ENTER;

    hv = gv_stashsv(Package, GV_ADDWARN); /* should exist already	*/

    hvname = HvNAME_get(hv);
    if (!hvname || strNE(hvname, "main")) {
        /* make it think it's in main:: */
	hv_name_set(hv, "main", 4, 0);
        (void) hv_store(hv,"_",1,(SV *)PL_defgv,0);  /* connect _ to global */
        SvREFCNT_inc((SV *)PL_defgv);  /* want to keep _ around! */
    }
    LEAVE;
#line 433 "Opcode.c"
	PUTBACK;
	return;
    }
}


XS_EUPXS(XS_Opcode__safe_call_sv); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode__safe_call_sv)
{
    dVAR; dXSARGS;
    if (items != 3)
       croak_xs_usage(cv,  "Package, mask, codesv");
    PERL_UNUSED_VAR(ax); /* -Wall */
    SP -= items;
    {
	SV *	Package = ST(0)
;
	SV *	mask = ST(1)
;
	SV *	codesv = ST(2)
;
#line 294 "Opcode.xs"
    char op_mask_buf[OP_MASK_BUF_SIZE];
    GV *gv;
    HV *dummy_hv;

    ENTER;

    opmask_addlocal(aTHX_ mask, op_mask_buf);

    save_aptr(&PL_endav);
    PL_endav = (AV*)sv_2mortal((SV*)newAV()); /* ignore END blocks for now	*/

    save_hptr(&PL_defstash);		/* save current default stash	*/
    /* the assignment to global defstash changes our sense of 'main'	*/
    PL_defstash = gv_stashsv(Package, GV_ADDWARN); /* should exist already	*/

    SAVEGENERICSV(PL_curstash);
    PL_curstash = (HV *)SvREFCNT_inc_simple(PL_defstash);

    /* defstash must itself contain a main:: so we'll add that now	*/
    /* take care with the ref counts (was cause of long standing bug)	*/
    /* XXX I'm still not sure if this is right, GV_ADDWARN should warn!	*/
    gv = gv_fetchpvs("main::", GV_ADDWARN, SVt_PVHV);
    sv_free((SV*)GvHV(gv));
    GvHV(gv) = (HV*)SvREFCNT_inc(PL_defstash);

    /* %INC must be clean for use/require in compartment */
    dummy_hv = save_hash(PL_incgv);
    GvHV(PL_incgv) = (HV*)SvREFCNT_inc(GvHV(gv_HVadd(gv_fetchpvs("INC",GV_ADD,SVt_PVHV))));

    /* Invalidate class and method caches */
    ++PL_sub_generation;
    hv_clear(PL_stashcache);

    PUSHMARK(SP);
    /* use caller’s context */
    perl_call_sv(codesv, GIMME_V|G_EVAL|G_KEEPERR);
    sv_free( (SV *) dummy_hv);  /* get rid of what save_hash gave us*/
    SPAGAIN; /* for the PUTBACK added by xsubpp */
    LEAVE;

    /* Invalidate again */
    ++PL_sub_generation;
    hv_clear(PL_stashcache);
#line 499 "Opcode.c"
	PUTBACK;
	return;
    }
}


XS_EUPXS(XS_Opcode_verify_opset); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_verify_opset)
{
    dVAR; dXSARGS;
    if (items < 1 || items > 2)
       croak_xs_usage(cv,  "opset, fatal = 0");
    {
	SV *	opset = ST(0)
;
	int	fatal;
	int	RETVAL;
	dXSTARG;

	if (items < 2)
	    fatal = 0;
	else {
	    fatal = (int)SvIV(ST(1))
;
	}
#line 344 "Opcode.xs"
    RETVAL = verify_opset(aTHX_ opset,fatal);
#line 527 "Opcode.c"
	XSprePUSH; PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}


XS_EUPXS(XS_Opcode_invert_opset); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_invert_opset)
{
    dVAR; dXSARGS;
    if (items != 1)
       croak_xs_usage(cv,  "opset");
    {
	SV *	opset = ST(0)
;
#line 352 "Opcode.xs"
    {
    char *bitmap;
    dMY_CXT;
    STRLEN len = opset_len;

    opset = sv_2mortal(new_opset(aTHX_ opset));	/* verify and clone opset */
    bitmap = SvPVX(opset);
    while(len-- > 0)
	bitmap[len] = ~bitmap[len];
    /* take care of extra bits beyond PL_maxo in last byte	*/
    if (PL_maxo & 07)
	bitmap[opset_len-1] &= ~(0xFF << (PL_maxo & 0x07));
    }
    ST(0) = opset;
#line 558 "Opcode.c"
    }
    XSRETURN(1);
}


XS_EUPXS(XS_Opcode_opset_to_ops); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_opset_to_ops)
{
    dVAR; dXSARGS;
    if (items < 1 || items > 2)
       croak_xs_usage(cv,  "opset, desc = 0");
    PERL_UNUSED_VAR(ax); /* -Wall */
    SP -= items;
    {
	SV *	opset = ST(0)
;
	int	desc;

	if (items < 2)
	    desc = 0;
	else {
	    desc = (int)SvIV(ST(1))
;
	}
#line 373 "Opcode.xs"
    {
    STRLEN len;
    int i, j, myopcode;
    const char * const bitmap = SvPV(opset, len);
    char **names = (desc) ? get_op_descs() : get_op_names();
    dMY_CXT;

    verify_opset(aTHX_ opset,1);
    for (myopcode=0, i=0; i < opset_len; i++) {
	const U16 bits = bitmap[i];
	for (j=0; j < 8 && myopcode < PL_maxo; j++, myopcode++) {
	    if ( bits & (1 << j) )
		XPUSHs(newSVpvn_flags(names[myopcode], strlen(names[myopcode]),
				      SVs_TEMP));
	}
    }
    }
#line 601 "Opcode.c"
	PUTBACK;
	return;
    }
}


XS_EUPXS(XS_Opcode_opset); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_opset)
{
    dVAR; dXSARGS;
    PERL_UNUSED_VAR(cv); /* -W */
    PERL_UNUSED_VAR(items); /* -W */
    {
#line 395 "Opcode.xs"
    int i;
    SV *bitspec;
    STRLEN len, on;

    SV * const opset = sv_2mortal(new_opset(aTHX_ Nullsv));
    char * const bitmap = SvPVX(opset);
    for (i = 0; i < items; i++) {
	const char *opname;
	on = 1;
	if (verify_opset(aTHX_ ST(i),0)) {
	    opname = "(opset)";
	    bitspec = ST(i);
	}
	else {
	    opname = SvPV(ST(i), len);
	    if (*opname == '!') { on=0; ++opname;--len; }
	    bitspec = get_op_bitspec(aTHX_ opname, len, 1);
	}
	set_opset_bits(aTHX_ bitmap, bitspec, on, opname);
    }
    ST(0) = opset;
#line 637 "Opcode.c"
    }
    XSRETURN(1);
}

#define PERMITING  (ix == 0 || ix == 1)
#define ONLY_THESE (ix == 0 || ix == 2)

XS_EUPXS(XS_Opcode_permit_only); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_permit_only)
{
    dVAR; dXSARGS;
    dXSI32;
    if (items < 1)
       croak_xs_usage(cv,  "safe, ...");
    {
	SV *	safe = ST(0)
;
#line 429 "Opcode.xs"
    int i;
    SV *bitspec, *mask;
    char *bitmap;
    STRLEN len;
    dMY_CXT;

    if (!SvROK(safe) || !SvOBJECT(SvRV(safe)) || SvTYPE(SvRV(safe))!=SVt_PVHV)
	croak("Not a Safe object");
    mask = *hv_fetchs((HV*)SvRV(safe), "Mask", 1);
    if (ONLY_THESE)	/* *_only = new mask, else edit current	*/
	sv_setsv(mask, sv_2mortal(new_opset(aTHX_ PERMITING ? opset_all : Nullsv)));
    else
	verify_opset(aTHX_ mask,1); /* croaks */
    bitmap = SvPVX(mask);
    for (i = 1; i < items; i++) {
	const char *opname;
	int on = PERMITING ? 0 : 1;		/* deny = mask bit on	*/
	if (verify_opset(aTHX_ ST(i),0)) {	/* it's a valid mask	*/
	    opname = "(opset)";
	    bitspec = ST(i);
	}
	else {				/* it's an opname/optag	*/
	    opname = SvPV(ST(i), len);
	    /* invert if op has ! prefix (only one allowed)	*/
	    if (*opname == '!') { on = !on; ++opname; --len; }
	    bitspec = get_op_bitspec(aTHX_ opname, len, 1); /* croaks */
	}
	set_opset_bits(aTHX_ bitmap, bitspec, on, opname);
    }
    ST(0) = &PL_sv_yes;
#line 686 "Opcode.c"
    }
    XSRETURN(1);
}


XS_EUPXS(XS_Opcode_opdesc); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_opdesc)
{
    dVAR; dXSARGS;
    PERL_UNUSED_VAR(cv); /* -W */
    PERL_UNUSED_VAR(items); /* -W */
    PERL_UNUSED_VAR(ax); /* -Wall */
    SP -= items;
    {
#line 465 "Opcode.xs"
    int i;
    STRLEN len;
    SV **args;
    char **op_desc = get_op_descs(); 
    dMY_CXT;

    /* copy args to a scratch area since we may push output values onto	*/
    /* the stack faster than we read values off it if masks are used.	*/
    args = (SV**)SvPVX(newSVpvn_flags((char*)&ST(0), items*sizeof(SV*), SVs_TEMP));
    for (i = 0; i < items; i++) {
	const char * const opname = SvPV(args[i], len);
	SV *bitspec = get_op_bitspec(aTHX_ opname, len, 1);
	if (SvIOK(bitspec)) {
	    const int myopcode = SvIV(bitspec);
	    if (myopcode < 0 || myopcode >= PL_maxo)
		croak("panic: opcode %d (%s) out of range",myopcode,opname);
	    XPUSHs(newSVpvn_flags(op_desc[myopcode], strlen(op_desc[myopcode]),
				  SVs_TEMP));
	}
	else if (SvPOK(bitspec) && SvCUR(bitspec) == (STRLEN)opset_len) {
	    int b, j;
	    const char * const bitmap = SvPV_nolen_const(bitspec);
	    int myopcode = 0;
	    for (b=0; b < opset_len; b++) {
		const U16 bits = bitmap[b];
		for (j=0; j < 8 && myopcode < PL_maxo; j++, myopcode++)
		    if (bits & (1 << j))
			XPUSHs(newSVpvn_flags(op_desc[myopcode],
					      strlen(op_desc[myopcode]),
					      SVs_TEMP));
	    }
	}
	else
	    croak("panic: invalid bitspec for \"%s\" (type %u)",
		opname, (unsigned)SvTYPE(bitspec));
    }
#line 738 "Opcode.c"
	PUTBACK;
	return;
    }
}


XS_EUPXS(XS_Opcode_define_optag); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_define_optag)
{
    dVAR; dXSARGS;
    if (items != 2)
       croak_xs_usage(cv,  "optagsv, mask");
    {
	SV *	optagsv = ST(0)
;
	SV *	mask = ST(1)
;
#line 508 "Opcode.xs"
    STRLEN len;
    const char *optag = SvPV(optagsv, len);

    put_op_bitspec(aTHX_ optag, len, mask); /* croaks */
    ST(0) = &PL_sv_yes;
#line 762 "Opcode.c"
    }
    XSRETURN(1);
}


XS_EUPXS(XS_Opcode_empty_opset); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_empty_opset)
{
    dVAR; dXSARGS;
    if (items != 0)
       croak_xs_usage(cv,  "");
    {
#line 518 "Opcode.xs"
    ST(0) = sv_2mortal(new_opset(aTHX_ Nullsv));
#line 777 "Opcode.c"
    }
    XSRETURN(1);
}


XS_EUPXS(XS_Opcode_full_opset); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_full_opset)
{
    dVAR; dXSARGS;
    if (items != 0)
       croak_xs_usage(cv,  "");
    {
#line 523 "Opcode.xs"
    dMY_CXT;
    ST(0) = sv_2mortal(new_opset(aTHX_ opset_all));
#line 793 "Opcode.c"
    }
    XSRETURN(1);
}


XS_EUPXS(XS_Opcode_opmask_add); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_opmask_add)
{
    dVAR; dXSARGS;
    if (items != 1)
       croak_xs_usage(cv,  "opset");
    {
	SV *	opset = ST(0)
;
#line 530 "Opcode.xs"
    if (!PL_op_mask)
	Newxz(PL_op_mask, PL_maxo, char);
#line 811 "Opcode.c"
#line 533 "Opcode.xs"
    opmask_add(aTHX_ opset);
#line 814 "Opcode.c"
    }
    XSRETURN_EMPTY;
}


XS_EUPXS(XS_Opcode_opcodes); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_opcodes)
{
    dVAR; dXSARGS;
    if (items != 0)
       croak_xs_usage(cv,  "");
    PERL_UNUSED_VAR(ax); /* -Wall */
    SP -= items;
    {
#line 538 "Opcode.xs"
    if (GIMME_V == G_ARRAY) {
	croak("opcodes in list context not yet implemented"); /* XXX */
    }
    else {
	XPUSHs(sv_2mortal(newSViv(PL_maxo)));
    }
#line 836 "Opcode.c"
	PUTBACK;
	return;
    }
}


XS_EUPXS(XS_Opcode_opmask); /* prototype to pass -Wmissing-prototypes */
XS_EUPXS(XS_Opcode_opmask)
{
    dVAR; dXSARGS;
    if (items != 0)
       croak_xs_usage(cv,  "");
    {
#line 548 "Opcode.xs"
    ST(0) = sv_2mortal(new_opset(aTHX_ Nullsv));
    if (PL_op_mask) {
	char * const bitmap = SvPVX(ST(0));
	int myopcode;
	for(myopcode=0; myopcode < PL_maxo; ++myopcode) {
	    if (PL_op_mask[myopcode])
		bitmap[myopcode >> 3] |= 1 << (myopcode & 0x07);
	}
    }
#line 860 "Opcode.c"
    }
    XSRETURN(1);
}

#ifdef __cplusplus
extern "C"
#endif
XS_EXTERNAL(boot_Opcode); /* prototype to pass -Wmissing-prototypes */
XS_EXTERNAL(boot_Opcode)
{
#if PERL_VERSION_LE(5, 21, 5)
    dVAR; dXSARGS;
#else
    dVAR; dXSBOOTARGSXSAPIVERCHK;
#endif
#if (PERL_REVISION == 5 && PERL_VERSION < 9)
    char* file = __FILE__;
#else
    const char* file = __FILE__;
#endif

    PERL_UNUSED_VAR(file);

    PERL_UNUSED_VAR(cv); /* -W */
    PERL_UNUSED_VAR(items); /* -W */
#if PERL_VERSION_LE(5, 21, 5)
    XS_VERSION_BOOTCHECK;
#  ifdef XS_APIVERSION_BOOTCHECK
    XS_APIVERSION_BOOTCHECK;
#  endif
#endif

        (void)newXSproto_portable("Opcode::_safe_pkg_prep", XS_Opcode__safe_pkg_prep, file, "$");
        (void)newXSproto_portable("Opcode::_safe_call_sv", XS_Opcode__safe_call_sv, file, "$$$");
        (void)newXSproto_portable("Opcode::verify_opset", XS_Opcode_verify_opset, file, "$;$");
        (void)newXSproto_portable("Opcode::invert_opset", XS_Opcode_invert_opset, file, "$");
        (void)newXSproto_portable("Opcode::opset_to_ops", XS_Opcode_opset_to_ops, file, "$;$");
        (void)newXSproto_portable("Opcode::opset", XS_Opcode_opset, file, ";@");
        cv = newXSproto_portable("Opcode::deny", XS_Opcode_permit_only, file, "$;@");
        XSANY.any_i32 = 3;
        cv = newXSproto_portable("Opcode::deny_only", XS_Opcode_permit_only, file, "$;@");
        XSANY.any_i32 = 2;
        cv = newXSproto_portable("Opcode::permit", XS_Opcode_permit_only, file, "$;@");
        XSANY.any_i32 = 1;
        cv = newXSproto_portable("Opcode::permit_only", XS_Opcode_permit_only, file, "$;@");
        XSANY.any_i32 = 0;
        (void)newXSproto_portable("Opcode::opdesc", XS_Opcode_opdesc, file, ";@");
        (void)newXSproto_portable("Opcode::define_optag", XS_Opcode_define_optag, file, "$$");
        (void)newXSproto_portable("Opcode::empty_opset", XS_Opcode_empty_opset, file, "");
        (void)newXSproto_portable("Opcode::full_opset", XS_Opcode_full_opset, file, "");
        (void)newXSproto_portable("Opcode::opmask_add", XS_Opcode_opmask_add, file, "$");
        (void)newXSproto_portable("Opcode::opcodes", XS_Opcode_opcodes, file, "");
        (void)newXSproto_portable("Opcode::opmask", XS_Opcode_opmask, file, "");

    /* Initialisation Section */

#line 256 "Opcode.xs"
{
    MY_CXT_INIT;
    STATIC_ASSERT_STMT(PL_maxo < OP_MASK_BUF_SIZE);
    opset_len = (PL_maxo + 7) / 8;
    if (opcode_debug >= 1)
	warn("opset_len %ld\n", (long)opset_len);
    op_names_init(aTHX);
}

#line 927 "Opcode.c"

    /* End of Initialisation Section */

#if PERL_VERSION_LE(5, 21, 5)
#  if PERL_VERSION_GE(5, 9, 0)
    if (PL_unitcheckav)
        call_list(PL_scopestack_ix, PL_unitcheckav);
#  endif
    XSRETURN_YES;
#else
    Perl_xs_boot_epilog(aTHX_ ax);
#endif
}

