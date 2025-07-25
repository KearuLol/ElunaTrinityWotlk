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

#ifndef TRINITY_MAP_H
#define TRINITY_MAP_H

#include "Define.h"

#include "Cell.h"
#include "DynamicTree.h"
#include "GridDefines.h"
#include "GridRefManager.h"
#include "MapDefines.h"
#include "MapRefManager.h"
#include "MPSCQueue.h"
#include "ObjectGuid.h"
#include "Optional.h"
#include "SharedDefines.h"
#include "SpawnData.h"
#include "Timer.h"
#include "Transaction.h"
#include "UniqueTrackablePtr.h"
#include <bitset>
#include <list>
#include <memory>
#include <mutex>
#ifdef ELUNA
#include "LuaValue.h"
#endif

class Battleground;
class BattlegroundMap;
class CreatureGroup;
#ifdef ELUNA
class Eluna;
#endif
class GameObjectModel;
class Group;
class InstanceMap;
class InstanceSave;
class InstanceScript;
class MapInstanced;
class Object;
class Player;
class TempSummon;
class Transport;
class Unit;
class Weather;
class WorldObject;
class WorldPacket;
class WorldSession;
struct MapDifficulty;
struct MapEntry;
struct Position;
struct ScriptAction;
struct ScriptInfo;
struct SummonPropertiesEntry;
enum Difficulty : uint8;
enum WeatherState : uint32;

namespace Trinity { struct ObjectUpdater; }
namespace VMAP { enum class ModelIgnoreFlags : uint32; }
namespace G3D { class Plane; }

struct ScriptAction
{
    ObjectGuid sourceGUID;
    ObjectGuid targetGUID;
    ObjectGuid ownerGUID;                                   ///> owner of source if source is item
    ScriptInfo const* script;                               ///> pointer to static script data
};

/// Represents a map magic value of 4 bytes (used in versions)
union u_map_magic
{
    char asChar[4]; ///> Non-null terminated string
    uint32 asUInt;  ///> uint32 representation
};

// ******************************************
// Map file format defines
// ******************************************
struct map_fileheader
{
    u_map_magic mapMagic;
    uint32 versionMagic;
    u_map_magic buildMagic;
    uint32 areaMapOffset;
    uint32 areaMapSize;
    uint32 heightMapOffset;
    uint32 heightMapSize;
    uint32 liquidMapOffset;
    uint32 liquidMapSize;
    uint32 holesOffset;
    uint32 holesSize;
};

#define MAP_AREA_NO_AREA      0x0001

struct map_areaHeader
{
    uint32 fourcc;
    uint16 flags;
    uint16 gridArea;
};

#define MAP_HEIGHT_NO_HEIGHT            0x0001
#define MAP_HEIGHT_AS_INT16             0x0002
#define MAP_HEIGHT_AS_INT8              0x0004
#define MAP_HEIGHT_HAS_FLIGHT_BOUNDS    0x0008

struct map_heightHeader
{
    uint32 fourcc;
    uint32 flags;
    float  gridHeight;
    float  gridMaxHeight;
};

#define MAP_LIQUID_NO_TYPE    0x0001
#define MAP_LIQUID_NO_HEIGHT  0x0002

struct map_liquidHeader
{
    uint32 fourcc;
    uint8 flags;
    uint8 liquidFlags;
    uint16 liquidType;
    uint8  offsetX;
    uint8  offsetY;
    uint8  width;
    uint8  height;
    float  liquidLevel;
};

#define MAP_LIQUID_TYPE_NO_WATER    0x00
#define MAP_LIQUID_TYPE_WATER       0x01
#define MAP_LIQUID_TYPE_OCEAN       0x02
#define MAP_LIQUID_TYPE_MAGMA       0x04
#define MAP_LIQUID_TYPE_SLIME       0x08

#define MAP_ALL_LIQUIDS   (MAP_LIQUID_TYPE_WATER | MAP_LIQUID_TYPE_OCEAN | MAP_LIQUID_TYPE_MAGMA | MAP_LIQUID_TYPE_SLIME)

#define MAP_LIQUID_TYPE_DARK_WATER  0x10

class TC_GAME_API GridMap
{
    uint32  _flags;
    union{
        float* m_V9;
        uint16* m_uint16_V9;
        uint8* m_uint8_V9;
    };
    union{
        float* m_V8;
        uint16* m_uint16_V8;
        uint8* m_uint8_V8;
    };
    G3D::Plane* _minHeightPlanes;
    // Height level data
    float _gridHeight;
    float _gridIntHeightMultiplier;

    // Area data
    uint16* _areaMap;

    // Liquid data
    float _liquidLevel;
    uint16* _liquidEntry;
    uint8* _liquidFlags;
    float* _liquidMap;
    uint16 _gridArea;
    uint16 _liquidGlobalEntry;
    uint8 _liquidGlobalFlags;
    uint8 _liquidOffX;
    uint8 _liquidOffY;
    uint8 _liquidWidth;
    uint8 _liquidHeight;

    uint16* _holes;

    bool loadAreaData(FILE* in, uint32 offset, uint32 size);
    bool loadHeightData(FILE* in, uint32 offset, uint32 size);
    bool loadLiquidData(FILE* in, uint32 offset, uint32 size);
    bool loadHolesData(FILE* in, uint32 offset, uint32 size);
    bool isHole(int row, int col) const;

    // Get height functions and pointers
    typedef float (GridMap::*GetHeightPtr) (float x, float y) const;
    GetHeightPtr _gridGetHeight;
    float getHeightFromFloat(float x, float y) const;
    float getHeightFromUint16(float x, float y) const;
    float getHeightFromUint8(float x, float y) const;
    float getHeightFromFlat(float x, float y) const;

public:
    GridMap();
    ~GridMap();
    bool loadData(char const* filename);
    void unloadData();

