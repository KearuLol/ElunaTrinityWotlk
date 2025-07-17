#include <Bag.h>
#include <Chat.h>
#include <DatabaseEnv.h>
#include <Log.h>
#include <ObjectMgr.h>
#include <WorldSession.h>
#include <Mail.h>

#include "SpellModifier/SpellModifier.h"
#include "Teleporter/Teleporter.h"

class CustomWorldScripts : public WorldScript
{
public:
    CustomWorldScripts() : WorldScript("CustomWorldScripts") {}

    void OnStartup() override
    {
        // Loads Teleport System
        //sTeleSystem.Load();

        // Loads Spell Modifier
        sSpellModifier.Load();
    }
};

class CustomCommandScripts : public CommandScript
{
public:
    CustomCommandScripts() : CommandScript("CustomCommandScripts") {}

    Trinity::ChatCommands::ChatCommandTable GetCommands() const override
    {
        static Trinity::ChatCommands::ChatCommandTable ListCommandTable =
        {
            { "inventory",          HandleListInventoryCommand,         rbac::RBAC_ROLE_GAMEMASTER,         Trinity::ChatCommands::Console::Yes },
        };
        static Trinity::ChatCommands::ChatCommandTable ReloadCommandTable =
        {
            { "spell_modifier",     HandleReloadSpellModifierCommand,   rbac::RBAC_ROLE_ADMINISTRATOR,      Trinity::ChatCommands::Console::Yes },
            { "teleport_system",    HandleReloadTeleportSystemCommand,  rbac::RBAC_ROLE_ADMINISTRATOR,      Trinity::ChatCommands::Console::Yes },
            { "upgrade_system",     HandleReloadUpgradeSystemCommand,   rbac::RBAC_ROLE_ADMINISTRATOR,      Trinity::ChatCommands::Console::Yes },
        };
        //static Trinity::ChatCommands::ChatCommandTable WorldCommandTable =
        //{
        //    { "disable",     HandleDisableWorldChatCommand,             rbac::RBAC_ROLE_PLAYER,             Trinity::ChatCommands::Console::No },
        //    { "enable",      HandleEnableWorldChatCommand,              rbac::RBAC_ROLE_PLAYER,             Trinity::ChatCommands::Console::No },
        //};
        static std::vector<ChatCommand> CustomCommandTable =
        {
            { "buff",               HandleBuffCommand,                  rbac::RBAC_ROLE_PLAYER,             Trinity::ChatCommands::Console::No },
            { "UpdateTele",         HandleUpdateTeleCommand,            rbac::RBAC_ROLE_GAMEMASTER,         Trinity::ChatCommands::Console::No },
            { "list",               ListCommandTable },
            { "reload",             ReloadCommandTable },
            //{ "world",              WorldCommandTable },
        };
        return CustomCommandTable;
    }

    // Buff Command

    static bool HandleBuffCommand(ChatHandler* handler, const char* /**/)
    {
        Player* p = handler->GetSession()->GetPlayer();
        if (p->InBattleground() || p->InArena())
        {
            ChatHandler(p->GetSession()).SendNotify("You can't use this command in pvp zones.");
            return true;
        }

        handler->SendNotify("You have been buffed, enjoy!");
        return true;
    }

    static bool HandleUpdateTeleCommand(ChatHandler* handler, Optional<uint16> voteID)
    {
        if (handler && handler->GetPlayer())
        {
            std::string QUERY = "UPDATE `teleport_locations` SET `Map_ID` = " + std::to_string(handler->GetPlayer()->GetMapId());
            QUERY += ", `Pos_X` = " + std::to_string(handler->GetPlayer()->GetPositionX());
            QUERY += ", `Pos_Y` = " + std::to_string(handler->GetPlayer()->GetPositionY());
            QUERY += ", `Pos_Z` = " + std::to_string(handler->GetPlayer()->GetPositionZ());
            QUERY += ", `Pos_O` = " + std::to_string(handler->GetPlayer()->GetOrientation());
            if (voteID)
            {
                QUERY += " WHERE `ID` = " + std::to_string(voteID.value());
                CustomDatabase.PQuery("{}", QUERY);
            }
            else
                handler->SendSysMessage("Incorrect ID.");
        }

        return true;
    }

