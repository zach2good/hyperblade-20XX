// C runtime functions the original links from LIBCMT.LIB, named so calls to them resolve.

#ifndef HYPERBLADE_LIBRARY_H
#define HYPERBLADE_LIBRARY_H

// operator delete. It tail-calls free, which sits eleven bytes later.
// LIBRARY: HYPERX 0x0049f052
// ??3@YAXPAX@Z

// LIBRARY: HYPERX 0x0049f05d
// _free

// LIBRARY: HYPERX 0x0049eb54
// __ftol

// LIBRARY: HYPERX 0x004a10f3
// _floor

// _mbschr at 0x0049f918 delegates to the assembly strchr at 0x004a6430 on a single-byte code page.
// LIBRARY: HYPERX 0x004a6430
// _strchr

// LIBRARY: HYPERX 0x0049f918
// __mbschr

// _mbsrchr at 0x0049f8a6 delegates to the assembly strrchr at 0x004a17f0 on a single-byte page.
// LIBRARY: HYPERX 0x004a17f0
// _strrchr

// LIBRARY: HYPERX 0x0049f8a6
// __mbsrchr

// _mbsstr at 0x0049f75c delegates to strstr at 0x004a63a0 on a single-byte code page.
// LIBRARY: HYPERX 0x0049f75c
// __mbsstr

// LIBRARY: HYPERX 0x004a63a0
// _strstr

// The one-character tail strstr jumps into rather than calls.
// LIBRARY: HYPERX 0x004a6436
// ___from_strstr_to_strchr

// __findfirst, __findclose and their FILETIME converters; 0x0049f9b0 is strpbrk, 0x0049f7d2 _mbspbrk.
// LIBRARY: HYPERX 0x004a108f
// ___timet_from_ft

// LIBRARY: HYPERX 0x004a7f60
// ___loctotime_t

// LIBRARY: HYPERX 0x004a0fa2
// __findfirst

// LIBRARY: HYPERX 0x004a106f
// __findclose

// LIBRARY: HYPERX 0x0049f7d2
// __mbspbrk

// LIBRARY: HYPERX 0x0049f9b0
// _strpbrk

// LIBRARY: HYPERX 0x0049fb0a
// _sprintf

// 0x0049eb7b is srand, not an errno setter: its caller seeds it from 0x004a16bb, then calls rand.
// LIBRARY: HYPERX 0x004a16bb
// _time

// LIBRARY: HYPERX 0x0049eb7b
// _srand

// fgetc, not getc: getc is a macro in this library and has no symbol.
// LIBRARY: HYPERX 0x0049fd6a
// _fgetc

// The out-of-line getc, which stays a call because _MT undefines the macro.
// LIBRARY: HYPERX 0x0049fd9b
// _getc

// LIBRARY: HYPERX 0x0049eb88
// _rand

// LIBRARY: HYPERX 0x0049f2bb
// ??2@YAPAXI@Z

// _set_new_handler; new.h declares it without extern "C", so the name is C++ mangled.
// LIBRARY: HYPERX 0x0049f867
// ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z

// LIBRARY: HYPERX 0x0049f1af
// _atexit

// LIBRARY: HYPERX 0x0049fc56
// _fclose

// The JPEG library's temporary file backing store uses these five.
// LIBRARY: HYPERX 0x004a2eae
// _tmpfile

// LIBRARY: HYPERX 0x004a302b
// _fseek

// LIBRARY: HYPERX 0x004a034f
// _fread

// LIBRARY: HYPERX 0x004a23ae
// _fwrite

// LIBRARY: HYPERX 0x004a2db0
// _strncpy

// The memory manager reads its ceiling from JPEGMEM when the environment sets it.
// LIBRARY: HYPERX 0x004a2d05
// _getenv

// The bitmap loader tries two extensions and asks this which one is there.
// LIBRARY: HYPERX 0x004a1675
// __access

// _mbctolower: unlike tolower at 0x004a288a it folds values above 0xff through __crtLCMapStringA.
// LIBRARY: HYPERX 0x0049fbdb
// __mbctolower