    uint16 getArea(float x, float y) const;
    inline float getHeight(float x, float y) const {return (this->*_gridGetHeight)(x, y);}
    float getMinHeight(float x, float y) const;
    float getLiquidLevel(float x, float y) const;
    ZLiquidStatus GetLiquidStatus(float x, float y, float z, Optional<uint8> ReqLiquidType, LiquidData* data = 0, float collisionHeight = 2.03128f); // DEFAULT_COLLISION_HEIGHT in Object.h
};

#pragma pack(push, 1)

struct ZoneDynamicInfo
{
    ZoneDynamicInfo();

    uint32 MusicId;

    std::unique_ptr<Weather> DefaultWeather;
    WeatherState WeatherId;
    float Intensity;

    struct LightOverride
    {
        uint32 AreaLightId;
        uint32 OverrideLightId;
        uint32 TransitionMilliseconds;
    };
    std::vector<LightOverride> LightOverrides;
};

#pragma pack(pop)

#define MAX_HEIGHT            100000.0f                     // can be use for find ground height at surface
#define INVALID_HEIGHT       -100000.0f                     // for check, must be equal to VMAP_INVALID_HEIGHT, real value for unknown height is VMAP_INVALID_HEIGHT_VALUE
#define MAX_FALL_DISTANCE     250000.0f                     // "unlimited fall" to find VMap ground if it is available, just larger than MAX_HEIGHT - INVALID_HEIGHT
#define DEFAULT_HEIGHT_SEARCH     50.0f                     // default search distance to find height at nearby locations
#define MIN_UNLOAD_DELAY      1                             // immediate unload
#define MAP_INVALID_ZONE      0xFFFFFFFF

struct RespawnInfo; // forward declaration
struct CompareRespawnInfo
{
    bool operator()(RespawnInfo const* a, RespawnInfo const* b) const;
};
using ZoneDynamicInfoMap = std::unordered_map<uint32 /*zoneId*/, ZoneDynamicInfo>;
struct RespawnListContainer;
using RespawnInfoMap = std::unordered_map<ObjectGuid::LowType, RespawnInfo*>;
struct TC_GAME_API RespawnInfo
{
    virtual ~RespawnInfo();

    SpawnObjectType type;
    ObjectGuid::LowType spawnId;
    uint32 entry;
    time_t respawnTime;
    uint32 gridId;
};
inline bool CompareRespawnInfo::operator()(RespawnInfo const* a, RespawnInfo const* b) const
{
    if (a == b)
        return false;
    if (a->respawnTime != b->respawnTime)
        return (a->respawnTime > b->respawnTime);
    if (a->spawnId != b->spawnId)
        return a->spawnId < b->spawnId;
    ASSERT(a->type != b->type, "Duplicate respawn entry for spawnId (%u,%u) found!", a->type, a->spawnId);
    return a->type < b->type;
}

extern template class TypeUnorderedMapContainer<AllMapStoredObjectTypes, ObjectGuid>;
typedef TypeUnorderedMapContainer<AllMapStoredObjectTypes, ObjectGuid> MapStoredObjectTypesContainer;

class TC_GAME_API Map : public GridRefManager<NGridType>
{
    friend class MapReference;
    public:
        Map(uint32 id, time_t, uint32 InstanceId, uint8 SpawnMode, Map* _parent = nullptr);
        virtual ~Map();

        MapEntry const* GetEntry() const { return i_mapEntry; }

        // currently unused for normal maps
        bool CanUnload(uint32 diff)
        {
            if (!m_unloadTimer)
                return false;

            if (m_unloadTimer <= diff)
                return true;

            m_unloadTimer -= diff;
            return false;
        }

        virtual bool AddPlayerToMap(Player*);
        virtual void RemovePlayerFromMap(Player*, bool);

        template<class T> bool AddToMap(T *);
        template<class T> void RemoveFromMap(T *, bool);

        void VisitNearbyCellsOf(WorldObject* obj, TypeContainerVisitor<Trinity::ObjectUpdater, GridTypeMapContainer> &gridVisitor, TypeContainerVisitor<Trinity::ObjectUpdater, WorldTypeMapContainer> &worldVisitor);
        virtual void Update(uint32);

        float GetVisibilityRange() const { return m_VisibleDistance; }
        //function for setting up visibility distance for maps on per-type/per-Id basis
        virtual void InitVisibilityDistance();

        void PlayerRelocation(Player*, float x, float y, float z, float orientation);
        void CreatureRelocation(Creature* creature, float x, float y, float z, float ang, bool respawnRelocationOnFail = true);
        void GameObjectRelocation(GameObject* go, float x, float y, float z, float orientation, bool respawnRelocationOnFail = true);
        void DynamicObjectRelocation(DynamicObject* go, float x, float y, float z, float orientation);

        template<class T, class CONTAINER>
        void Visit(Cell const& cell, TypeContainerVisitor<T, CONTAINER>& visitor);

        bool IsRemovalGrid(float x, float y) const
        {
            GridCoord p = Trinity::ComputeGridCoord(x, y);
            return !getNGrid(p.x_coord, p.y_coord) || getNGrid(p.x_coord, p.y_coord)->GetGridState() == GRID_STATE_REMOVAL;
        }
        bool IsRemovalGrid(Position const& pos) const { return IsRemovalGrid(pos.GetPositionX(), pos.GetPositionY()); }