    // List Commands

    static bool HandleListInventoryCommand(ChatHandler* handler, Optional<Trinity::ChatCommands::PlayerIdentifier> targetIdentifier)
    {
        // Checks for target, or <name> 
        if (!targetIdentifier)
            targetIdentifier = Trinity::ChatCommands::PlayerIdentifier::FromTarget(handler);
        if (!targetIdentifier)
        {
            handler->PSendSysMessage("You must target a player, or type the name of character.");
            return true;
        }

        Player* target = targetIdentifier->GetConnectedPlayer();

        // If targeted player is online
        if (target)
        {
            std::string Output = "(Account:" + std::to_string(target->GetSession()->GetAccountId()) + " Guid:" + std::to_string(target->GetGUID()) + "): " + target->GetSession()->GetPlayerName() + " has ";

            for (uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
                if (Item* pItem = target->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    handler->PSendSysMessage("%s%sx%u - %s", Output, (handler->IsConsole() ? "[" + std::to_string(pItem->GetEntry()) + "] [" + pItem->GetTemplate()->Name1 + "]" : pItem->GetItemLink()), pItem->GetCount(), i >= INVENTORY_SLOT_BAG_END ? "Bag" : "Equipped");

            for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; i++)
                for (uint8 j = 0; j < MAX_BAG_SIZE; j++)
                    if (Item* pItem = target->GetItemByPos(((i << 8) | j)))
                        handler->PSendSysMessage("%s%sx%u - Bag", Output, (handler->IsConsole() ? "[" + std::to_string(pItem->GetEntry()) + "] [" + pItem->GetTemplate()->Name1 + "]" : pItem->GetItemLink()), pItem->GetCount());

            for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_ITEM_END; i++)
                if (Item* pItem = target->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    handler->PSendSysMessage("%s%sx%u - Bank", Output, (handler->IsConsole() ? "[" + std::to_string(pItem->GetEntry()) + "] [" + pItem->GetTemplate()->Name1 + "]" : pItem->GetItemLink()), pItem->GetCount());

            for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; i++)
                if (Item* pItem = target->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                {
                    handler->PSendSysMessage("%s%sx%u - Bank", Output, (handler->IsConsole() ? "[" + std::to_string(pItem->GetEntry()) + "] [" + pItem->GetTemplate()->Name1 + "]" : pItem->GetItemLink()), pItem->GetCount());

                    for (uint8 j = 0; j < MAX_BAG_SIZE; j++)
                        if (Item* pItem2 = target->GetItemByPos(i, j))
                            handler->PSendSysMessage("%s%sx%u - Bank", Output, (handler->IsConsole() ? "[" + std::to_string(pItem->GetEntry()) + "] [" + pItem2->GetTemplate()->Name1 + "]" : pItem2->GetItemLink()), pItem2->GetCount());
                }

            for (uint8 i = CURRENCYTOKEN_SLOT_START; i < CURRENCYTOKEN_SLOT_END; i++)
                if (Item* pItem = target->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
                    handler->PSendSysMessage("%s%sx%u - Currency", Output, (handler->IsConsole() ? "[" + std::to_string(pItem->GetEntry()) + "] [" + pItem->GetTemplate()->Name1 + "]" : pItem->GetItemLink()), pItem->GetCount());

            /* - Lists Items In Mail
            const PlayerMails& mails = target->GetMails();
            for (Mail* mail : mails)
            {
                if (!mail)
                    continue;

                if (mail->HasItems())
                {
                    for (auto &item : mail->items)
                    {
                        item.item_template;
                    }
                }
            }
            */

            return true;
        }

        // if target player is offline
        CustomDatabasePreparedStatement* stmt = CustomDatabase.GetPreparedStatement(CUSTOM_CHAR_SEL_INVENTORY);
        stmt->setString(0, targetIdentifier->GetName());
        PreparedQueryResult queryResult = CustomDatabase.Query(stmt);
        if (queryResult)
        {
            do
            {
                Field* fields = queryResult->Fetch();

                uint32 item_entry = fields[0].GetUInt32();
                std::string item_name = fields[1].GetString();
                uint32 item_count = fields[2].GetUInt32();
                std::string char_name = fields[3].GetString();
                std::string item_enchants = fields[4].GetString();
                int16 RandomPropertyID = fields[5].GetInt16();
                uint16 CharLevel = fields[6].GetUInt16();
                uint32 acc_guid = fields[7].GetUInt32();
                uint32 char_guid = fields[8].GetUInt32();
                bool IsMail = fields[9].GetBool();

                uint16 ctr = 0;
                std::ostringstream ssEnchants;
                std::string temp;
                std::string RandomSuffixFactor;

                for (auto& s : item_enchants)
                {
                    if (s != ' ')
                        temp += s;
                    else
                    {
                        switch (ctr)
                        {
                        case 0:
                        case 6:
                        case 9:
                        case 12:
                        case 15:
                            ssEnchants << ":" << temp;
                            break;
                        case 21:
                            RandomSuffixFactor = temp;
                            break;
                        }

                        temp.clear();
                        ctr++;
                    }
                }

                ssEnchants << ":" << RandomPropertyID << ":" << RandomSuffixFactor << ":" << CharLevel;

                std::ostringstream ItemLink;
                if (!handler->IsConsole())
                    ItemLink << "|c" << std::hex << ItemQualityColors[sObjectMgr->GetItemTemplate(item_entry)->Quality] << std::dec << "|Hitem:" << item_entry << ssEnchants.str() << "|h[" << item_name << "]|h|r";
                else
                    ItemLink << "[" << item_entry << "] [" << item_name << "]";

                std::string IsInMail = IsMail ? (" - Mail") : "";

                handler->PSendSysMessage("(Account:%u Guid:%u): %s has %sx%u%s", acc_guid, char_guid, char_name, ItemLink.str(), item_count, IsInMail);
            } while (queryResult->NextRow());
        }
        else
            handler->PSendSysMessage("Player %s not found, or doesn't have any items.", targetIdentifier->GetName());

        return true;
    }

    // Reload Commands

    static bool HandleReloadSpellModifierCommand(ChatHandler* handler, char const* /*args*/)
    {
        sSpellModifier.Load();
        handler->SendGlobalGMSysMessage("Spell Modifier data reloaded.");
        return true;
    }

    static bool HandleReloadTeleportSystemCommand(ChatHandler* handler, const char* /**/)
    {
        TC_LOG_INFO("misc", "Reloading teleport_locations tables...");
        //sTeleSystem.Load();
        handler->SendGlobalGMSysMessage("Teleport System reloaded.");
        return true;
    }

    static bool HandleReloadUpgradeSystemCommand(ChatHandler* handler, const char* /**/)
    {
        TC_LOG_INFO("misc", "Reloading item_upgrades tables...");
        //sUpgradeSystem.Load();
        handler->SendGlobalGMSysMessage("Upgrade System reloaded.");
        return true;
    }

    static bool HandleDisableWorldChatCommand(ChatHandler* handler)
    {
        //sPlayerInfo.UpdateWorldChat(handler->GetPlayer()->GetGUID(), false);
        handler->PSendSysMessage("You have disabled world chat.");
        return true;
    }

    static bool HandleEnableWorldChatCommand(ChatHandler* handler)
    {
        //sPlayerInfo.UpdateWorldChat(handler->GetPlayer()->GetGUID(), true);
        handler->PSendSysMessage("You have enabled world chat.");
        return true;
    }
};

void AddSC_CustomEventScripts()
{
    new CustomWorldScripts();
    new CustomCommandScripts();
}
