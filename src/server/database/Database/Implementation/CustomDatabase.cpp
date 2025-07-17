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

#include "CustomDatabase.h"
#include "MySQLPreparedStatement.h"

void CustomDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_CUSTOMDATABASE_STATEMENTS);

    PrepareStatement(CUSTOM_SEL_SPELL_MODIFIER, "SELECT SpellID, PvpModifier, PveModifier, HealModifier, Comment FROM spell_modifier", CONNECTION_SYNCH);
    PrepareStatement(CUSTOM_CHAR_SEL_INVENTORY, "SELECT ii.`itemEntry`, it.`name` AS itemName, ii.`count`, ch.`name` AS CharName, ii.`enchantments`, ii.`randomPropertyId`, ch.`level`, ch.`account`, ii.`owner_guid`, IF(mi.`item_guid` IS NOT NULL, 1, 0) AS IsMail FROM characters.`item_instance` ii INNER JOIN characters.`characters` ch ON ch.`guid` = ii.`owner_guid` INNER JOIN world.`item_template` it ON ii.`itemEntry` = it.`entry` LEFT JOIN characters.`mail_items` mi ON mi.`item_guid` = ii.`guid` WHERE LOWER(ch.`name`) LIKE LOWER(?) ORDER BY ii.`itemEntry`;", CONNECTION_SYNCH);
}

CustomDatabaseConnection::CustomDatabaseConnection(MySQLConnectionInfo& connInfo) : MySQLConnection(connInfo)
{
}

CustomDatabaseConnection::CustomDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* q, MySQLConnectionInfo& connInfo) : MySQLConnection(q, connInfo)
{
}

CustomDatabaseConnection::~CustomDatabaseConnection()
{
}
