/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ruby_sanctum.h"
#include "ScriptedCreature.h"

enum ZarithrianTexts
{
    SAY_AGGRO                   = 0,    // Alexstrasza has chosen capable allies.... A pity that I must END YOU!
    SAY_KILL                    = 1,    // You thought you stood a chance? - It's for the best.
    SAY_ADDS                    = 2,    // Turn them to ash, minions!
    SAY_DEATH                   = 3,    // HALION! I...
};

enum ZarithrianSpells
{
    // General Zarithrian
    SPELL_INTIMIDATING_ROAR     = 74384,
    SPELL_CLEAVE_ARMOR          = 74367,

    // Zarithrian Spawn Stalker
    SPELL_SUMMON_FLAMECALLER    = 74398,

    // Onyx Flamecaller
    SPELL_BLAST_NOVA            = 74392,
    SPELL_LAVA_GOUT             = 74394
};

enum ZarithrianEvents
{
    // General Zarithrian
    EVENT_CLEAVE                = 1,
    EVENT_INTIDMDATING_ROAR,
    EVENT_SUMMON_ADDS,
    EVENT_SUMMON_ADDS2,

    // Onyx Flamecaller
    EVENT_BLAST_NOVA,
    EVENT_LAVA_GOUT
};

enum ZarithrianMisc
{
    SPLINE_GENERAL_EAST = 1,
    SPLINE_GENERAL_WEST = 2,
    POINT_GENERAL_ROOM  = 3
};

// 39746 - General Zarithrian
struct boss_general_zarithrian : public BossAI
{
    boss_general_zarithrian(Creature* creature) : BossAI(creature, DATA_GENERAL_ZARITHRIAN) { }

    void Reset() override
    {
        _Reset();
        if (instance->GetBossState(DATA_SAVIANA_RAGEFIRE) == DONE && instance->GetBossState(DATA_BALTHARUS_THE_WARBORN) == DONE)
        {
            me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
            me->SetImmuneToPC(false);
        }
    }

    bool CanAIAttack(Unit const* target) const override
    {
        return (instance->GetBossState(DATA_SAVIANA_RAGEFIRE) == DONE && instance->GetBossState(DATA_BALTHARUS_THE_WARBORN) == DONE && BossAI::CanAIAttack(target));
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        events.ScheduleEvent(EVENT_CLEAVE, 8s);
        events.ScheduleEvent(EVENT_INTIDMDATING_ROAR, 14s);
        events.ScheduleEvent(EVENT_SUMMON_ADDS, 15s);
        if (Is25ManRaid())
            events.ScheduleEvent(EVENT_SUMMON_ADDS2, 16s);
    }

    // Override to not set adds in combat yet.
    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();
        Talk(SAY_DEATH);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        summons.DespawnAll();
        _DespawnAtEvade();
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_KILL);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_SUMMON_ADDS:
                    Talk(SAY_ADDS);
                    [[fallthrough]];
                case EVENT_SUMMON_ADDS2:
                {
                    if (Creature* stalker1 = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_ZARITHRIAN_SPAWN_STALKER_1)))
                        stalker1->CastSpell(stalker1, SPELL_SUMMON_FLAMECALLER, true);

                    if (Creature* stalker2 = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_ZARITHRIAN_SPAWN_STALKER_2)))
                        stalker2->CastSpell(stalker2, SPELL_SUMMON_FLAMECALLER, true);

                    events.Repeat(45s);
                    break;
                }
                case EVENT_INTIDMDATING_ROAR:
                    DoCastSelf(SPELL_INTIMIDATING_ROAR);
                    events.Repeat(35s, 40s);
                    break;
                case EVENT_CLEAVE:
                    DoCastVictim(SPELL_CLEAVE_ARMOR);
                    events.ScheduleEvent(EVENT_CLEAVE, 15s);
                    break;
                default:
                    break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }
};

// 39814 - Onyx Flamecaller
struct npc_onyx_flamecaller : public ScriptedAI
{
    npc_onyx_flamecaller(Creature* creature) : ScriptedAI(creature), _instance(creature->GetInstanceScript()), _lavaGoutCount(0) { }

    void Reset() override
    {
        _events.Reset();
        _lavaGoutCount = 0;
        me->SetReactState(REACT_DEFENSIVE);
        MoveToGeneral();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EVENT_BLAST_NOVA, 17s);
        _events.ScheduleEvent(EVENT_LAVA_GOUT, 3s);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        // Let Zarithrian count as summoner.
        if (Creature* zarithrian = _instance->GetCreature(DATA_GENERAL_ZARITHRIAN))
            zarithrian->AI()->JustSummoned(me);
    }

    void MovementInform(uint32 type, uint32 pointId) override
    {
        if (type != SPLINE_CHAIN_MOTION_TYPE && pointId != POINT_GENERAL_ROOM)
            return;

        DoZoneInCombat();
    }

    void MoveToGeneral()
    {
        if (me->GetPositionY() < 500.0f)
            me->GetMotionMaster()->MoveAlongSplineChain(POINT_GENERAL_ROOM, SPLINE_GENERAL_WEST, false);
        else
            me->GetMotionMaster()->MoveAlongSplineChain(POINT_GENERAL_ROOM, SPLINE_GENERAL_EAST, false);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_BLAST_NOVA:
                    DoCastAOE(SPELL_BLAST_NOVA);
                    _events.Repeat(15s, 20s);
                    break;
                case EVENT_LAVA_GOUT:
                    if (_lavaGoutCount >= 3)
                    {
                        _lavaGoutCount = 0;
                        _events.Repeat(8s);
                        break;
                    }
                    DoCastVictim(SPELL_LAVA_GOUT);
                    _lavaGoutCount++;
                    _events.Repeat(1s);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
    InstanceScript* _instance;
    uint8 _lavaGoutCount;
};

void AddSC_boss_general_zarithrian()
{
    RegisterRubySanctumCreatureAI(boss_general_zarithrian);
    RegisterRubySanctumCreatureAI(npc_onyx_flamecaller);
}