// The string formatter's six; it walks the format with the multibyte routines, so the build is _MBCS.
// LIBRARY: HYPERX 0x004a131a
// __mbsinc

// LIBRARY: HYPERX 0x004a11c2
// __mbclen

// LIBRARY: HYPERX 0x004a11f5
// __ismbcdigit

// LIBRARY: HYPERX 0x004a130f
// _atoi

// LIBRARY: HYPERX 0x004a11d8
// _wcslen

// LIBRARY: HYPERX 0x004a0cbc
// _vsprintf

// The bitmap reader shrinks its block to the size the header asks for.
// LIBRARY: HYPERX 0x004a04f6
// _realloc

// LIBRARY: HYPERX 0x0049fda6
// _fscanf

// LIBRARY: HYPERX 0x0049fe04
// _fopen

// _stricmp is an alias that links as _strcmpi; 0x0049f2d2 is _mbsicmp and 0x004a1820 is _strcmpi.
// LIBRARY: HYPERX 0x0049f2d2
// __mbsicmp

// _mbscmp, which delegates to strcmp on a single-byte code page.
// LIBRARY: HYPERX 0x0049ea56
// __mbscmp

// The assembly _strcmpi, not the C in STRICMP.C.
// LIBRARY: HYPERX 0x004a1820
// __strcmpi

// The library's block copies land on the shared memcpy/memmove body.
// LIBRARY: HYPERX 0x004a29d0
// _memcpy

// LIBRARY: HYPERX 0x004a21f0
// _memset

// LIBRARY: HYPERX 0x0049f5c8
// _sscanf

// _mbsnbicmp at 0x0049f467 delegates to _strnicmp at 0x004a5680 on a single-byte code page.
// LIBRARY: HYPERX 0x0049f467
// __mbsnbicmp

// LIBRARY: HYPERX 0x004a5680
// __strnicmp

// The vector constructor iterator, for an array of a class with a constructor and a destructor.
// LIBRARY: HYPERX 0x0049f5fc
// ??_L@YGXPAXIHP6EX0@Z1@Z

// Its destructor counterpart, emitted for `delete[]`.
// LIBRARY: HYPERX 0x0049f67e
// ??_M@YGXPAXIHP6EX0@Z@Z

// The low level file routines the model loader uses directly rather than through stdio.
// LIBRARY: HYPERX 0x0049f0a5
// _malloc

// LIBRARY: HYPERX 0x004a062e
// __close

// LIBRARY: HYPERX 0x004a07e6
// __open

// LIBRARY: HYPERX 0x004a070e
// __lseek

// LIBRARY: HYPERX 0x004a0d0d
// __read

// _ismbcprint, which the text input widget filters with.
// LIBRARY: HYPERX 0x004a00fa
// __ismbcprint

// The locking wrappers the buffered file calls go through.
// LIBRARY: HYPERX 0x004a1e75
// __lock_file

// LIBRARY: HYPERX 0x0049fc87
// __fclose_lk

// LIBRARY: HYPERX 0x004a1ec7
// __unlock_file

// LIBRARY: HYPERX 0x004a5781
// __input

// LIBRARY: HYPERX 0x004a6240
// _strlen

// LIBRARY: HYPERX 0x004a3580
// _strcmp

// The small-block heap the free path walks.
// LIBRARY: HYPERX 0x004a48dd
// ___sbh_find_block

// LIBRARY: HYPERX 0x004a4908
// ___sbh_free_block

// LIBRARY: HYPERX 0x004a6604
// __output

// LIBRARY: HYPERX 0x004a64ec
// __flsbuf

// The file-handle locking pair, and the two error slots every wrapper reports through.
// LIBRARY: HYPERX 0x004a7a13
// __lock_fhandle

// LIBRARY: HYPERX 0x004a068b
// __close_lk

// LIBRARY: HYPERX 0x004a7a72
// __unlock_fhandle

// LIBRARY: HYPERX 0x004a77a1
// __errno

// LIBRARY: HYPERX 0x004a77aa
// ___doserrno