        bool IsGridLoaded(uint32 gridId) const { return IsGridLoaded(GridCoord(gridId % MAX_NUMBER_OF_GRIDS, gridId / MAX_NUMBER_OF_GRIDS)); }
        bool IsGridLoaded(float x, float y) const { return IsGridLoaded(Trinity::ComputeGridCoord(x, y)); }
        bool IsGridLoaded(Position const& pos) const { return IsGridLoaded(pos.GetPositionX(), pos.GetPositionY()); }

        bool GetUnloadLock(GridCoord const& p) const { return getNGrid(p.x_coord, p.y_coord)->getUnloadLock(); }
        void SetUnloadLock(GridCoord const& p, bool on) { getNGrid(p.x_coord, p.y_coord)->setUnloadExplicitLock(on); }
        void LoadGrid(float x, float y);
        void LoadAllCells();
        bool UnloadGrid(NGridType& ngrid, bool pForce);
        void GridMarkNoUnload(uint32 x, uint32 y);
        void GridUnmarkNoUnload(uint32 x, uint32 y);
        virtual void UnloadAll();

        void ResetGridExpiry(NGridType &grid, float factor = 1) const
        {
            grid.ResetTimeTracker(time_t(float(i_gridExpiry)*factor));
        }

        time_t GetGridExpiry(void) const { return i_gridExpiry; }
        uint32 GetId() const;

        static bool ExistMap(uint32 mapid, int gx, int gy);
        static bool ExistVMap(uint32 mapid, int gx, int gy);

        static void InitStateMachine();
        static void DeleteStateMachine();

        Map const* GetParent() const { return m_parentMap; }

        void GetFullTerrainStatusForPosition(uint32 phaseMask, float x, float y, float z, PositionFullTerrainStatus& data, Optional<uint8> reqLiquidType = {}, float collisionHeight = 2.03128f) const; // DEFAULT_COLLISION_HEIGHT in Object.h
        ZLiquidStatus GetLiquidStatus(uint32 phaseMask, float x, float y, float z, Optional<uint8> ReqLiquidType, LiquidData* data = nullptr, float collisionHeight = 2.03128f) const; // DEFAULT_COLLISION_HEIGHT in Object.h

        bool GetAreaInfo(uint32 phaseMask, float x, float y, float z, uint32& mogpflags, int32& adtId, int32& rootId, int32& groupId) const;
        uint32 GetAreaId(uint32 phaseMask, float x, float y, float z) const;
        uint32 GetAreaId(uint32 phaseMask, Position const& pos) const { return GetAreaId(phaseMask, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()); }
        uint32 GetZoneId(uint32 phaseMask, float x, float y, float z) const;
        uint32 GetZoneId(uint32 phaseMask, Position const& pos) const { return GetZoneId(phaseMask, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()); }
        void GetZoneAndAreaId(uint32 phaseMask, uint32& zoneid, uint32& areaid, float x, float y, float z) const;
        void GetZoneAndAreaId(uint32 phaseMask, uint32& zoneid, uint32& areaid, Position const& pos) const { GetZoneAndAreaId(phaseMask, zoneid, areaid, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ()); }

        float GetWaterLevel(float x, float y) const;
        bool IsInWater(uint32 phaseMask, float x, float y, float z, LiquidData* data = nullptr) const;
        bool IsUnderWater(uint32 phaseMask, float x, float y, float z) const;

        void MoveAllCreaturesInMoveList();
        void MoveAllGameObjectsInMoveList();
        void MoveAllDynamicObjectsInMoveList();
        void RemoveAllObjectsInRemoveList();
        virtual void RemoveAllPlayers();

        // used only in MoveAllCreaturesInMoveList and ObjectGridUnloader
        bool CreatureRespawnRelocation(Creature* c, bool diffGridOnly);
        bool GameObjectRespawnRelocation(GameObject* go, bool diffGridOnly);

        // assert print helper
        bool CheckGridIntegrity(Creature* c, bool moved) const;

        uint32 GetInstanceId() const { return i_InstanceId; }
        uint8 GetSpawnMode() const { return (i_spawnMode); }

        Trinity::unique_weak_ptr<Map> GetWeakPtr() const { return m_weakRef; }
        void SetWeakPtr(Trinity::unique_weak_ptr<Map> weakRef) { m_weakRef = std::move(weakRef); }

        enum EnterState
        {
            CAN_ENTER = 0,
            CANNOT_ENTER_ALREADY_IN_MAP = 1, // Player is already in the map
            CANNOT_ENTER_NO_ENTRY, // No map entry was found for the target map ID
            CANNOT_ENTER_UNINSTANCED_DUNGEON, // No instance template was found for dungeon map
            CANNOT_ENTER_DIFFICULTY_UNAVAILABLE, // Requested instance difficulty is not available for target map
            CANNOT_ENTER_NOT_IN_RAID, // Target instance is a raid instance and the player is not in a raid group
            CANNOT_ENTER_CORPSE_IN_DIFFERENT_INSTANCE, // Player is dead and their corpse is not in target instance
            CANNOT_ENTER_INSTANCE_BIND_MISMATCH, // Player's permanent instance save is not compatible with their group's current instance bind
            CANNOT_ENTER_TOO_MANY_INSTANCES, // Player has entered too many instances recently
            CANNOT_ENTER_MAX_PLAYERS, // Target map already has the maximum number of players allowed
            CANNOT_ENTER_ZONE_IN_COMBAT, // A boss encounter is currently in progress on the target map
            CANNOT_ENTER_UNSPECIFIED_REASON
        };
        virtual EnterState CannotEnter(Player* /*player*/) { return CAN_ENTER; }
        char const* GetMapName() const;

