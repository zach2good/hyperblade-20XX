#ifndef HYPERBLADE_AGENDAITEMIDS_H
#define HYPERBLADE_AGENDAITEMIDS_H

// DATA/ENUMIDS.H's AgendaItemIDs; each indexes m_abAgendaPurged and g_apAgendaItemPools.
enum AgendaItemIDs
{
    Agenda_CBeAttached           = 0,
    Agenda_CSteerToTarget        = 1,
    Agenda_CSetPositionRelative  = 2,
    Agenda_CSteerToward          = 3,
    Agenda_CAdjustAcceleration   = 4,
    Agenda_CSteerToPosition      = 5,
    Agenda_CSteerBetweenFacing   = 6,
    Agenda_CBungeeToward         = 7,
    Agenda_CAvoidObjects         = 8,
    Agenda_CMonitorKeys          = 9,
    Agenda_CSpeedBurst           = 10,
    Agenda_CGlideOnSurface       = 11,
    Agenda_CSlideOnRail          = 12,
    Agenda_CProjectAndBounce     = 13,
    Agenda_CStepAnimation        = 14,
    Agenda_CRelaxToPosition      = 15,
    Agenda_CSetAttitude          = 16,
    Agenda_CRelaxYaw             = 17,
    Agenda_CTimerActivity        = 18,
    Agenda_CTrack                = 19,
    Agenda_CTrackTraj            = 20,
    Agenda_CWatch                = 21,
    Agenda_CTrackPasser_Receiver = 22,
    Agenda_CViewIt               = 23,
    Agenda_CPanSound             = 24,
    Agenda_CHeadTrack            = 25,
    Agenda_CDopplerSnd           = 26,
    Agenda_CPaletteEffect        = 27,
    Agenda_CDisplayShotPower     = 28,
    Agenda_CDisplayPassBar       = 29,
    Agenda_CMonitorHeldKeys      = 30,
    Agenda_CUpdateShotPower      = 31,
    Agenda_CExplosionAnimation   = 32,
    Agenda_CCheckAI              = 33,
    Agenda_CCircleTrack          = 34,
    Agenda_Count                 = 35
};

#endif