// LIBRARY: HYPERX 0x0049fa17
// _exit

// LIBRARY: HYPERX 0x004a6e72
// __freebuf

// More CRT helpers the original calls.

// LIBRARY: HYPERX 0x0049f0b7
// __nh_malloc

// LIBRARY: HYPERX 0x0049f131
// __onexit

// LIBRARY: HYPERX 0x0049f88b
// __callnewh

// _cinit: calls _FPinit when set, then runs _initterm over the __xi and __xc ranges.
// LIBRARY: HYPERX 0x0049f9ea
// __cinit

// LIBRARY: HYPERX 0x0049fdd3
// __fsopen

// LIBRARY: HYPERX 0x004a037e
// __fread_lk

// LIBRARY: HYPERX 0x004a0773
// __lseek_lk

// LIBRARY: HYPERX 0x004a07fd
// __sopen

// LIBRARY: HYPERX 0x004a0acc
// __write

// LIBRARY: HYPERX 0x004a0d72
// __read_lk

// LIBRARY: HYPERX 0x004a1284
// _atol

// LIBRARY: HYPERX 0x004a1340
// _memmove

// LIBRARY: HYPERX 0x004a1f64
// _isspace

// LIBRARY: HYPERX 0x004a23dd
// __fwrite_lk

// LIBRARY: HYPERX 0x004a2d26
// __getenv_lk

// LIBRARY: HYPERX 0x004a3057
// __fseek_lk

// LIBRARY: HYPERX 0x004a3aa3
// __mtinit

// LIBRARY: HYPERX 0x004a3b0a
// __getptd

// LIBRARY: HYPERX 0x004a4863
// __heap_init

// LIBRARY: HYPERX 0x004a4c33
// ___sbh_alloc_block

// LIBRARY: HYPERX 0x004a50e8
// ___sbh_resize_block

// LIBRARY: HYPERX 0x004a6e9d
// __getbuf

// LIBRARY: HYPERX 0x004a712d
// __getstream

// LIBRARY: HYPERX 0x004a759c
// ___crtGetStringTypeA

// LIBRARY: HYPERX 0x004a772e
// __dosmaperr

// LIBRARY: HYPERX 0x004a7952
// __free_osfhnd

// LIBRARY: HYPERX 0x004a79d1
// __get_osfhandle

// LIBRARY: HYPERX 0x004a7a94
// __ioinit

// LIBRARY: HYPERX 0x004a7df4
// __isctype

// LIBRARY: HYPERX 0x004a8022
// __handle_qnan1

// LIBRARY: HYPERX 0x004a8076
// __except1

// LIBRARY: HYPERX 0x004a86da
// __frnd

// LIBRARY: HYPERX 0x004a8715
// __sptype

// LIBRARY: HYPERX 0x004a884d
// __ctrlfp

// LIBRARY: HYPERX 0x004a88d0
// __allmul

// LIBRARY: HYPERX 0x004a9260
// __aullrem

// LIBRARY: HYPERX 0x004a92e0
// __aulldiv

// LIBRARY: HYPERX 0x004a9348
// __XcptFilter

// LIBRARY: HYPERX 0x004a94c0
// __wincmdln

// LIBRARY: HYPERX 0x004a9c1c
// __strdup

// LIBRARY: HYPERX 0x004aa6df
// _mbtowc

// LIBRARY: HYPERX 0x004aa810
// __allshl

// LIBRARY: HYPERX 0x004aa82f
// __isatty

// LIBRARY: HYPERX 0x004aa858
// _wctomb

// LIBRARY: HYPERX 0x0049f0e3
// __heap_alloc

// LIBRARY: HYPERX 0x004a0b31
// __write_lk

// LIBRARY: HYPERX 0x004a1ea4
// __lock_file2

// LIBRARY: HYPERX 0x004a1ef6
// __unlock_file2

// LIBRARY: HYPERX 0x004a30e4
// __mtinitlocks

// LIBRARY: HYPERX 0x004a3af7
// __initptd

