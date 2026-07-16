#include "common.h"
#include <memory>
#include <string.h>
#include <algorithm>
#include "json/ArduinoJson.h"
#include "fat/File.h"
#include "core/task/TaskQueue.h"
#include "JsonGameDataService.h"

#pragma GCC optimize("Os")

#define GAME_DATA_FILE_PATH   "/_pico/gamedata.json"
#define KEY_GAMES             "games"
#define KEY_FAVORITE          "favorite"
#define KEY_LAUNCH_COUNT      "launchCount"
#define KEY_LAST_PLAYED       "lastPlayed"

// ArduinoJson silently drops data when its pool is exhausted, so the pool is
// sized from the entry count (write) or file size (read) instead of a fixed
// JSON_RESERVED_SIZE like settings.json uses.
// ~160 bytes/entry = object node + favorite/launchCount/lastPlayed keys and
// values + up to 96 chars of fileName key.
#define JSON_POOL_BASE_SIZE       4096
#define JSON_POOL_PER_ENTRY       160
#define JSON_POOL_READ_MAX        (96 * 1024)

static u32 writePoolSize(u32 entryCount)
{
    return JSON_POOL_BASE_SIZE + entryCount * JSON_POOL_PER_ENTRY;
}

JsonGameDataService::JsonGameDataService()
{
    Load();
}

GameDataEntry* JsonGameDataService::Find(const char* fileName)
{
    for (u32 i = 0; i < _entryCount; i++)
    {
        if (!strcasecmp(_entries[i].fileName.GetString(), fileName))
            return &_entries[i];
    }
    return nullptr;
}

const GameDataEntry* JsonGameDataService::GetEntry(const char* fileName) const
{
    return const_cast<JsonGameDataService*>(this)->Find(fileName);
}

GameDataEntry& JsonGameDataService::GetOrCreateEntry(const char* fileName)
{
    if (auto* existing = Find(fileName))
        return *existing;

    if (_entryCount == _entryCapacity)
    {
        u32 newCapacity = _entryCapacity == 0 ? 16 : _entryCapacity * 2;
        auto newEntries = std::make_unique<GameDataEntry[]>(newCapacity);
        for (u32 i = 0; i < _entryCount; i++)
            newEntries[i] = _entries[i];
        _entries = std::move(newEntries);
        _entryCapacity = newCapacity;
    }

    auto& entry = _entries[_entryCount++];
    entry = GameDataEntry();
    entry.fileName = fileName;
    return entry;
}

void JsonGameDataService::ToggleFavorite(const char* fileName)
{
    auto& entry = GetOrCreateEntry(fileName);
    entry.favorite = !entry.favorite;
    _version++;
}

void JsonGameDataService::RecordLaunch(const char* fileName, const char* lastPlayedDateTime)
{
    auto& entry = GetOrCreateEntry(fileName);
    entry.launchCount++;
    entry.lastPlayed = lastPlayedDateTime;
    _version++;
}

void JsonGameDataService::SaveAsync(TaskQueueBase* ioTaskQueue)
{
    // serialization happens on the calling (main) thread: ArduinoJson links
    // pointers into _entries, which a concurrent mutation could reallocate if
    // this ran on the IO thread. Only the finished byte buffer crosses threads.
    DynamicJsonDocument json(writePoolSize(_entryCount));
    auto games = json[KEY_GAMES].to<JsonObject>();
    for (u32 i = 0; i < _entryCount; i++)
    {
        const auto& entry = _entries[i];
        // entries reset back to all-default state are pruned on write
        if (!entry.favorite && entry.launchCount == 0)
            continue;
        auto game = games[entry.fileName.GetString()].to<JsonObject>();
        if (entry.favorite)
            game[KEY_FAVORITE] = true;
        if (entry.launchCount > 0)
            game[KEY_LAUNCH_COUNT] = entry.launchCount;
        if (entry.lastPlayed.GetString()[0] != 0)
            game[KEY_LAST_PLAYED] = entry.lastPlayed.GetString();
    }
    if (json.overflowed())
    {
        LOG_ERROR("Game data json pool exhausted, not saving\n");
        return;
    }

    u32 outputSize = measureJsonPretty(json);
    u8* fileData = new(cache_align) u8[outputSize];
    serializeJsonPretty(json, fileData, outputSize);

    ioTaskQueue->Enqueue([fileData, outputSize] (const vu8& cancelRequested)
    {
        const auto file = std::make_unique<File>();
        if (file->Open(GAME_DATA_FILE_PATH, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        {
            LOG_ERROR("Couldn't open game data file for writing\n");
        }
        else
        {
            u32 bytesWritten;
            if (file->Write(fileData, outputSize, bytesWritten) != FR_OK || bytesWritten != outputSize)
                LOG_ERROR("Error while writing game data file\n");
            else
                LOG_DEBUG("Game data file written\n");
        }
        delete[] fileData;
        return TaskResult<void>::Completed();
    });
}

void JsonGameDataService::Load()
{
    const auto file = std::make_unique<File>();
    if (file->Open(GAME_DATA_FILE_PATH, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return;

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
        return;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u32 bytesRead = 0;
    if (file->Read(fileData.get(), fileSize, bytesRead) != FR_OK)
        return;

    DynamicJsonDocument json(std::clamp<u32>(fileSize * 3, 2 * JSON_POOL_BASE_SIZE, JSON_POOL_READ_MAX));
    if (deserializeJson(json, fileData.get(), fileSize) != DeserializationError::Ok)
    {
        LOG_ERROR("Couldn't parse game data file\n");
        return;
    }

    auto games = json[KEY_GAMES].as<JsonObjectConst>();
    if (games.isNull())
        return;
    for (auto item : games)
    {
        auto& entry = GetOrCreateEntry(item.key().c_str());
        entry.favorite = item.value()[KEY_FAVORITE] | false;
        entry.launchCount = item.value()[KEY_LAUNCH_COUNT] | 0u;
        entry.lastPlayed = item.value()[KEY_LAST_PLAYED] | "";
    }
}
