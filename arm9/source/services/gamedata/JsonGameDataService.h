#pragma once
#include <memory>
#include "IGameDataService.h"

class JsonGameDataService : public IGameDataService
{
public:
    JsonGameDataService();

    const GameDataEntry* GetEntry(const char* fileName, const char* gameCode = nullptr) const override;
    void ToggleFavorite(const char* fileName, const char* gameCode = nullptr,
        const char* fullPath = nullptr) override;
    void ToggleCompleted(const char* fileName, const char* gameCode = nullptr,
        const char* fullPath = nullptr) override;
    bool BackfillPath(const char* fileName, const char* gameCode,
        const char* fullPath) override;
    bool HasUnpathedFlaggedEntry() const override;
    void RecordLaunch(const char* fileName, const char* gameCode,
        const char* fullPath, const char* lastPlayedDateTime) override;
    void RemoveEntry(const char* fileName, const char* gameCode = nullptr) override;
    bool CloseOpenSession(const char* nowDateTime) override;
    u32 GetEntryCount() const override { return _entryCount; }
    const GameDataEntry& GetEntryByIndex(u32 index) const override { return _entries[index]; }
    u32 GetVersion() const override { return _version; }
    void SaveAsync(TaskQueueBase* ioTaskQueue) override;

private:
    std::unique_ptr<GameDataEntry[]> _entries;
    u32 _entryCount = 0;
    u32 _entryCapacity = 0;
    u32 _version = 0;
    String<char, 96> _sessionGameFileName;
    String<char, 8> _sessionGameCode;
    String<char, 20> _sessionStart;

    GameDataEntry* Find(const char* fileName);
    GameDataEntry* FindByCode(const char* gameCode);
    GameDataEntry& GetOrCreateEntry(const char* fileName, const char* gameCode);
    void Load();
};