// LIBRARY: HYPERX 0x004a489f
// ___sbh_heap_init

// LIBRARY: HYPERX 0x004a4f3c
// ___sbh_alloc_new_region

// LIBRARY: HYPERX 0x004a4fed
// ___sbh_alloc_new_group

// LIBRARY: HYPERX 0x004a6ee1
// __filbuf

// LIBRARY: HYPERX 0x004a6fbd
// __openfile

// LIBRARY: HYPERX 0x004a77b3
// __alloc_osfhnd

// LIBRARY: HYPERX 0x004a78d6
// __set_osfhnd

// LIBRARY: HYPERX 0x004a7c50
// __chsize_lk

// LIBRARY: HYPERX 0x004a7e70
// _strcpy

// LIBRARY: HYPERX 0x004a810e
// __raise_exc

// LIBRARY: HYPERX 0x004a83c1
// __handle_exc

// LIBRARY: HYPERX 0x004a85d8
// __umatherr

// LIBRARY: HYPERX 0x004a8660
// __set_errno

// LIBRARY: HYPERX 0x004a86ad
// __errcode

// LIBRARY: HYPERX 0x004a8b90
// _calloc

// LIBRARY: HYPERX 0x004a9b6f
// __mbsnbicoll

// LIBRARY: HYPERX 0x004a9bae
// ___wtomb_environ

// LIBRARY: HYPERX 0x004a9d31
// __ftell_lk

// LIBRARY: HYPERX 0x004aa6b0
// __chkstk

// LIBRARY: HYPERX 0x004aa73c
// __mbtowc_lk

// LIBRARY: HYPERX 0x004aa8b1
// __wctomb_lk

// LIBRARY: HYPERX 0x004a876f
// __decomp

// LIBRARY: HYPERX 0x004a8830
// __statfp

// LIBRARY: HYPERX 0x004a883e
// __clrfp

// LIBRARY: HYPERX 0x004a8870
// __set_statfp

// LIBRARY: HYPERX 0x004ab73d
// __setmode_lk

// LIBRARY: HYPERX 0x004abc6f
// __matherr

// LIBRARY: HYPERX 0x004abdc3
// ___crtCompareStringA

// LIBRARY: HYPERX 0x004a86ec
// __set_exp

// LIBRARY: HYPERX 0x004a53de
// __msize

// The runtime's own lock and unlock.
// LIBRARY: HYPERX 0x004a310d
// __lock

// LIBRARY: HYPERX 0x004a316e
// __unlock

// The runtime's own fatal-error exit.
// LIBRARY: HYPERX 0x004a21a7
// __amsg_exit

// LIBRARY: HYPERX 0x004a9950
// __FF_MSGBANNER

// LIBRARY: HYPERX 0x004a9989
// __NMSG_WRITE

// LIBRARY: HYPERX 0x004a1f54
// _putc

// The exception-handling and math helpers the runtime supplies, named from this build's PDB.
// LIBRARY: HYPERX 0x0049ecbd
// ?_UnwindNestedFrames@@YGXPAUEHRegistrationNode@@PAUEHExceptionRecord@@@Z

// LIBRARY: HYPERX 0x0049ed0c
// ___CxxFrameHandler

// LIBRARY: HYPERX 0x0049eee6
// ?_GetRangeOfTrysToCheck@@YAPBU_s_TryBlockMapEntry@@PBU_s_FuncInfo@@HHPAI1@Z

// LIBRARY: HYPERX 0x0049ef64
// __global_unwind2

// LIBRARY: HYPERX 0x0049efa6
// __local_unwind2

// LIBRARY: HYPERX 0x004ac956
// _RtlUnwind@16

// LIBRARY: HYPERX 0x004a3f3c
// ___InternalCxxFrameHandler

// A second entry nine bytes into the same body, where __CallSettingFrame@12 enters it.
// LIBRARY: HYPERX 0x0049f031
// __NLG_Notify1

// LIBRARY: HYPERX 0x0049f03a
// __NLG_Notify

