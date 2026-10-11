# Checks that each checker in the battery still reports the fault it was written for.
#
# Each probe plants the exact shape its checker exists to catch, runs the checker, and compares
# its output before and after. The file is restored in a finally arm. A restore can still fail on
# Windows if another process holds the file, so check that `git status` is empty afterwards.
#
# The signal is the output, not the exit code: some battery entries are surveys that exit 0 and
# only change a count. A checker with no probe is listed as unproven, not counted as passing.
#
# Usage: python tools/check_battery_bites.py [--list]
#   --list  also print each checker that bites and each unproven one.
# Exits 1 on any fault.

import os
import re
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

sys.path.insert(0, os.path.join(ROOT, "tools"))

# (checker entry, file to plant in, the text to find, what to put there instead).
#
# The find text is an exact existing line, so a probe that no longer applies fails loudly.
# A compiled pattern finds its first match instead, and the plant is a template expanded against
# it; use one where the anchoring line is likely to be edited. A find of None means the plant is
# a new file at that path, for a checker that asks git what is tracked.
PROBES = (
    ("check_self_assign.py", "src/engine/rokobject.cpp",
     "    m_bAtSpawnSlot = 0;",
     "    m_bAtSpawnSlot = m_bAtSpawnSlot;"),

    ("check_declaration_style.py", "src/engine/rokobject.cpp",
     "    m_bAtSpawnSlot = 0;",
     "    int nProbeA, nProbeB;\n    (void)nProbeA;\n    (void)nProbeB;\n    m_bAtSpawnSlot = 0;"),

    ("check_null_test_casts.py", "src/engine/engineobject.cpp",
     re.compile(r"^(\s*)if \((m_p\w+) != 0\)", re.M),
     r"\1if ((char*)\2 != 0)"),

    ("self_type_casts.py", "src/planvm/cslideonrail.cpp",
     "    m_pPosition->z = ",
     "    ((Vec3*)m_pPosition)->z = "),

    # The call has to be one whose return type a header states, or there is nothing to
    # compare the cast against and the checker is right to say nothing.
    ("redundant_return_casts.py", "src/common/behobjavoidance.cpp",
     re.compile(r"= (\w+)->GetPhysicsBody\(\);"),
     r"= (PhysicsBody*)\1->GetPhysicsBody();"),

    ("check_chained_casts.py", "src/planvm/cslideonrail.cpp",
     "    m_pSpline = 0;",
     "    m_pSpline = (Spline*)(char*)m_pSpline;"),

    # A short taking an int. The int must be a name only one class declares, or the checker
    # drops it.
    ("implicit_narrowing.py", "src/engine/rokobject.cpp",
     "    m_bAtSpawnSlot = 0;",
     "    short nProbe;\n    nProbe = m_nCarrierSlot;\n    (void)nProbe;\n    m_bAtSpawnSlot = 0;"),

    # A forward declaration with the other keyword; nothing else mentions the class to clash with.
    ("check_class_struct_keyword.py", "src/planvm/cslideonrail.h",
     "class CSlideOnRail : public AgendaItem",
     "struct CSlideOnRail;\n\nclass CSlideOnRail : public AgendaItem"),

    ("check_explicit_ctors.py", "src/planvm/cslideonrail.h",
     "    CSlideOnRail();",
     "    CSlideOnRail(int nProbe);"),

    # A byte offset on a pointer to a record. On a pointer to a scalar the same term is an
    # element index, and the checker skips it.
    ("check_scaled_offsets.py", "src/planvm/cslideonrail.cpp",
     "    m_pSpline = 0;",
     "    Spline* pProbe;\n    pProbe = m_pSpline + 0x2c;\n    (void)pProbe;\n    m_pSpline = 0;"),

    # The checker reads only a declaration and an assignment in separate statements.
    ("check_discarded_types.py", "src/planvm/cslideonrail.cpp",
     "    m_pSpline = 0;",
     "    char* pDiscard;\n    pDiscard = (char*)m_pSpline;\n    (void)pDiscard;\n    m_pSpline = 0;"),

    # A define nothing reads.
    ("check_dead_defines.py", "src/planvm/cslideonrail.cpp",
     "#include \"planvm/cslideonrail.h\"",
     "#include \"planvm/cslideonrail.h\"\n#define PROBE_UNREAD 0x1234"),

    ("dead_defines.py", "src/planvm/cslideonrail.cpp",
     "#include \"planvm/cslideonrail.h\"",
     "#include \"planvm/cslideonrail.h\"\n#define PROBE_UNREAD 0x1234"),

    # One name defined in two files with two values. The checker treats a value below 0x10 as a
    # count, not an offset, unless the name ends in a sentinel suffix.
    ("check_duplicate_constants.py", "src/planvm/cslideonrail.cpp",
     "#include \"planvm/cslideonrail.h\"",
     "#include \"planvm/cslideonrail.h\"\n#define ARENA_TRACK_UP 0x99"),

    # An enumerator that a different file defines as a macro. In one file the two would not
    # compile.
    ("check_macro_enum_clash.py", "src/globals.h",
     "// Globals shared across translation units.",
     "#define SHADE_WHITE 1\n// Globals shared across translation units."),

    # A local read and never written, which VC6 does not diagnose.
    ("check_unassigned_locals.py", "src/planvm/cslideonrail.cpp",
     "    m_pSpline = 0;",
     "    Spline* pNever;\n    m_pSpline = pNever;\n    m_pSpline = 0;"),

    # A cast of this to the body's own class, in an ordinary member. The checker's
    # `(\\w+)::(\\w+)` never matches `Class::~Class`, so a destructor's body is skipped.
    ("check_self_casts.py", "src/planvm/cslideonrail.cpp",
     "    *m_pPrevPos = *m_pPosition;",
     "    ((CSlideOnRail*)this)->CSlideOnRail::Release(0);\n"
     "    *m_pPrevPos = *m_pPosition;"),

    # Both units in one statement and on the same literal, which is the rule the checker
    # enforces.
    ("check_mixed_units.py", "src/planvm/cslideonrail.cpp",
     "    *m_pPrevPos = *m_pPosition;",
     "    *(float*)(m_pflSpeed + 4) = *(float*)((char*)m_pVelocity + 4);\n"
     "    *m_pPrevPos = *m_pPosition;"),

    # Two writes to one name with no read between them.
    ("check_dead_stores.py", "src/planvm/cslideonrail.cpp",
     "    m_pSpline = 0;",
     "    Spline* pDead;\n    pDead = m_pSpline;\n    pDead = 0;\n    (void)pDead;\n"
     "    m_pSpline = 0;"),

    # A comment between a marker and its declaration, which reccmp reads as the symbol's name.
    ("check_marker_comments.py", "src/planvm/cslideonrail.cpp",
     "// FUNCTION: HYPERX 0x0040cee0",
     "// FUNCTION: HYPERX 0x0040cee0\n// A sentence where the declaration should be."),
    # An alias of this that the body goes on to never read.
    ("check_dead_this_alias.py", "src/planvm/cslideonrail.cpp",
     "    *m_pPrevPos = *m_pPosition;",
     "    char* const pObj = (char*)this;\n"
     "    *m_pPrevPos = *m_pPosition;"),

    # An offset comment that goes backwards.
    ("check_field_offsets.py", "src/planvm/cslideonrail.h",
     "    Spline* m_pSpline;     // 0x2c",
     "    Spline* m_pSpline;     // 0x04"),

    # An explicit destructor call with no class qualifier, which on a virtual destructor is
    # a vtable call where the tree always means the direct one.
    ("check_dtor_calls.py", "src/planvm/cslideonrail.cpp",
     "    m_pSpline = 0;",
     "    ((CSlideOnRail*)this)->~CSlideOnRail();\n    m_pSpline = 0;"),

    # A path named in a document that is not in the tree.
    ("check_doc_paths.py", "docs/RE_CRUFT.md",
     "## What is left",
     "A probe naming src/nosuchplace/nosuchfile.cpp.\n\n## What is left"),
    ("check_doc_paths.py", "CONTRIBUTING.md",
     "## Where records go",
     "A probe naming src/nosuchplace/nosuchfile.cpp.\n\n## Where records go"),
    ("check_doc_paths.py", "tools/README.md",
     "## Checkers",
     "A probe naming src/nosuchplace/nosuchfile.cpp.\n\n## Checkers"),
    # A close with nothing opening it.
    ("check_balance.py", "src/planvm/cslideonrail.cpp",
     "    m_pSpline = 0;",
     "    m_pSpline = 0);"),

    # A byte offset added to a pointer to a record. The checker reads only a bare local name,
    # not a cast around it.
    ("check_record_arith.py", "src/planvm/cslideonrail.cpp",
     "    *m_pPrevPos = *m_pPosition;",
     "    Spline* pRec;\n    pRec = m_pSpline;\n"
     "    *(float*)(pRec + 0x10) = 0.0f;\n"
     "    *m_pPrevPos = *m_pPosition;"),

    # A virtual declared with no `= 0` and no body, which VC6 asks for only when it emits
    # the table, so an abstract class can carry one and the build stay green.
    ("check_virtual_bodies.py", "src/planvm/cslideonrail.h",
     "    virtual void OnListChanged();",
     "    virtual void OnListChanged();\n    virtual void ProbeNeverDefined();"),

    # A hole in a class's declared run, which slides every field under it.
    ("check_field_tiling.py", "src/planvm/cslideonrail.h",
     "    float*  m_pflSpeed;    // 0x24, the address of the body's flSpeed",
     ""),

    # A cast reading four bytes out of a two byte field. The base must be a name the body
    # declares: the checker cannot resolve an inherited member reached through this.
    ("check_cast_width.py", "src/planvm/cslideonrail.cpp",
     "    *m_pPrevPos = *m_pPosition;",
     "    EngineObject* pWide;\n    pWide = m_pOwner;\n"
     "    *(int*)&pWide->m_wBoneCount = 0;\n"
     "    *m_pPrevPos = *m_pPosition;"),
    # Two GLOBAL markers naming one address, which leaves reccmp's choice of name arbitrary.
    ("check_globals.py", "src/globals.h",
     "// Globals shared across translation units.",
     "// GLOBAL: HYPERX 0x004cb440\nextern int g_nProbeDouble;\n"
     "// Globals shared across translation units."),

    # A field declared at offset nought on a class whose vptr already holds those bytes.
    ("check_vptr_offsets.py", "src/planvm/cslideonrail.h",
     "    Vec3*   m_pVelocity;   // 0x10, the address of the body's vel",
     "    int     m_nProbeAtZero; // 0x00\n"
     "    Vec3*   m_pVelocity;   // 0x10, the address of the body's vel"),

    # tools/README.md no longer matching the directory it indexes.
    ("index.py --check", "tools/README.md",
     "# The tools",
     "# The tools\n\nA line the generator did not write."),
    # A body reaching past the end of its own class.
    ("check_body_reach.py", "src/planvm/cslideonrail.cpp",
     "    *m_pPrevPos = *m_pPosition;",
     "    *(int*)((char*)this + 0x400) = 0;\n"
     "    *m_pPrevPos = *m_pPosition;"),

    # A score written into a source comment.
    ("check_score_prose.py", "src/planvm/cslideonrail.cpp",
     "// FUNCTION: HYPERX 0x0040cee0",
     "// It went 50.00 to exact for the spelling above.\n"
     "// FUNCTION: HYPERX 0x0040cee0"),

    # A volatile spelled to force a store, with no shared or hardware reason beside it.
    ("check_volatile.py", "src/planvm/cslideonrail.cpp",
     "    m_pSpline = 0;",
     "    *(void* volatile*)&m_pSpline = 0;"),

    # Inline assembly in a function the register of original assembly does not list.
    ("check_asm_bodies.py", "src/planvm/cslideonrail.cpp",
     "    m_pSpline = 0;",
     "    __asm nop\n    m_pSpline = 0;"),

    # A slot declared with an argument count its own table does not carry.
    ("check_slot_arity.py", "src/planvm/cslideonrail.h",
     "    virtual void Process(int nArg);",
     "    virtual void Process(int nArg, int nProbe);"),

    # A body claiming a class the image's tables do not put it on.
    ("check_member_owner.py", "src/planvm/cslideonrail.cpp",
     "void CSlideOnRail::Process(int nArg)",
     "void CGlideOnSurface::Process(int nArg)"),
    # A table in the image that no class names, made by breaking the class's marker.
    ("check_tables_named.py", "src/planvm/cslideonrail.h",
     "// VTABLE: HYPERX 0x004b1a84",
     "// The table that used to be named here was 0x004b1a84."),

    # A member storing a g_vtbl array that is not its own class's table (0x004b1a84).
    ("check_vtbl_stores.py", "src/planvm/cslideonrail.cpp",
     "    *m_pPrevPos = *m_pPosition;",
     "    *(const void**)this = g_vtbl004b2010;\n"
     "    *m_pPrevPos = *m_pPosition;"),
    # An untracked source file. The probe writes the path and the finally arm removes it.
    ("check_stray_sources.py", "src/zz_probe_stray.cpp", None,
     "// A file the probe wrote, which nothing tracks.\n"),
    # One class declared in two files. view_classes.py reads a declaration only with the
    # opener at column 0 and its brace on the next line.
    ("view_classes.py --twice", "src/planvm/agendaitem.h",
     "#define HYPERBLADE_AGENDAITEM_H",
     "#define HYPERBLADE_AGENDAITEM_H\n\nclass CSlideOnRail\n{\n};"),
    # A base's own body reading an offset only a derived class declares. AgendaItem ends at
    # 0x10.
    ("base_reads_derived.py", "src/planvm/agendaitem.cpp",
     "void AgendaItem::OnListChanged()\n{",
     "void AgendaItem::OnListChanged()\n{\n    *(int*)((char*)this + 0x10) = 0;"),
    # Two siblings reserving the same run. A reserve counts only with a field after it, and its
    # name must end in its own offset.
    ("shared_reserves.py", "src/engine/crelaxyaw.h",
     "#define HYPERBLADE_CRELAXYAW_H",
     "#define HYPERBLADE_CRELAXYAW_H\n\n"
     "class ProbeReserveA : public AgendaItem\n{\n"
     "    char m_gap10[4];           // 0x10\n"
     "    float m_flRate;            // 0x14\n};\n\n"
     "class ProbeReserveB : public AgendaItem\n{\n"
     "    char m_gap10[4];           // 0x10\n"
     "    float m_flRate;            // 0x14\n};"),
    # Two headers sharing a guard macro. The second header read is empty, which the build can
    # miss.
    ("check_include_hygiene.py", "src/math/point.h",
     "#ifndef HYPERBLADE_POINT_H\n#define HYPERBLADE_POINT_H",
     "#ifndef HYPERBLADE_QUAT_H\n#define HYPERBLADE_QUAT_H"),

    # A forward declaration for a class nothing defines.
    ("check_ghost_forwards.py", "src/graphics/meshbone.h",
     "class MeshBoneDesc",
     "class ProbeRetiredClass;\n\nclass MeshBoneDesc"),

    ("check_self_naming_notes.py", "src/math/point.h",
     "#ifndef HYPERBLADE_POINT_H",
     "// The twin of src/math/point.h.\n#ifndef HYPERBLADE_POINT_H"),

    # A comment naming a src/ file that is not there.
    ("check_source_paths.py", "src/planvm/cslideonrail.h",
     "class CSlideOnRail",
     "// See src/planvm/probe_retired_file.cpp for the rest.\nclass CSlideOnRail"),

    # A name and an address that disagree, planted on an address the tree marks.
    ("check_named_addresses.py", "src/ui/infobar.cpp",
     "// This class's LoadByName at 0x0044f8f0;",
     "// This class's ProbeWrongName at 0x0044f8f0;"),

    # A note naming another index. id_ACF_PanSndType is registered at 13.
    ("check_id_notes.py", "src/anim/animevent.cpp",
     "the plan action table registers this body at 13.",
     "the plan action table registers this body at 14."),

    # A bare // closing a note.
    ("check_note_scars.py", "src/planvm/cslideonrail.cpp",
     '#include "globals.h"\n',
     '#include "globals.h"\n\n// A note.\n//\n'),

    # A forward declaration below the include that defines the class.
    ("check_redundant_forwards.py", "src/planvm/cslideonrail.cpp",
     '#include "globals.h"\n',
     '#include "globals.h"\n\nclass CSlideOnRail;\n'),

    # A global restated with no note above it. globals.h already declares g_pGameEngine.
    ("check_restated_externs.py", "src/planvm/cslideonrail.cpp",
     '#include "globals.h"\n',
     '#include "globals.h"\n\nextern struct GameEngine* g_pGameEngine;\n'),

    # An extern array with no bound.
    ("check_extern_bounds.py", "src/planvm/cslideonrail.cpp",
     '#include "globals.h"\n',
     '#include "globals.h"\n\nextern int g_aProbeUnbounded[];\n'),

    # A global named for its address.
    ("placeholder_globals.py --max 0", "src/planvm/cslideonrail.cpp",
     '#include "globals.h"\n',
     '#include "globals.h"\n\nint g_nProbe004B0000;\n'),

    # A function named for its address.
    ("placeholder_functions.py --max 0", "src/planvm/cslideonrail.cpp",
     '#include "globals.h"\n',
     '#include "globals.h"\n\nvoid InitializeProbe004B0000(void);\n'),

    # A member declared and defined nowhere. It goes below a declaration, because a note above
    # a run of them exempts the run.
    ("check_undefined_members.py", "src/planvm/cslideonrail.h",
     "    CSlideOnRail();\n",
     "    CSlideOnRail();\n    void ProbeNoBody();\n"),

    # A sentence naming one file on both sides of an "and".
    ("check_doubled_file_names.py", "src/math/math.cpp",
     "// A = A * B for four by four matrices, with B read only.\n",
     "// A = A * B for four by four matrices. See quat.h and quat.h.\n"),

    # Two lines of a note written twice.
    ("check_duplicated_prose.py", "src/common/mem.cpp",
     "// What it does: clears 192 bytes as three runs of 64, first dwords, then words, then bytes, with\n"
     "// one pointer walking on through all three.\n",
     "// What it does: clears 192 bytes as three runs of 64, first dwords, then words, then bytes, with\n"
     "// one pointer walking on through all three.\n"
     "// What it does: clears 192 bytes as three runs of 64, first dwords, then words, then bytes, with\n"
     "// one pointer walking on through all three.\n"),

    # An include the file has no use for. math/quat.h reaches no system include; the checker
    # skips any header that pulls in common/compat.h.
    ("unused_includes.py", "src/graphics/mdm1.cpp",
     '#include "common/compat.h"\n',
     '#include "common/compat.h"\n#include "math/quat.h"\n'),

    ("unused_includes.py --headers", "src/math/point.h",
     "#define HYPERBLADE_POINT_H\n",
     '#define HYPERBLADE_POINT_H\n#include "math/quat.h"\n'),

    # Naming an unread parameter is the fault shape: nothing then admits the body never reads it.
    ("unused_params.py", "src/anim/animevent.cpp",
     "void __cdecl AnimEvent::PlaySnd(Interactor*, PlanNode* pEvent, short*, int)\n",
     "void __cdecl AnimEvent::PlaySnd(Interactor*, PlanNode* pEvent, short* pProbe, int)\n"),

    ("const_accessors.py", "src/anim/animdb.h",
     "    const char* GetName() const\n",
     "    const char* ProbeAccessor()\n"
     "    {\n"
     "        return m_szName;\n"
     "    }\n"
     "\n"
     "    const char* GetName() const\n"),

    # A ledger row whose leading score is not the report's.
    ("check_ledger_figures.py", "symbols/retired.csv",
     re.compile(r'^(0x[0-9a-f]{8},[^,\n]+,[\w-]+,")[5-9]\d\.\d\d', re.M),
     r"\g<1>10.00"),

    # The same, stated after the row's size.
    ("check_ledger_figures.py", "symbols/retired.csv",
     re.compile(r'^(0x[0-9a-f]{8},[^,\n]+,[\w-]+,"\d+ bytes(?:,| at) )[5-9]\d\.\d\d', re.M),
     r"\g<1>10.00"),

    # A cast of a call to the scalar it already returns.
    ("redundant_return_casts.py", "src/anim/animplayer.cpp",
     re.compile(r"= (\w+)->GetEntryCount\(\);"),
     r"= (short)\1->GetEntryCount();"),

    # A local initialised once and never written, without const.
    ("const_locals.py", "src/engine/csteertoposition.cpp",
     "    PhysicsBody* const pBody = m_pOwner->GetPhysicsBody();",
     "    PhysicsBody* pBody = m_pOwner->GetPhysicsBody();"),

    # Two $E2 owners linked the wrong way round, which pairs each marker with the other's thunk.
    ("check_link_order.py", "CMakeLists.txt",
     "    src/graphics/meshbone.cpp\n    src/engine/rokobject.cpp",
     "    src/engine/rokobject.cpp\n    src/graphics/meshbone.cpp"),

    # Two markers of one unit renumbered, so each pairs with the other's thunk in the right file.
    ("check_link_order.py", "src/common/staticinit.h",
     "// SYNTHETIC: HYPERX 0x00401800\n// $E8\n// SYNTHETIC: HYPERX 0x00401810\n// $E7",
     "// SYNTHETIC: HYPERX 0x00401800\n// $E7\n// SYNTHETIC: HYPERX 0x00401810\n// $E8"),

    # A retired row that names a function the tree has renamed.
    ("check_retired.py", "symbols/retired.csv",
     "0x00452730,MessageQueue::Update,",
     "0x00452730,InfoBar::ProcessMessageQueue,"),

    ("base_this_casts.py", "src/planvm/cslideonrail.cpp",
     "    m_pSpline = 0;",
     "    ((AgendaItem*)this)->AgendaItem::Release(0);\n    m_pSpline = 0;"),

    # A kind define whose value is another type in the client's table.
    ("kind_names.py", "src/engine/gameengine.cpp",
     "        KIND_NULL, KIND_DROMER, KIND_ROK,",
     "        KIND_NULL, KIND_ROK, KIND_DROMER,"),

    # A define at the offset of a field that holds another kind of value. The DEF prefix names
    # ObjectDef, so the include that brings it in is the anchor.
    ("rival_names.py", "src/engine/playerobject.cpp",
     re.compile(r'^#include "objectdef\.h"', re.M),
     r"\g<0>\n#define DEF_MASS 0x3c\n#define DEF_MASS_AT(p) ((char*)(p) + DEF_MASS)"),

    # A name for a blob offset that the covering class calls a count.
    ("blob_rivals.py", "src/planvm/crelaxtoposition.cpp",
     "    if (pNode->m_wKind == BEH_GLIDE_TO_POSITION)",
     "#define NODE_TEXT 0x04\n    char* const pNodeRead = (char*)pNode;\n\n"
     "    if (*(short*)(pNodeRead + NODE_TEXT) == 0x0b)\n    {\n        return;\n    }\n\n"
     "    if (pNode->m_wKind == BEH_GLIDE_TO_POSITION)"),

    # These two read DATA/ENUMIDS.H, or their caches in tools/ without it.
    ("plan_names.py", "src/planvm/planvm.cpp",
     "    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DECIDE_ATTACHMENT].pfnDecide             = PlanCond::Decide_Attachment;",
     "    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DECIDE_ATTACHMENT].pfnDecide             = PlanCond::DecideNever;"),

    ("enum_names.py", "src/engine/collisionobjectids.h",
     "#define COB_ROK              0x1b",
     "#define COB_ROK              0x14"),

    ("marker_next_line.py", "src/math/vec3.cpp",
     "// FUNCTION: HYPERX 0x00430320",
     "// FUNCTION: HYPERX 0x00430320\n// A sentence where the declaration should be."),

    # Two markers on one address.
    ("check_lint.py", "src/planvm/cslideonrail.cpp",
     "// FUNCTION: HYPERX 0x0040cfa0",
     "// FUNCTION: HYPERX 0x0040cee0"),

    # A class spelled into a free function's name.
    ("check_prefixed_functions.py", "src/planvm/cslideonrail.cpp",
     "// FUNCTION: HYPERX 0x0040cee0",
     "static void SlideOnRailInteractor__Probe(void)\n{\n}\n\n"
     "// FUNCTION: HYPERX 0x0040cee0"),

    # A body of a class whose bodies all sit in another file.
    ("namespace_spread.py", "src/planvm/cslideonrail.cpp",
     "// FUNCTION: HYPERX 0x0040cee0",
     "void RokObject::ProbeSpread(void)\n{\n}\n\n"
     "// FUNCTION: HYPERX 0x0040cee0"),

    # A game function at global scope, which belongs to a class or a namespace.
    ("namespace_spread.py", "src/planvm/cslideonrail.cpp",
     "// FUNCTION: HYPERX 0x0040cee0",
     "void ProbeAtGlobalScope(void)\n{\n}\n\n"
     "// FUNCTION: HYPERX 0x0040cee0"),

    # A marked body for a class that shares its header: MeshBoneDesc is defined in meshbone.h.
    ("header_homes.py", "src/planvm/cslideonrail.cpp",
     "// FUNCTION: HYPERX 0x0040cee0",
     "// FUNCTION: HYPERX 0x0040cee0\nvoid MeshBoneDesc::Probe(void)\n{\n}\n\n"
     "// FUNCTION: HYPERX 0x0040cee0"),

    # A class with no marked body that shares a header: a virtual function makes it one.
    ("header_homes.py", "src/graphics/meshbone.h",
     "class MeshBoneDesc\n",
     "class ProbeView\n{\npublic:\n    virtual void Probe();\n};\n\nclass MeshBoneDesc\n"),

    # More pairs than the fixture's sites can give.
    ("x87_precision.py --selftest", "tools/x87_precision_fixture.json",
     '"min_pairs":5,',
     '"min_pairs":50,'),

    # The first branch pushed with the second branch's tag.
    ("plan_dump.py --selftest", "tools/plan_dump.py",
     "        return short((word & 0xfff) | branch | 0x9000)",
     "        return short((word & 0xfff) | branch | 0xa000)"),
)

