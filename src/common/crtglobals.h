// The CRT's own data; each name is a comment because the decorated names are not identifiers.

#ifndef HYPERBLADE_CRTGLOBALS_H
#define HYPERBLADE_CRTGLOBALS_H

// GLOBAL: HYPERX1001 0x004ca590
// ___nullstring

// GLOBAL: HYPERX1001 0x004ca594
// ___wnullstring

// Undecorated because cdjpeg.c includes <ctype.h>, which declares both.
// GLOBAL: HYPERX1001 0x004ca5fc
// _pctype

// The table _pctype points one element into.
// GLOBAL: HYPERX1001 0x004ca604
// __ctype

// GLOBAL: HYPERX1001 0x004ca99c
// __mb_cur_max

// GLOBAL: HYPERX1001 0x004ca9a0
// ___decimal_point

// GLOBAL: HYPERX1001 0x004caa88
// __d_inf

// GLOBAL: HYPERX1001 0x004caa90
// __d_ind

// GLOBAL: HYPERX1001 0x004caa98
// __d_max

// GLOBAL: HYPERX1001 0x004caaa8
// __d_mzero

// GLOBAL: HYPERX1001 0x004cafa8
// __matherr_flag

// GLOBAL: HYPERX1001 0x004d2c10
// ?_pnhHeap@@3P6AHI@ZA

// GLOBAL: HYPERX1001 0x004d2c34
// __environ

// GLOBAL: HYPERX1001 0x004d2c38
// ___initenv

// GLOBAL: HYPERX1001 0x004d2c3c
// __wenviron

// GLOBAL: HYPERX1001 0x004d2c70
// __cflush

// GLOBAL: HYPERX1001 0x004d2d2c
// __fmode

// GLOBAL: HYPERX1001 0x004d2d48
// ___lc_codepage

// GLOBAL: HYPERX1001 0x004d2f30
// __commode

// GLOBAL: HYPERX1001 0x004d2fc8
// ___setlc_active

// GLOBAL: HYPERX1001 0x004d2fe0
// ___pioinfo

// GLOBAL: HYPERX1001 0x004d30fc
// __crtheap

// GLOBAL: HYPERX1001 0x004d3324
// ___mblcid

// GLOBAL: HYPERX1001 0x004d3328
// __acmdln

// GLOBAL: HYPERX1001 0x004d332c
// ___piob

// GLOBAL: HYPERX1001 0x004d4340
// __nstream

// GLOBAL: HYPERX1001 0x004d4344
// ___env_initialized

// GLOBAL: HYPERX1001 0x004d4348
// ___mbctype_initialized

// GLOBAL: HYPERX1001 0x004d434c
// ___onexitend

// GLOBAL: HYPERX1001 0x004ca348
// ___app_type

// The exit _amsg_exit takes, _exit unless something replaces it.
// GLOBAL: HYPERX1001 0x004ca344
// __aexit_rtn

// GLOBAL: HYPERX1001 0x004ca528
// ___tlsindex

// GLOBAL: HYPERX1001 0x004ca578
// ___sbh_threshold

// GLOBAL: HYPERX1001 0x004d2c14
// __umaskval

// GLOBAL: HYPERX1001 0x004d2c18
// __osver

// GLOBAL: HYPERX1001 0x004d2c1c
// __winver

// GLOBAL: HYPERX1001 0x004d2c20
// __winmajor

// GLOBAL: HYPERX1001 0x004d2c24
// __winminor

// GLOBAL: HYPERX1001 0x004d2d1c
// __newmode

// GLOBAL: HYPERX1001 0x004d30e0
// __nhandle

// GLOBAL: HYPERX1001 0x004d3100
// ___mbcodepage

// GLOBAL: HYPERX1001 0x004d311c
// ___ismbcodepage

// GLOBAL: HYPERX1001 0x004d4350
// ___onexitbegin

// GLOBAL: HYPERX1001 0x004cad08
// __First_FPE_Indx

// GLOBAL: HYPERX1001 0x004cad0c
// __Num_FPE

// GLOBAL: HYPERX1001 0x004d30e4
// ___sbh_sizeHeaderList

// GLOBAL: HYPERX1001 0x004d30e8
// ___sbh_indGroupDefer

// GLOBAL: HYPERX1001 0x004d30ec
// ___sbh_pHeaderScan

// GLOBAL: HYPERX1001 0x004d30f0
// ___sbh_pHeaderDefer

// GLOBAL: HYPERX1001 0x004d30f4
// ___sbh_cntHeaderList

// GLOBAL: HYPERX1001 0x004d30f8
// ___sbh_pHeaderList

