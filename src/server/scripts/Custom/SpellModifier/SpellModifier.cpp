#include <DatabaseEnv.h>
#include <DBCStores.h>
#include <Log.h>
#include <SpellMgr.h>
#include <Unit.h>

#include "SpellModifier.h"

void SpellDamageModifier::ModifySpellDamage(float& damagePct, uint32 spellId, bool victimIsPlayer)
{
    if (ModifiedSpells.find(spellId) != ModifiedSpells.end())
    {
        float mValue = victimIsPlayer ? ModifiedSpells[spellId].PlayerModifier : ModifiedSpells[spellId].CreatureModifier;
        damagePct *= (mValue * 0.01f);
    }
}

void SpellDamageModifier::ModifySpellHealing(float& healingPct, uint32 spellId)
{
    if (ModifiedSpells.find(spellId) != ModifiedSpells.end())
    {
        float mValue = ModifiedSpells[spellId].HealingModifier;
        healingPct *= (mValue * 0.01f);
    }
}

void SpellDamageModifier::ModifyMeleeDamage(float& damagePct, uint32 spellId, bool victimIsPlayer)
{
    if (ModifiedSpells.find(spellId) != ModifiedSpells.end())
    {
        float mValue = victimIsPlayer ? ModifiedSpells[spellId].PlayerModifier : ModifiedSpells[spellId].CreatureModifier;
        damagePct *= (mValue * 0.01f);
    }
}

void SpellDamageModifier::Load()
{
    TC_LOG_INFO("server.loading", "Loading Damage Modifiers...");
    ModifiedSpells.clear();
    uint32 msStartTime = getMSTime();
    int nCounter = 0;

    CustomDatabasePreparedStatement* stmt = CustomDatabase.GetPreparedStatement(CUSTOM_SEL_SPELL_MODIFIER);
    PreparedQueryResult res = CustomDatabase.Query(stmt);
    if (res)
    {
        do
        {
            Field* pField = res->Fetch();
            ModSpellInfo info{};

            uint32 SpellID = pField[0].GetUInt32();;
            info.PlayerModifier = pField[1].GetFloat();
            info.CreatureModifier = pField[2].GetFloat();
            info.HealingModifier = pField[3].GetFloat();
            info.Comment = pField[4].GetString();

            if (ModifiedSpells.find(SpellID) != ModifiedSpells.end())
            {
                TC_LOG_ERROR("server.loading", "Spell ID {} exists in the database multiple times...", SpellID);
                continue;
            }

            if (info.CreatureModifier < 0.0f || info.PlayerModifier < 0.0f || info.HealingModifier < 0.0f)
            {
                TC_LOG_ERROR("server.loading", "Spell ID {} Modifier is negative...", SpellID);
                continue;
            }

            ModifiedSpells.emplace(SpellID, info);
            nCounter++;

        } while (res->NextRow());
    }
    TC_LOG_INFO("server.loading", "Loaded {} Modified Spells in {} ms", nCounter, GetMSTimeDiffToNow(msStartTime));
}
