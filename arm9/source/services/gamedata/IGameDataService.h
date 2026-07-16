#pragma once
#include "GameDataEntry.h"

class TaskQueueBase;

class IGameDataService
{
public:
    virtual ~IGameDataService() = 0;

    /// @brief Returns the entry for the given file name, or nullptr when none
    ///        exists. The pointer is only valid until the next mutation
    ///        (compare GetVersion to detect those).
    virtual const GameDataEntry* GetEntry(const char* fileName) const = 0;

    virtual void ToggleFavorite(const char* fileName) = 0;
    virtual void RecordLaunch(const char* fileName, const char* lastPlayedDateTime) = 0;

    /// @brief Incremented on every mutation; cheap to poll for UI refresh.
    virtual u32 GetVersion() const = 0;

    /// @brief Serializes on the calling thread (so concurrent mutations can't
    ///        invalidate the data being written) and enqueues the SD write on
    ///        the given task queue.
    virtual void SaveAsync(TaskQueueBase* ioTaskQueue) = 0;
};

inline IGameDataService::~IGameDataService() { }