// The partial-array unwind the vector constructor iterator's funclet calls when one throws.
// LIBRARY: HYPERX 0x0049f6fe
// ?__ArrayUnwind@@YGXPAXIHP6EX0@Z@Z

// The exit lock pair, which the library's own _onexit calls.
// LIBRARY: HYPERX 0x0049fade
// __lockexit

// LIBRARY: HYPERX 0x0049fae7
// __unlockexit

// The call WinMainCRTStartup makes before __setargv.
// LIBRARY: HYPERX 0x004a981e
// ___crtGetEnvironmentStringsA

// LIBRARY: HYPERX 0x004a480d
// ?_inconsistency@@YAXXZ

// LIBRARY: HYPERX 0x004a62c4
// __except_handler3

// LIBRARY: HYPERX 0x004a4291
// ___FrameUnwindToState

// The unwind trampoline ___FrameUnwindToState calls; it sets the frame pointer to the handler's.
// LIBRARY: HYPERX 0x004a4760
// __CallSettingFrame@12

// LIBRARY: HYPERX 0x004a1f19
// _fputc

// The LCMapString wrapper from A_MAP.C, not _tolower_lk at 0x004a28f9.
// LIBRARY: HYPERX 0x004a5423
// ___crtLCMapStringA

// LIBRARY: HYPERX 0x004a28f9
// __tolower_lk

// LIBRARY: HYPERX 0x004a7e80
// _strcat

// LIBRARY: HYPERX 0x004abd3a
// ___crtMessageBoxA

// The four names it loads USER32 by. LIBCMT supplies the strings.
// STRING: HYPERX 0x004b4d94
// "user32.dll"
// STRING: HYPERX 0x004b4d88
// "MessageBoxA"
// STRING: HYPERX 0x004b4d78
// "GetActiveWindow"
// STRING: HYPERX 0x004b4d64
// "GetLastActivePopup"

// LIBRARY: HYPERX 0x004a9adc
// __commit

// LIBRARY: HYPERX 0x0049ec7b
// ?_JumpToContinuation@@YGXPAXPAUEHRegistrationNode@@@Z

// LIBRARY: HYPERX 0x0049ed42
// ?_CallCatchBlock2@@YAPAXPAUEHRegistrationNode@@PBU_s_FuncInfo@@PAXHK@Z

// LIBRARY: HYPERX 0x0049edbb
// ?_CallSETranslator@@YAHPAUEHExceptionRecord@@PAUEHRegistrationNode@@PAX2PBU_s_FuncInfo@@H1@Z

// LIBRARY: HYPERX 0x0049f00e
// __abnormal_termination

// LIBRARY: HYPERX 0x0049fb85
// ??_Gtype_info@@UAEPAXI@Z

// LIBRARY: HYPERX 0x0049fcfc
// __ungetc_lk
// LIBRARY: HYPERX 0x004a24e7
// _signal

// LIBRARY: HYPERX 0x004a26cb
// _raise

// LIBRARY: HYPERX 0x004a3183
// __setmbcp

// LIBRARY: HYPERX 0x004a373d
// __fassign

// LIBRARY: HYPERX 0x004a399a
// __cftog

// LIBRARY: HYPERX 0x004a3a2d
// __cfltcvt

// LIBRARY: HYPERX 0x004a6381
// __seh_longjmp_unwind@4

// LIBRARY: HYPERX 0x004a6e0e
// ?__CxxUnhandledExceptionFilter@@YGJPAU_EXCEPTION_POINTERS@@@Z

// The strspn entry into the assembly body strspn, strcspn and strpbrk share.
// LIBRARY: HYPERX 0x004a76f0
// _strspn

// LIBRARY: HYPERX 0x004a9e92
// __control87

// LIBRARY: HYPERX 0x004a9ec7
// __controlfp

// LIBRARY: HYPERX 0x004a9ff8
// __ZeroTail

// LIBRARY: HYPERX 0x004aa041
// __IncMan

// LIBRARY: HYPERX 0x004aa097
// __RoundMan

// LIBRARY: HYPERX 0x004aa123
// __CopyMan

