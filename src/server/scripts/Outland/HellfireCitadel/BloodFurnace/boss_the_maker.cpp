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
#include "ScriptedCreature.h"
#include "blood_furnace.h"

enum MakerTexts
{
    SAY_AGGRO                   = 0,
    SAY_SLAY                    = 1,
    SAY_DEATH                   = 2
};

enum MakerSpells
{
    SPELL_ACID_SPRAY            = 38153,
    SPELL_EXPLODING_BREAKER     = 30925,
    SPELL_KNOCKDOWN             = 20276,
    SPELL_DOMINATION            = 25772
};

enum MakerEvents
{
    EVENT_ACID_SPRAY            = 1,
    EVENT_EXPLODING_BREAKER,
    EVENT_DOMINATION,
    EVENT_KNOCKDOWN
};

// 17381 - The Maker
struct boss_the_maker : public BossAI
{
    boss_the_maker(Creature* creature) : BossAI(creature, DATA_THE_MAKER) { }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);

        events.ScheduleEvent(EVENT_ACID_SPRAY, 15s);
        events.ScheduleEvent(EVENT_EXPLODING_BREAKER, 6s);
        events.ScheduleEvent(EVENT_DOMINATION, 120s);
        events.ScheduleEvent(EVENT_KNOCKDOWN, 10s);
    }

    void KilledUnit(Unit* who) override
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_SLAY);
    }

    void JustDied(Unit* /*killer*/) override
    {
        _JustDied();
        Talk(SAY_DEATH);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_ACID_SPRAY:
                DoCastVictim(SPELL_ACID_SPRAY);
                events.Repeat(15s, 23s);
                break;
            case EVENT_EXPLODING_BREAKER:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SPELL_EXPLODING_BREAKER);
                events.Repeat(4s, 12s);
                break;
            case EVENT_DOMINATION:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true))
                    DoCast(target, SPELL_DOMINATION);
                events.Repeat(120s);
                break;
            case EVENT_KNOCKDOWN:
                DoCastVictim(SPELL_KNOCKDOWN);
                events.Repeat(4s, 12s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_the_maker()
{
    RegisterBloodFurnaceCreatureAI(boss_the_maker);
}