# Checkers with no probe, and what a probe would have to plant.
UNPROVEN = {
}


def run(entry):
    """Return the checker's exit code and output."""
    parts = entry.split()
    out = subprocess.run([sys.executable, os.path.join("tools", parts[0])] + parts[1:],
                         capture_output=True, text=True, cwd=ROOT)

    return "%d\n%s" % (out.returncode, out.stdout)


def write(whole, text):
    """Write a file, retrying because another process can hold the handle briefly on Windows."""
    for wait in (0, 0.2, 0.5, 1.0, 2.0):
        if wait:
            time.sleep(wait)

        try:
            open(whole, "w", encoding="utf-8", newline="").write(text)
            return
        except OSError:
            continue

    raise SystemExit("COULD NOT PUT %s BACK. Restore it with git checkout before anything "
                     "else; a probe's plant is still in it." % whole)


def appears(entry, path, plant):
    """Like bites(), for a fault that is a new file at `path`. The finally arm removes it."""
    whole = os.path.join(ROOT, path)

    if os.path.exists(whole):
        return None, "%s is already there, so the probe would edit rather than create" % path

    before = run(entry)

    try:
        write(whole, plant)

        after = run(entry)
    finally:
        for wait in (0, 0.2, 0.5, 1.0, 2.0):
            if wait:
                time.sleep(wait)

            try:
                if os.path.exists(whole):
                    os.remove(whole)

                break
            except OSError:
                continue
        else:
            raise SystemExit("COULD NOT REMOVE %s. Delete it before anything else." % whole)

    if after == before:
        return False, "said exactly the same thing with %s in the tree" % path

    return True, "gate" if before.split("\n")[0] != after.split("\n")[0] else "survey"