// LIBRARY: HYPERX 0x004aa13e
// __FillZeroMan

// LIBRARY: HYPERX 0x004aa14a
// __IsZeroMan

// LIBRARY: HYPERX 0x004aa165
// __ShrMan

// LIBRARY: HYPERX 0x004aa1f2
// __ld12cvt

// LIBRARY: HYPERX 0x004aa3e4
// __fptostr

// LIBRARY: HYPERX 0x004aa45b
// __fltout2

// LIBRARY: HYPERX 0x004aa4b7
// ___dtold

// The default in every _cfltcvt_tab slot until floating point is linked.
// LIBRARY: HYPERX 0x004aa56d
// __fptrap

// LIBRARY: HYPERX 0x004ab79e
// ___strgtold12

// LIBRARY: HYPERX 0x004abcc0
// _strncmp

// LIBRARY: HYPERX 0x004ac286
// ___addl

// LIBRARY: HYPERX 0x004ac2a7
// ___add_12

// LIBRARY: HYPERX 0x004ac305
// ___shl_12

// LIBRARY: HYPERX 0x004ac333
// ___shr_12

// LIBRARY: HYPERX 0x004ac360
// ___mtold12

// LIBRARY: HYPERX 0x004ac427
// _$I10_OUTPUT

// LIBRARY: HYPERX 0x004ac6ba
// ___ld12mul

// LIBRARY: HYPERX 0x0049fa28
// __exit

// LIBRARY: HYPERX 0x0049fb5c
// ??1type_info@@UAE@XZ

// LIBRARY: HYPERX 0x004a377b
// __cftoe

// LIBRARY: HYPERX 0x004a389e
// __cftof

// LIBRARY: HYPERX 0x004aa38a
// __atodbl

// LIBRARY: HYPERX 0x004aa3b7
// __atoflt

// LIBRARY: HYPERX 0x004aa646
// ?_ValidateRead@@YAHPBXI@Z

// LIBRARY: HYPERX 0x004aa662
// ?_ValidateWrite@@YAHPAXI@Z

// LIBRARY: HYPERX 0x004aa67e
// ?_ValidateExecute@@YAHP6GHXZ@Z

// LIBRARY: HYPERX 0x004ac8da
// ___multtenpow12

// LIBRARY: HYPERX 0x004aa35e
// __ld12tod

// LIBRARY: HYPERX 0x004aa374
// __ld12tof

// LIBRARY: HYPERX 0x0049eb05
// __fpmath

// LIBRARY: HYPERX 0x0049eb1c
// __cfltcvt_init

// Two of the six routines _cfltcvt_init stores, in its order.
// LIBRARY: HYPERX 0x004a36d7
// __cropzeros

// LIBRARY: HYPERX 0x004a3725
// __positive

// LIBRARY: HYPERX 0x0049f1c1
// ___onexitinit

// LIBRARY: HYPERX 0x0049fcd3
// _ungetc

// LIBRARY: HYPERX 0x004a1db9
// ___initstdio

// LIBRARY: HYPERX 0x004a1e61
// ___endstdio

// LIBRARY: HYPERX 0x004a355b
// ___initmbctable

// LIBRARY: HYPERX 0x004a3604
// __setdefaultprecision

// LIBRARY: HYPERX 0x004a3616
// __ms_p5_test_fdiv

// The two names the Pentium divide check loads KERNEL32 by.
// STRING: HYPERX 0x004b47f4
// "KERNEL32"
// STRING: HYPERX 0x004b47d8
// "IsProcessorFeaturePresent"

// The five literals the runtime-error writer builds its box from.
// STRING: HYPERX 0x004b4d4c
// "<program name unknown>"
// STRING: HYPERX 0x004b4d48
// "..."
// STRING: HYPERX 0x004b4d2c
// "Runtime Error!\n\nProgram: "
// STRING: HYPERX 0x004b4d28
// "\n\n"
// STRING: HYPERX 0x004b4d00
// "Microsoft Visual C++ Runtime Library"