        // have meaning only for instanced map (that have set real difficulty)
        Difficulty GetDifficulty() const { return Difficulty(GetSpawnMode()); }
        bool IsRegularDifficulty() const;
        MapDifficulty const* GetMapDifficulty() const;

        bool Instanceable() const;
        bool IsWorldMap() const;
        bool IsDungeon() const;
        bool IsNonRaidDungeon() const;
        bool IsRaid() const;
        bool IsRaidOrHeroicDungeon() const;
        bool IsHeroic() const;
        bool Is25ManRaid() const;
        bool IsBattleground() const;
        bool IsBattleArena() const;
        bool IsBattlegroundOrArena() const;
        bool GetEntrancePos(int32& mapid, float& x, float& y) const;

        void AddObjectToRemoveList(WorldObject* obj);
        void AddObjectToSwitchList(WorldObject* obj, bool on);
        virtual void DelayedUpdate(uint32 diff);

        void resetMarkedCells() { marked_cells.reset(); }
        bool isCellMarked(uint32 pCellId) { return marked_cells.test(pCellId); }
        void markCell(uint32 pCellId) { marked_cells.set(pCellId); }

        bool HavePlayers() const { return !m_mapRefManager.isEmpty(); }
        uint32 GetPlayersCountExceptGMs() const;
        bool ActiveObjectsNearGrid(NGridType const& ngrid) const;

        void AddWorldObject(WorldObject* obj) { i_worldObjects.insert(obj); }
        void RemoveWorldObject(WorldObject* obj) { i_worldObjects.erase(obj); }

        void SendToPlayers(WorldPacket const* data) const;
        bool SendZoneMessage(uint32 zone, WorldPacket const* packet, WorldSession const* self = nullptr, uint32 team = 0) const;

        typedef MapRefManager PlayerList;
        PlayerList const& GetPlayers() const { return m_mapRefManager; }

        //per-map script storage
        void ScriptsStart(std::map<uint32, std::multimap<uint32, ScriptInfo>> const& scripts, uint32 id, Object* source, Object* target);
        void ScriptCommandStart(ScriptInfo const& script, uint32 delay, Object* source, Object* target);

        // must called with AddToWorld
        void AddToActive(WorldObject* obj);

        // must called with RemoveFromWorld
        void RemoveFromActive(WorldObject* obj);

        template<class T> void SwitchGridContainers(T* obj, bool on);
        std::unordered_map<ObjectGuid::LowType /*leaderSpawnId*/, CreatureGroup*> CreatureGroupHolder;

        void UpdateIteratorBack(Player* player);

        TempSummon* SummonCreature(uint32 entry, Position const& pos, SummonPropertiesEntry const* properties = nullptr, uint32 duration = 0, WorldObject* summoner = nullptr, uint32 spellId = 0, uint32 vehId = 0, ObjectGuid privateObjectOwner = ObjectGuid::Empty);
        void SummonCreatureGroup(uint8 group, std::list<TempSummon*>* list = nullptr);
        Player* GetPlayer(ObjectGuid const& guid);
        Corpse* GetCorpse(ObjectGuid const& guid);
        Creature* GetCreature(ObjectGuid const& guid);
        GameObject* GetGameObject(ObjectGuid const& guid);
        Creature* GetCreatureBySpawnId(ObjectGuid::LowType spawnId) const;
        GameObject* GetGameObjectBySpawnId(ObjectGuid::LowType spawnId) const;
        WorldObject* GetWorldObjectBySpawnId(SpawnObjectType type, ObjectGuid::LowType spawnId) const
        {
            switch (type)
            {
                case SPAWN_TYPE_CREATURE:
                    return reinterpret_cast<WorldObject*>(GetCreatureBySpawnId(spawnId));
                case SPAWN_TYPE_GAMEOBJECT:
                    return reinterpret_cast<WorldObject*>(GetGameObjectBySpawnId(spawnId));
                default:
                    return nullptr;
            }
        }
        Transport* GetTransport(ObjectGuid const& guid);
        DynamicObject* GetDynamicObject(ObjectGuid const& guid);
        Pet* GetPet(ObjectGuid const& guid);

        MapStoredObjectTypesContainer& GetObjectsStore() { return _objectsStore; }

        typedef std::unordered_multimap<ObjectGuid::LowType, Creature*> CreatureBySpawnIdContainer;
        CreatureBySpawnIdContainer& GetCreatureBySpawnIdStore() { return _creatureBySpawnIdStore; }
        CreatureBySpawnIdContainer const& GetCreatureBySpawnIdStore() const { return _creatureBySpawnIdStore; }

        typedef std::unordered_multimap<ObjectGuid::LowType, GameObject*> GameObjectBySpawnIdContainer;
        GameObjectBySpawnIdContainer& GetGameObjectBySpawnIdStore() { return _gameobjectBySpawnIdStore; }
        GameObjectBySpawnIdContainer const& GetGameObjectBySpawnIdStore() const { return _gameobjectBySpawnIdStore; }

        std::unordered_set<Corpse*> const* GetCorpsesInCell(uint32 cellId) const
        {
            auto itr = _corpsesByCell.find(cellId);
            if (itr != _corpsesByCell.end())
                return &itr->second;

            return nullptr;
        }

        Corpse* GetCorpseByPlayer(ObjectGuid const& ownerGuid) const
        {
            auto itr = _corpsesByPlayer.find(ownerGuid);
            if (itr != _corpsesByPlayer.end())
                return itr->second;

            return nullptr;
        }