// Named _iob because jdatasrc.c and the error hooks include <stdio.h>, which declares it.
// GLOBAL: HYPERX1001 0x004ca0c0
// _iob

// GLOBAL: HYPERX1001 0x004ca978
// ___badioinfo

// GLOBAL: HYPERX1001 0x004cac90
// __XcptActTab

// GLOBAL: HYPERX1001 0x004d2fcc
// ___unguarded_readlc_active

// GLOBAL: HYPERX1001 0x004ca350
// __locktable

// GLOBAL: HYPERX1001 0x004d2c80
// ___error_mode

// GLOBAL: HYPERX1001 0x004d2f2c
// __adbgmsg

// GLOBAL: HYPERX1001 0x004ca058
// __NLG_Destination

// The three static stream buffers _stbuf hands out.
// GLOBAL: HYPERX1001 0x004d2d5c
// __stdbuf

// 0x004b48f0, ___lookuptable, is deliberately left without a marker.

// The six function pointers _cfltcvt_init fills; only the base is named.
// GLOBAL: HYPERX1001 0x004ca510
// __cfltcvt_tab

// GLOBAL: HYPERX1001 0x004ca570
// ?__pInconsistency@@3P6AXXZA

// GLOBAL: HYPERX1001 0x004cad10
// __XcptActTabSize

// The row count beside the exception-action table.
// GLOBAL: HYPERX1001 0x004cad14
// __XcptActTabCount

// GLOBAL: HYPERX1001 0x004cb0a8
// __pow10pos

// GLOBAL: HYPERX1001 0x004cb208
// __pow10neg

// GLOBAL: HYPERX1001 0x004d2c0c
// __adjust_fdiv

// GLOBAL: HYPERX1001 0x004d2c28
// ___argc

// GLOBAL: HYPERX1001 0x004d2c2c
// ___argv

// GLOBAL: HYPERX1001 0x004d2c44
// __pgmptr

// GLOBAL: HYPERX1001 0x004d2c4c
// __exitflag

// GLOBAL: HYPERX1001 0x004d2c50
// __C_Termination_Done

// GLOBAL: HYPERX1001 0x004d2c54
// __C_Exit_Done

// GLOBAL: HYPERX1001 0x004d2c78
// __aenvptr

// GLOBAL: HYPERX1001 0x004d3110
// ___mbulinfo

// The single-byte case-fold table _mbslwr and _mbsicmp index as a bare displacement.
// GLOBAL: HYPERX1001 0x004d3120
// __mbcasemap

// The base, not an interior: _strcmpi takes its address and tests element two.
// GLOBAL: HYPERX1001 0x004d2d30
// ___lc_handle

// type_info's vtable and RTTI descriptor, read as bare addresses.
// VTABLE: HYPERX1001 0x004b4790
// class type_info

// GLOBAL: HYPERX1001 0x004ca080
// ??_R0?AVtype_info@@@8

// _cinit's four arguments and the pointer it tests first.
// GLOBAL: HYPERX1001 0x004ca03c
// __FPinit

// GLOBAL: HYPERX1001 0x004bb000
// ___xc_a

// GLOBAL: HYPERX1001 0x004bb230
// ___xc_z

// GLOBAL: HYPERX1001 0x004bb234
// ___xi_a

// GLOBAL: HYPERX1001 0x004bb248
// ___xi_z

// The pre-termination and termination tables doexit runs.
// GLOBAL: HYPERX1001 0x004bb24c
// ___xp_a

// GLOBAL: HYPERX1001 0x004bb258
// ___xp_z

// GLOBAL: HYPERX1001 0x004bb25c
// ___xt_a

// GLOBAL: HYPERX1001 0x004bb264
// ___xt_z

// ___loctotime_t's time-zone values: seconds west of UTC, whether summer time applies, its offset.
// GLOBAL: HYPERX1001 0x004cabdc
// __timezone

// GLOBAL: HYPERX1001 0x004cabe0
// __daylight

// GLOBAL: HYPERX1001 0x004cabe4
// __dstbias

// The two time-zone name pointers.
// GLOBAL: HYPERX1001 0x004cac68
// __tzname

// The cumulative day counts at each month start, for leap and common years.
// GLOBAL: HYPERX1001 0x004caf40
// __lpdays

// GLOBAL: HYPERX1001 0x004caf74
// __days

// The multibyte character-class table. Its lead-byte flags start one byte in.
// GLOBAL: HYPERX1001 0x004d3220
// __mbctype

// 0x004cada8 is the end bound of the exception-message table at 0x004cad18, not __infinity.

#endif