def bites(entry, path, find, plant):
    """Whether the checker's output changes once the fault is in the tree."""
    if find is None:
        return appears(entry, path, plant)

    whole = os.path.join(ROOT, path)
    was = open(whole, encoding="utf-8", newline="").read()
    nl = "\r\n" if "\r\n" in was else "\n"

    if hasattr(find, "search"):
        match = find.search(was)

        if match is None:
            return None, "the probe's pattern matches nothing in %s any more" % path

        planted = was[:match.start()] + match.expand(plant.replace("\n", nl)) + was[match.end():]
    else:
        # The anchor takes the file's line endings too, or a multi-line find never matches CRLF.
        find = find.replace("\n", nl)

        if was.count(find) < 1:
            return None, "the probe's anchor is not in %s any more" % path

        planted = was.replace(find, plant.replace("\n", nl), 1)

    before = run(entry)

    try:
        write(whole, planted)

        after = run(entry)
    finally:
        write(whole, was)

    if after == before:
        if "SKIP" in before:
            return False, "skipped, so nothing was checked: %s" % before.strip().split("\n")[-1]

        return False, "said exactly the same thing with the fault planted in %s" % path

    # A gate moves its exit code; a survey moves only its count. The battery gates on the first.
    gate = before.split("\n")[0] != after.split("\n")[0]

    return True, "gate" if gate else "survey, the battery does not gate on it"


def main():
    os.chdir(ROOT)
    show = "--list" in sys.argv

    import checkers as battery

    faults = []
    bit = []

    for entry, path, find, plant in PROBES:
        answer, why = bites(entry, path, find, plant)

        if answer is True:
            bit.append((entry, why))
            continue

        faults.append((entry, why))

    listed = set(entry for entry, path, find, plant in PROBES) | set(UNPROVEN)
    missing = [entry for entry in battery.CHECKERS if entry not in listed]

    for entry in missing:
        faults.append((entry, "is in the battery and this file does not mention it"))

    if show:
        for entry, why in bit:
            print("%-40s bites, %s" % (entry, why))

        for entry in sorted(UNPROVEN):
            if entry in battery.CHECKERS:
                print("%-40s UNPROVEN, would need %s" % (entry, UNPROVEN[entry]))

    for entry, why in faults:
        print("FAULT %-34s %s" % (entry, why))

    print("%d checker(s) in the battery: %d probe(s) bite, %d with no probe, %d fault(s)"
          % (len(battery.CHECKERS), len(bit),
             len([e for e in UNPROVEN if e in battery.CHECKERS]), len(faults)))

    return 1 if faults else 0


if __name__ == "__main__":
    sys.exit(main())
