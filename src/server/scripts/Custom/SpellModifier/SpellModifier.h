#pragma once

class ModSpellInfo
{
public:
    float PlayerModifier = 100.0f;
    float CreatureModifier = 100.0f;
    float HealingModifier = 100.0f;
    std::string Comment;
};


class SpellDamageModifier
{
public:

    static SpellDamageModifier& instance()
    {
        static SpellDamageModifier SpDmgModInstance;
        return SpDmgModInstance;
    }

    void ModifySpellDamage(float& damagePct, uint32 spellId, bool victimIsPlayer);
    void ModifySpellHealing(float& healingPct, uint32 spellId);
    void ModifyMeleeDamage(float& damagePct, uint32 spellId, bool victimIsPlayer);

    void Load();

private:
    std::unordered_map<uint32, ModSpellInfo> ModifiedSpells;
};

#define sSpellModifier SpellDamageModifier::instance()
