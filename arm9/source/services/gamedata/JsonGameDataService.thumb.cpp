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
#define KEY_GAME_CODE         "gameCode"
#define KEY_FAVORITE          "favorite"
#define KEY_COMPLETED         "completed"
#define KEY_LAUNCH_COUNT      "launchCount"
#define KEY_PLAY_MINUTES      "playMinutes"
#define KEY_LAST_PLAYED       "lastPlayed"
#define KEY_PATH              "path"
#define KEY_SESSION_GAME      "sessionGame"
#define KEY_SESSION_GAME_CODE "sessionGameCode"
#define KEY_SESSION_START     "sessionStart"

#define SESSION_MAX_MINUTES   (6 * 60)

// ArduinoJson silently drops data when its pool is exhausted, so the pool is
// sized from the entry count (write) or file size (read) instead of a fixed
// JSON_RESERVED_SIZE like settings.json uses.
// ~440 bytes/entry = object node + favorite/completed/launchCount/lastPlayed/
// path keys and values + up to 96 chars of fileName key + up to 256 of path.
#define JSON_POOL_BASE_SIZE       4096
#define JSON_POOL_PER_ENTRY       440
#define JSON_POOL_READ_MAX        (96 * 1024)

static u32 writePoolSize(u32 entryCount)
{
    return JSON_POOL_BASE_SIZE + entryCount * JSON_POOL_PER_ENTRY;
}

/// @brief Parses "YYYY-MM-DD HH:MM" into absolute minutes (days-from-civil
///        algorithm), so sessions crossing midnight or month ends work.
static bool parseDateTime(const char* text, s64& totalMinutes)
{
    if (strlen(text) < 16)
        return false;
    auto number = [] (const char* p, int digits) -> int
    {
        int value = 0;
        for (int i = 0; i < digits; i++)
        {
            if (p[i] < '0' || p[i] > '9')
                return -1;
            value = value * 10 + (p[i] - '0');
        }
        return value;
    };
    int year = number(text, 4);
    int month = number(text + 5, 2);
    int day = number(text + 8, 2);
    int hour = number(text + 11, 2);
    int minute = number(text + 14, 2);
    if (year < 2000 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59)
    {
        return false;
    }
    year -= month <= 2;
    int era = year / 400;
    u32 yearOfEra = year - era * 400;
    u32 dayOfYear = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    u32 dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    s64 days = (s64)era * 146097 + dayOfEra;
    totalMinutes = days * 1440 + hour * 60 + minute;
    return true;
}