        MapInstanced* ToMapInstanced() { if (Instanceable()) return reinterpret_cast<MapInstanced*>(this); return nullptr; }
        MapInstanced const* ToMapInstanced() const { if (Instanceable()) return reinterpret_cast<MapInstanced const*>(this); return nullptr; }

        InstanceMap* ToInstanceMap() { if (IsDungeon()) return reinterpret_cast<InstanceMap*>(this); else return nullptr;  }
        InstanceMap const* ToInstanceMap() const { if (IsDungeon()) return reinterpret_cast<InstanceMap const*>(this); return nullptr; }

        BattlegroundMap* ToBattlegroundMap() { if (IsBattlegroundOrArena()) return reinterpret_cast<BattlegroundMap*>(this); else return nullptr;  }
        BattlegroundMap const* ToBattlegroundMap() const { if (IsBattlegroundOrArena()) return reinterpret_cast<BattlegroundMap const*>(this); return nullptr; }

        float GetWaterOrGroundLevel(uint32 phasemask, float x, float y, float z, float* ground = nullptr, bool swim = false, float collisionHeight = 2.03128f) const; // DEFAULT_COLLISION_HEIGHT in Object.h
        float GetMinHeight(float x, float y) const;
        float GetHeight(float x, float y, float z, bool checkVMap = true, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const;
        float GetGridHeight(float x, float y) const;
        float GetHeight(Position const& pos, bool vmap = true, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const { return GetHeight(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), vmap, maxSearchDist); }
        float GetHeight(uint32 phasemask, float x, float y, float z, bool vmap = true, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const { return std::max<float>(GetHeight(x, y, z, vmap, maxSearchDist), GetGameObjectFloor(phasemask, x, y, z, maxSearchDist)); }
        float GetHeight(uint32 phasemask, Position const& pos, bool vmap = true, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const { return GetHeight(phasemask, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), vmap, maxSearchDist); }
        bool isInLineOfSight(float x1, float y1, float z1, float x2, float y2, float z2, uint32 phasemask, LineOfSightChecks checks, VMAP::ModelIgnoreFlags ignoreFlags) const;
        void Balance() { _dynamicTree.balance(); }
        void RemoveGameObjectModel(GameObjectModel const& model) { _dynamicTree.remove(model); }
        void InsertGameObjectModel(GameObjectModel const& model) { _dynamicTree.insert(model); }
        bool ContainsGameObjectModel(GameObjectModel const& model) const { return _dynamicTree.contains(model);}
        float GetGameObjectFloor(uint32 phasemask, float x, float y, float z, float maxSearchDist = DEFAULT_HEIGHT_SEARCH) const
        {
            return _dynamicTree.getHeight(x, y, z, maxSearchDist, phasemask);
        }
        bool getObjectHitPos(uint32 phasemask, float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float &ry, float& rz, float modifyDist);

        /*
            RESPAWN TIMES
        */
        time_t GetLinkedRespawnTime(ObjectGuid guid) const;
        time_t GetRespawnTime(SpawnObjectType type, ObjectGuid::LowType spawnId) const
        {
            auto const& map = GetRespawnMapForType(type);
            auto it = map.find(spawnId);
            return (it == map.end()) ? 0 : it->second->respawnTime;
        }
        time_t GetCreatureRespawnTime(ObjectGuid::LowType spawnId) const { return GetRespawnTime(SPAWN_TYPE_CREATURE, spawnId); }
        time_t GetGORespawnTime(ObjectGuid::LowType spawnId) const { return GetRespawnTime(SPAWN_TYPE_GAMEOBJECT, spawnId); }

        void UpdatePlayerZoneStats(uint32 oldZone, uint32 newZone);

        void SaveRespawnTime(SpawnObjectType type, ObjectGuid::LowType spawnId, uint32 entry, time_t respawnTime, uint32 gridId, CharacterDatabaseTransaction dbTrans = nullptr, bool startup = false);
        void SaveRespawnInfoDB(RespawnInfo const& info, CharacterDatabaseTransaction dbTrans = nullptr);
        void LoadRespawnTimes();
        void DeleteRespawnTimes() { UnloadAllRespawnInfos(); DeleteRespawnTimesInDB(GetId(), GetInstanceId()); }
        static void DeleteRespawnTimesInDB(uint16 mapId, uint32 instanceId);

        void LoadCorpseData();
        void DeleteCorpseData();
        void AddCorpse(Corpse* corpse);
        void RemoveCorpse(Corpse* corpse);
        Corpse* ConvertCorpseToBones(ObjectGuid const& ownerGuid, bool insignia = false);
        void RemoveOldCorpses();

        void SendInitTransports(Player* player);
        void SendRemoveTransports(Player* player);
        void SendZoneDynamicInfo(uint32 zoneId, Player* player) const;
        void SendZoneWeather(uint32 zoneId, Player* player) const;
        void SendZoneWeather(ZoneDynamicInfo const& zoneDynamicInfo, Player* player) const;
        void SendZoneText(uint32 zoneId, const char* text, WorldSession const* self = nullptr, uint32 team = 0) const;

        void SetZoneMusic(uint32 zoneId, uint32 musicId);
        Weather* GetOrGenerateZoneDefaultWeather(uint32 zoneId);
        void SetZoneWeather(uint32 zoneId, WeatherState weatherId, float intensity);
        void SetZoneOverrideLight(uint32 zoneId, uint32 areaLightId, uint32 overrideLightId, Milliseconds transitionTime);

        void UpdateAreaDependentAuras();

        template<HighGuid high>
        inline ObjectGuid::LowType GenerateLowGuid()
        {
            static_assert(ObjectGuidTraits<high>::MapSpecific, "Only map specific guid can be generated in Map context");
            return GetGuidSequenceGenerator(high).Generate();
        }

        template<HighGuid high>
        inline ObjectGuid::LowType GetMaxLowGuid()
        {
            static_assert(ObjectGuidTraits<high>::MapSpecific, "Only map specific guid can be retrieved in Map context");
            return GetGuidSequenceGenerator(high).GetNextAfterMaxUsed();
        }

        void AddUpdateObject(Object* obj)
        {
            _updateObjects.insert(obj);
        }

        void RemoveUpdateObject(Object* obj)
        {
            _updateObjects.erase(obj);
        }

        size_t GetActiveNonPlayersCount() const
        {
            return m_activeNonPlayers.size();
        }

        virtual std::string GetDebugInfo() const;

    private:
        void LoadMapAndVMap(int gx, int gy);
        void LoadVMap(int gx, int gy);
        void LoadMap(int gx, int gy, bool reload = false);
        void LoadMMap(int gx, int gy);
        GridMap* GetGrid(float x, float y);

        void SetTimer(uint32 t) { i_gridExpiry = t < MIN_GRID_DELAY ? MIN_GRID_DELAY : t; }

        void SendInitSelf(Player* player);

        bool CreatureCellRelocation(Creature* creature, Cell new_cell);
        bool GameObjectCellRelocation(GameObject* go, Cell new_cell);
        bool DynamicObjectCellRelocation(DynamicObject* go, Cell new_cell);

        template<class T> void InitializeObject(T* obj);
        void AddCreatureToMoveList(Creature* c, float x, float y, float z, float ang);
        void RemoveCreatureFromMoveList(Creature* c);
        void AddGameObjectToMoveList(GameObject* go, float x, float y, float z, float ang);
        void RemoveGameObjectFromMoveList(GameObject* go);
        void AddDynamicObjectToMoveList(DynamicObject* go, float x, float y, float z, float ang);
        void RemoveDynamicObjectFromMoveList(DynamicObject* go);

        bool _creatureToMoveLock;
        std::vector<Creature*> _creaturesToMove;

        bool _gameObjectsToMoveLock;
        std::vector<GameObject*> _gameObjectsToMove;

        bool _dynamicObjectsToMoveLock;
        std::vector<DynamicObject*> _dynamicObjectsToMove;

        bool IsGridLoaded(GridCoord const&) const;
        void EnsureGridCreated(GridCoord const&);
        void EnsureGridCreated_i(GridCoord const&);
        bool EnsureGridLoaded(Cell const&);
        void EnsureGridLoadedForActiveObject(Cell const&, WorldObject* object);

        void buildNGridLinkage(NGridType* pNGridType) { pNGridType->link(this); }

        NGridType* getNGrid(uint32 x, uint32 y) const
        {
            ASSERT(x < MAX_NUMBER_OF_GRIDS && y < MAX_NUMBER_OF_GRIDS, "x = %u, y = %u", x, y);
            return i_grids[x][y];
        }

        void setNGrid(NGridType* grid, uint32 x, uint32 y);
        void ScriptsProcess();

        void SendObjectUpdates();

    protected:
        void SetUnloadReferenceLock(GridCoord const& p, bool on) { getNGrid(p.x_coord, p.y_coord)->setUnloadReferenceLock(on); }

        std::mutex _mapLock;
        std::mutex _gridLock;

        MapEntry const* i_mapEntry;
        uint8 i_spawnMode;
        uint32 i_InstanceId;
        Trinity::unique_weak_ptr<Map> m_weakRef;
        uint32 m_unloadTimer;
        float m_VisibleDistance;
        DynamicMapTree _dynamicTree;

        MapRefManager m_mapRefManager;
        MapRefManager::iterator m_mapRefIter;

        int32 m_VisibilityNotifyPeriod;

        typedef std::set<WorldObject*> ActiveNonPlayers;
        ActiveNonPlayers m_activeNonPlayers;
        ActiveNonPlayers::iterator m_activeNonPlayersIter;

        // Objects that must update even in inactive grids without activating them
        typedef std::set<Transport*> TransportsContainer;
        TransportsContainer _transports;
        TransportsContainer::iterator _transportsUpdateIter;

    private:
        Player* _GetScriptPlayerSourceOrTarget(Object* source, Object* target, ScriptInfo const* scriptInfo) const;
        Creature* _GetScriptCreatureSourceOrTarget(Object* source, Object* target, ScriptInfo const* scriptInfo, bool bReverse = false) const;
        GameObject* _GetScriptGameObjectSourceOrTarget(Object* source, Object* target, ScriptInfo const* scriptInfo, bool bReverse = false) const;
        Unit* _GetScriptUnit(Object* obj, bool isSource, ScriptInfo const* scriptInfo) const;
        Player* _GetScriptPlayer(Object* obj, bool isSource, ScriptInfo const* scriptInfo) const;
        Creature* _GetScriptCreature(Object* obj, bool isSource, ScriptInfo const* scriptInfo) const;
        WorldObject* _GetScriptWorldObject(Object* obj, bool isSource, ScriptInfo const* scriptInfo) const;
        void _ScriptProcessDoor(Object* source, Object* target, ScriptInfo const* scriptInfo) const;
        GameObject* _FindGameObject(WorldObject* pWorldObject, ObjectGuid::LowType guid) const;

        time_t i_gridExpiry;

        //used for fast base_map (e.g. MapInstanced class object) search for
        //InstanceMaps and BattlegroundMaps...
        Map* m_parentMap;

        NGridType* i_grids[MAX_NUMBER_OF_GRIDS][MAX_NUMBER_OF_GRIDS];
        GridMap* GridMaps[MAX_NUMBER_OF_GRIDS][MAX_NUMBER_OF_GRIDS];
        std::bitset<TOTAL_NUMBER_OF_CELLS_PER_MAP*TOTAL_NUMBER_OF_CELLS_PER_MAP> marked_cells;

        //these functions used to process player/mob aggro reactions and
        //visibility calculations. Highly optimized for massive calculations
        void ProcessRelocationNotifies(const uint32 diff);

        bool i_scriptLock;
        std::set<WorldObject*> i_objectsToRemove;
        std::map<WorldObject*, bool> i_objectsToSwitch;
        std::set<WorldObject*> i_worldObjects;

        typedef std::multimap<time_t, ScriptAction> ScriptScheduleMap;
        ScriptScheduleMap m_scriptSchedule;

    public:
        void ProcessRespawns();
        void ApplyDynamicModeRespawnScaling(WorldObject const* obj, ObjectGuid::LowType spawnId, uint32& respawnDelay, uint32 mode) const;

    private:
        // if return value is true, we can respawn
        // if return value is false, reschedule the respawn to new value of info->respawnTime iff nonzero, delete otherwise
        // if return value is false and info->respawnTime is nonzero, it is guaranteed to be greater than time(NULL)
        bool CheckRespawn(RespawnInfo* info);
        void DoRespawn(SpawnObjectType type, ObjectGuid::LowType spawnId, uint32 gridId);
        bool AddRespawnInfo(RespawnInfo const& info);
        void UnloadAllRespawnInfos();
        RespawnInfo* GetRespawnInfo(SpawnObjectType type, ObjectGuid::LowType spawnId) const;
        void Respawn(RespawnInfo* info, CharacterDatabaseTransaction dbTrans = nullptr);
        void DeleteRespawnInfo(RespawnInfo* info, CharacterDatabaseTransaction dbTrans = nullptr);
        void DeleteRespawnInfoFromDB(SpawnObjectType type, ObjectGuid::LowType spawnId, CharacterDatabaseTransaction dbTrans = nullptr);

    public:
        void GetRespawnInfo(std::vector<RespawnInfo const*>& respawnData, SpawnObjectTypeMask types) const;
        void Respawn(SpawnObjectType type, ObjectGuid::LowType spawnId, CharacterDatabaseTransaction dbTrans = nullptr)
        {
            if (RespawnInfo* info = GetRespawnInfo(type, spawnId))
                Respawn(info, dbTrans);
        }
        void RemoveRespawnTime(SpawnObjectType type, ObjectGuid::LowType spawnId, CharacterDatabaseTransaction dbTrans = nullptr, bool alwaysDeleteFromDB = false)
        {
            if (RespawnInfo* info = GetRespawnInfo(type, spawnId))
                DeleteRespawnInfo(info, dbTrans);
            // Some callers might need to make sure the database doesn't contain any respawn time
            else if (alwaysDeleteFromDB)
                DeleteRespawnInfoFromDB(type, spawnId, dbTrans);
        }
        size_t DespawnAll(SpawnObjectType type, ObjectGuid::LowType spawnId);

        bool ShouldBeSpawnedOnGridLoad(SpawnObjectType type, ObjectGuid::LowType spawnId) const;
        template <typename T> bool ShouldBeSpawnedOnGridLoad(ObjectGuid::LowType spawnId) const { return ShouldBeSpawnedOnGridLoad(SpawnData::TypeFor<T>, spawnId); }

        SpawnGroupTemplateData const* GetSpawnGroupData(uint32 groupId) const;

        bool IsSpawnGroupActive(uint32 groupId) const;

        // Enable the spawn group, which causes all creatures in it to respawn (unless they have a respawn timer)
        // The force flag can be used to force spawning additional copies even if old copies are still around from a previous spawn
        bool SpawnGroupSpawn(uint32 groupId, bool ignoreRespawn = false, bool force = false, std::vector<WorldObject*>* spawnedObjects = nullptr);

        // Despawn all creatures in the spawn group if spawned, optionally delete their respawn timer, and disable the group
        bool SpawnGroupDespawn(uint32 groupId, bool deleteRespawnTimes = false, size_t* count = nullptr);

        // Disable the spawn group, which prevents any creatures in the group from respawning until re-enabled
        // This will not affect any already-present creatures in the group
        void SetSpawnGroupInactive(uint32 groupId) { SetSpawnGroupActive(groupId, false); }

        typedef std::function<void(Map*)> FarSpellCallback;
        void AddFarSpellCallback(FarSpellCallback&& callback);
        bool IsParentMap() const { return GetParent() == this; }
#ifdef ELUNA
        Eluna* GetEluna() const { return eluna.get(); }

        LuaVal lua_data = LuaVal({});
#endif
    private:
        // Type specific code for add/remove to/from grid
        template<class T>
        void AddToGrid(T* object, Cell const& cell);

        template<class T>
        void DeleteFromWorld(T*);

        void AddToActiveHelper(WorldObject* obj)
        {
            m_activeNonPlayers.insert(obj);
        }

        void RemoveFromActiveHelper(WorldObject* obj)
        {
            // Map::Update for active object in proccess
            if (m_activeNonPlayersIter != m_activeNonPlayers.end())
            {
                ActiveNonPlayers::iterator itr = m_activeNonPlayers.find(obj);
                if (itr == m_activeNonPlayers.end())
                    return;
                if (itr == m_activeNonPlayersIter)
                    ++m_activeNonPlayersIter;
                m_activeNonPlayers.erase(itr);
            }
            else
                m_activeNonPlayers.erase(obj);
        }

        std::unique_ptr<RespawnListContainer> _respawnTimes;
        RespawnInfoMap       _creatureRespawnTimesBySpawnId;
        RespawnInfoMap       _gameObjectRespawnTimesBySpawnId;
        RespawnInfoMap& GetRespawnMapForType(SpawnObjectType type)
        {
            switch (type)
            {
                default:
                    ABORT();
                case SPAWN_TYPE_CREATURE:
                    return _creatureRespawnTimesBySpawnId;
                case SPAWN_TYPE_GAMEOBJECT:
                    return _gameObjectRespawnTimesBySpawnId;
            }
        }
        RespawnInfoMap const& GetRespawnMapForType(SpawnObjectType type) const
        {
            switch (type)
            {
                default:
                    ABORT();
                case SPAWN_TYPE_CREATURE:
                    return _creatureRespawnTimesBySpawnId;
                case SPAWN_TYPE_GAMEOBJECT:
                    return _gameObjectRespawnTimesBySpawnId;
            }
        }

        void SetSpawnGroupActive(uint32 groupId, bool state);
        std::unordered_set<uint32> _toggledSpawnGroupIds;

        uint32 _respawnCheckTimer;
        std::unordered_map<uint32, uint32> _zonePlayerCountMap;

        ZoneDynamicInfoMap _zoneDynamicInfo;
        IntervalTimer _weatherUpdateTimer;

        ObjectGuidGenerator& GetGuidSequenceGenerator(HighGuid high);

        std::map<HighGuid, std::unique_ptr<ObjectGuidGenerator>> _guidGenerators;
        MapStoredObjectTypesContainer _objectsStore;
        CreatureBySpawnIdContainer _creatureBySpawnIdStore;
        GameObjectBySpawnIdContainer _gameobjectBySpawnIdStore;
        std::unordered_map<uint32/*cellId*/, std::unordered_set<Corpse*>> _corpsesByCell;
        std::unordered_map<ObjectGuid, Corpse*> _corpsesByPlayer;
        std::unordered_set<Corpse*> _corpseBones;

        std::unordered_set<Object*> _updateObjects;

        MPSCQueue<FarSpellCallback> _farSpellCallbacks;
#ifdef ELUNA
        std::unique_ptr<Eluna> eluna;
#endif
};

enum InstanceResetMethod
{
    INSTANCE_RESET_ALL,
    INSTANCE_RESET_CHANGE_DIFFICULTY,
    INSTANCE_RESET_GLOBAL,
    INSTANCE_RESET_GROUP_DISBAND,
    INSTANCE_RESET_GROUP_JOIN,
    INSTANCE_RESET_RESPAWN_DELAY
};

class TC_GAME_API InstanceMap : public Map
{
    public:
        InstanceMap(uint32 id, time_t, uint32 InstanceId, uint8 SpawnMode, Map* _parent, TeamId InstanceTeam);
        ~InstanceMap();
        bool AddPlayerToMap(Player*) override;
        void RemovePlayerFromMap(Player*, bool) override;
        void Update(uint32) override;
        void CreateInstanceData(bool load);
        bool Reset(uint8 method);
        uint32 GetScriptId() const { return i_script_id; }
        std::string const& GetScriptName() const;
        InstanceScript* GetInstanceScript() { return i_data; }
        InstanceScript const* GetInstanceScript() const { return i_data; }
        void PermBindAllPlayers();
        void UnloadAll() override;
        EnterState CannotEnter(Player* player) override;
        void SendResetWarnings(uint32 timeLeft) const;
        void SetResetSchedule(bool on);

        /* this checks if any players have a permanent bind (included reactivatable expired binds) to the instance ID
        it needs a DB query, so use sparingly */
        bool HasPermBoundPlayers() const;
        uint32 GetMaxPlayers() const;
        uint32 GetMaxResetDelay() const;
        TeamId GetTeamIdInInstance() const { return i_script_team; }
        Team GetTeamInInstance() const { return i_script_team == TEAM_ALLIANCE ? ALLIANCE : HORDE; }

        virtual void InitVisibilityDistance() override;

        std::string GetDebugInfo() const override;
    private:
        bool m_resetAfterUnload;
        bool m_unloadWhenEmpty;
        InstanceScript* i_data;
        uint32 i_script_id;
        TeamId i_script_team;
};

class TC_GAME_API BattlegroundMap : public Map
{
    public:
        BattlegroundMap(uint32 id, time_t, uint32 InstanceId, Map* _parent, uint8 spawnMode);
        ~BattlegroundMap();

        bool AddPlayerToMap(Player*) override;
        void RemovePlayerFromMap(Player*, bool) override;
        EnterState CannotEnter(Player* player) override;
        void SetUnload();
        //void UnloadAll(bool pForce);
        void RemoveAllPlayers() override;

        virtual void InitVisibilityDistance() override;
        Battleground* GetBG() { return m_bg; }
        void SetBG(Battleground* bg) { m_bg = bg; }
    private:
        Battleground* m_bg;
};

template<class T, class CONTAINER>
inline void Map::Visit(Cell const& cell, TypeContainerVisitor<T, CONTAINER>& visitor)
{
    const uint32 x = cell.GridX();
    const uint32 y = cell.GridY();
    const uint32 cell_x = cell.CellX();
    const uint32 cell_y = cell.CellY();

    if (!cell.NoCreate())
        EnsureGridLoaded(cell);

    NGridType* grid = getNGrid(x, y);
    if (grid && grid->isGridObjectDataLoaded())
        grid->VisitGrid(cell_x, cell_y, visitor);
}
#endif
