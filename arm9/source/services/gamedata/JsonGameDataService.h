#pragma once
#include <memory>
#include "IGameDataService.h"

class JsonGameDataService : public IGameDataService
{
public:
    JsonGameDataService();

    const GameDataEntry* GetEntry(const char* fileName) const override;
    void ToggleFavorite(const char* fileName) override;
    void RecordLaunch(const char* fileName, const char* fullPath, const char* lastPlayedDateTime) override;
    u32 GetEntryCount() const override { return _entryCount; }
    const GameDataEntry& GetEntryByIndex(u32 index) const override { return _entries[index]; }
    u32 GetVersion() const override { return _version; }
    void SaveAsync(TaskQueueBase* ioTaskQueue) override;

private:
    std::unique_ptr<GameDataEntry[]> _entries;
    u32 _entryCount = 0;
    u32 _entryCapacity = 0;
    u32 _version = 0;

    GameDataEntry* Find(const char* fileName);
    GameDataEntry& GetOrCreateEntry(const char* fileName);
    void Load();
};