// homebrew headers can hold garbage where retail games keep their code; only
// printable codes are usable as identity (and safe inside the json file)
static bool isUsableGameCode(const char* gameCode)
{
    if (!gameCode || gameCode[0] == 0)
        return false;
    for (const char* c = gameCode; *c != 0; c++)
    {
        if (*c < 0x20 || *c >= 0x7F)
            return false;
    }
    return true;
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

GameDataEntry* JsonGameDataService::FindByCode(const char* gameCode)
{
    for (u32 i = 0; i < _entryCount; i++)
    {
        if (_entries[i].gameCode.GetString()[0] != 0 &&
            !strcasecmp(_entries[i].gameCode.GetString(), gameCode))
        {
            return &_entries[i];
        }
    }
    return nullptr;
}

const GameDataEntry* JsonGameDataService::GetEntry(const char* fileName, const char* gameCode) const
{
    auto* self = const_cast<JsonGameDataService*>(this);
    if (isUsableGameCode(gameCode))
    {
        if (auto* byCode = self->FindByCode(gameCode))
            return byCode;
    }
    return self->Find(fileName);
}

GameDataEntry& JsonGameDataService::GetOrCreateEntry(const char* fileName, const char* gameCode)
{
    bool hasCode = isUsableGameCode(gameCode);
    if (hasCode)
    {
        if (auto* byCode = FindByCode(gameCode))
        {
            // self-heal: the file may have been renamed since the entry was
            // written; the code is its stable identity
            byCode->fileName = fileName;
            return *byCode;
        }
    }
    if (auto* existing = Find(fileName))
    {
        // upgrade a legacy name-keyed entry as soon as its code is known
        if (hasCode)
            existing->gameCode = gameCode;
        return *existing;
    }

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
    if (hasCode)
        entry.gameCode = gameCode;
    return entry;
}

void JsonGameDataService::ToggleFavorite(const char* fileName, const char* gameCode)
{
    auto& entry = GetOrCreateEntry(fileName, gameCode);
    entry.favorite = !entry.favorite;
    _version++;
}

void JsonGameDataService::ToggleCompleted(const char* fileName, const char* gameCode)
{
    auto& entry = GetOrCreateEntry(fileName, gameCode);
    entry.completed = !entry.completed;
    _version++;
}

void JsonGameDataService::RecordLaunch(const char* fileName, const char* gameCode,
    const char* fullPath, const char* lastPlayedDateTime)
{
    auto& entry = GetOrCreateEntry(fileName, gameCode);
    entry.launchCount++;
    entry.lastPlayed = lastPlayedDateTime;
    entry.path = fullPath;
    // a play session opens now and closes at the next launcher boot
    _sessionGameFileName = fileName;
    _sessionGameCode = isUsableGameCode(gameCode) ? gameCode : "";
    _sessionStart = lastPlayedDateTime;
    _version++;
}

bool JsonGameDataService::CloseOpenSession(const char* nowDateTime)
{
    if (_sessionStart.GetString()[0] == 0)
        return false;

    s64 startMinutes = 0;
    s64 nowMinutes = 0;
    if (parseDateTime(_sessionStart.GetString(), startMinutes) &&
        parseDateTime(nowDateTime, nowMinutes))
    {
        s64 minutes = nowMinutes - startMinutes;
        // longer than the cap means the console was off, not playing
        if (minutes >= 1 && minutes <= SESSION_MAX_MINUTES)
        {
            auto& entry = GetOrCreateEntry(_sessionGameFileName.GetString(),
                _sessionGameCode.GetString());
            entry.playMinutes += (u32)minutes;
        }
    }
    _sessionStart = "";
    _sessionGameFileName = "";
    _sessionGameCode = "";
    _version++;
    // the cleared session (and any credited time) must be persisted
    return true;
}

void JsonGameDataService::RemoveEntry(const char* fileName, const char* gameCode)
{
    GameDataEntry* entry = isUsableGameCode(gameCode) ? FindByCode(gameCode) : nullptr;
    if (!entry)
        entry = Find(fileName);
    if (!entry)
        return;
    _entryCount--;
    *entry = _entries[_entryCount];
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
        if (!entry.favorite && !entry.completed && entry.launchCount == 0 && entry.playMinutes == 0)
            continue;
        auto game = games[entry.fileName.GetString()].to<JsonObject>();
        if (entry.gameCode.GetString()[0] != 0)
            game[KEY_GAME_CODE] = entry.gameCode.GetString();
        if (entry.favorite)
            game[KEY_FAVORITE] = true;
        if (entry.completed)
            game[KEY_COMPLETED] = true;
        if (entry.launchCount > 0)
            game[KEY_LAUNCH_COUNT] = entry.launchCount;
        if (entry.playMinutes > 0)
            game[KEY_PLAY_MINUTES] = entry.playMinutes;
        if (entry.lastPlayed.GetString()[0] != 0)
            game[KEY_LAST_PLAYED] = entry.lastPlayed.GetString();
        if (entry.path.GetString()[0] != 0)
            game[KEY_PATH] = entry.path.GetString();
    }
    if (_sessionStart.GetString()[0] != 0)
    {
        json[KEY_SESSION_GAME] = _sessionGameFileName.GetString();
        if (_sessionGameCode.GetString()[0] != 0)
            json[KEY_SESSION_GAME_CODE] = _sessionGameCode.GetString();
        json[KEY_SESSION_START] = _sessionStart.GetString();
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
        auto& entry = GetOrCreateEntry(item.key().c_str(), item.value()[KEY_GAME_CODE] | "");
        entry.favorite = item.value()[KEY_FAVORITE] | false;
        entry.completed = item.value()[KEY_COMPLETED] | false;
        entry.launchCount = item.value()[KEY_LAUNCH_COUNT] | 0u;
        entry.playMinutes = item.value()[KEY_PLAY_MINUTES] | 0u;
        entry.lastPlayed = item.value()[KEY_LAST_PLAYED] | "";
        entry.path = item.value()[KEY_PATH] | "";
    }
    _sessionGameFileName = json[KEY_SESSION_GAME] | "";
    _sessionGameCode = json[KEY_SESSION_GAME_CODE] | "";
    _sessionStart = json[KEY_SESSION_START] | "";
}