// LIBRARY: HYPERX 0x004a3654
// __ms_p5_mp_test_fdiv

// LIBRARY: HYPERX 0x004a367d
// __forcdecpt

// LIBRARY: HYPERX 0x004a46c9
// ?_DestructExceptionObject@@YAXPAUEHExceptionRecord@@E@Z

// LIBRARY: HYPERX 0x004a47ac
// ?terminate@@YAXXZ

// LIBRARY: HYPERX 0x004a899e
// _toupper

// Its lower-case twin, which _forcdecpt calls to find the exponent marker.
// LIBRARY: HYPERX 0x004a288a
// _tolower

// The floating point input converter behind atof and scanf's %f.
// LIBRARY: HYPERX 0x004a7d75
// __fltin2

// LIBRARY: HYPERX 0x004a8a0d
// __toupper_lk

// LIBRARY: HYPERX 0x004a8c1d
// __fcloseall

// LIBRARY: HYPERX 0x004a8c9e
// ___tzset

// LIBRARY: HYPERX 0x004a8f53
// __isindst

// LIBRARY: HYPERX 0x004a9518
// __setenvp

// LIBRARY: HYPERX 0x004a95d1
// __setargv

// LIBRARY: HYPERX 0x004a9ca3
// __ultoa

// LIBRARY: HYPERX 0x004a9cbe
// __getpid

// LIBRARY: HYPERX 0x004a9cc4
// __rmtmp

// LIBRARY: HYPERX 0x004aa696
// _abort

// LIBRARY: HYPERX 0x0049ecaf
// ?_CallMemberFunction0@@YGXPAX0@Z

// LIBRARY: HYPERX 0x0049ecb6
// ?_CallMemberFunction2@@YGXPAX00H@Z

// LIBRARY: HYPERX 0x004a6e54
// ?__CxxSetUnhandledExceptionFilter@@YAXXZ

// LIBRARY: HYPERX 0x004a1d7d
// _fprintf

// LIBRARY: HYPERX 0x004a8ad9
// __stbuf

// LIBRARY: HYPERX 0x004a8b66
// __ftbuf

// The throw, and the type information the two compiler-generated throws in this tree pass it.
// LIBRARY: HYPERX 0x0049fba1
// __CxxThrowException@8

// GLOBAL: HYPERX 0x004b5a90
// _TI2PAD

// The other one: an unsigned int, thrown by the mixer search to stop it.
// GLOBAL: HYPERX 0x004b8e40
// _TI1I

// The three version.dll entry points the splash dialog reads its own version resource with.
// LIBRARY: HYPERX 0x004ac95c
// _VerQueryValueA@16

// LIBRARY: HYPERX 0x004ac962
// _GetFileVersionInfoA@16

// LIBRARY: HYPERX 0x004ac968
// _GetFileVersionInfoSizeA@8

// The mode table's sort, which DDraw::EnumDisplayModes reaches with a direct CALL.
// LIBRARY: HYPERX 0x004a0181
// _qsort

// The DirectDraw entry point, from ddraw.lib, reached with a direct CALL.
// LIBRARY: HYPERX 0x004ac96e
// _DirectDrawCreate@12

// The driver enumeration entry point, beside DirectDrawCreate and reached the same way.
// LIBRARY: HYPERX 0x004ac974
// _DirectDrawEnumerateA@8

// The DirectSound entry point, from dsound.lib.
// LIBRARY: HYPERX 0x004ac97a
// _DirectSoundCreate@12

// The two DirectPlay entry points the network set-up dialog calls, from dplayx.lib.
// LIBRARY: HYPERX 0x004ac980
// _DirectPlayEnumerateA@8

// LIBRARY: HYPERX 0x004ac986
// _DirectPlayCreate@12

// __CIacos: argument and answer in ST0, which VC6 emits for acos().
// LIBRARY: HYPERX 0x0049ebb0
// __CIacos

// _mbsncpy counts characters and delegates to _strncpy at 0x004a2db0 on a single-byte page.
// LIBRARY: HYPERX 0x004a0466
// __mbsncpy

// _mbsspnp answers a pointer and delegates to strspn at 0x004a76f0 on a single-byte page.
// LIBRARY: HYPERX 0x004a18f0
// __mbsspnp

// __CIasin: argument and answer in ST0, the twin of __CIacos at 0x0049ebb0.
// LIBRARY: HYPERX 0x0049f1f0
// __CIasin

// The compiler's pow helper: base in ST(1), exponent in ST(0).
// LIBRARY: HYPERX 0x0049fe20
// __CIpow

// LIBCMT's WinMainCRTStartup, from WINCRT0.C built with _WINMAIN_ and _MT.

// LIBRARY: HYPERX 0x004a209f
// _WinMainCRTStartup

// _mbslen, which the original measures the two waiting messages with.

// LIBRARY: HYPERX 0x004a1797
// __mbslen

// _ftime, which fills a struct _timeb.

// LIBRARY: HYPERX 0x004a1fba
// __ftime

// The x87 sqrt helper: argument and answer in ST0.

// LIBRARY: HYPERX 0x004a0040
// __CIsqrt

// The printable-character test the JPEG comment handler filters with.

// LIBRARY: HYPERX 0x004a1f8c
// _isprint

// The C entries of the transcendental routines; their shared bodies are unnamed labels in LIBCMT.

// LIBRARY: HYPERX 0x0049ebc4
// _acos

// LIBRARY: HYPERX 0x0049f204
// _asin

// LIBRARY: HYPERX 0x0049fe39
// _pow

// The floating point runtime's own helpers, identified by their offsets from __load_CW.

// LIBRARY: HYPERX 0x004a3d50
// __startTwoArgErrorHandling

// LIBRARY: HYPERX 0x004a3d67
// __startOneArgErrorHandling

// LIBRARY: HYPERX 0x004a3db0
// __twoToTOS

// LIBRARY: HYPERX 0x004a3dc5
// __load_CW

// LIBRARY: HYPERX 0x004a3ddc
// __convertTOStoQNaN

// LIBRARY: HYPERX 0x004a3df5
// __fload_withFB

// LIBRARY: HYPERX 0x004a3e38
// __checkTOS_withFB

// LIBRARY: HYPERX 0x004a3e4e
// __fast_exit

// LIBRARY: HYPERX 0x004a3e5b
// __math_exit

// LIBRARY: HYPERX 0x004a3e85
// __check_overflow_exit

// LIBRARY: HYPERX 0x004a3e99
// __check_range_exit

// The two pieces of runtime data those helpers read, kept beside the markers that need them.

// GLOBAL: HYPERX 0x004d2c08
// ___fastflag

// GLOBAL: HYPERX 0x004ca530
// __indefinite

// _mbslwr, in place and locale aware; the team checksum is its only caller.
// LIBRARY: HYPERX 0x004a1990
// __mbslwr

// atof; the plan tokenizer is its only caller.
// LIBRARY: HYPERX 0x004a0f4b
// _atof

// The public entries of FFLUSH.C.
// LIBRARY: HYPERX 0x004a2248
// _fflush

// LIBRARY: HYPERX 0x004a2277
// __fflush_lk

// LIBRARY: HYPERX 0x004a22a5
// __flush

// LIBRARY: HYPERX 0x004a2301
// __flushall

// LIBRARY: HYPERX 0x004abcf8
// __ismbblead

// LIBRARY: HYPERX 0x004ac040
// ___crtsetenv

// LIBRARY: HYPERX 0x004a1d66
// _strtoul

// LIBRARY: HYPERX 0x0049f2c9
// __purecall

// LIBRARY: HYPERX 0x004a1a1a
// __splitpath

// LIBRARY: HYPERX 0x004a8904
// __mbsnbcpy

// LIBRARY: HYPERX 0x004a7407
// __powhlp

// LIBRARY: HYPERX 0x004a7537
// __d_inttype

// LIBRARY: HYPERX 0x004aa576
// __87except

// LIBRARY: HYPERX 0x004ab6ab
// __fpclass

#endif
