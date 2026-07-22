#pragma once
#include "GameDataEntry.h"

class TaskQueueBase;

class IGameDataService
{
public:
    virtual ~IGameDataService() = 0;

    /// @brief Returns the entry for the given game, or nullptr when none
    ///        exists. Lookup is by game code first (when given), then by file
    ///        name. The pointer is only valid until the next mutation
    ///        (compare GetVersion to detect those).
    virtual const GameDataEntry* GetEntry(const char* fileName, const char* gameCode = nullptr) const = 0;

    /// @brief Mutations resolve the entry by code first and self-heal its
    ///        stored file name, so renamed files keep their data. When
    ///        fullPath is given it is stored on the entry, so lists that
    ///        navigate to a game (favorites, recents) work for games that
    ///        were marked but never launched.
    virtual void ToggleFavorite(const char* fileName, const char* gameCode = nullptr,
        const char* fullPath = nullptr) = 0;
    virtual void ToggleCompleted(const char* fileName, const char* gameCode = nullptr,
        const char* fullPath = nullptr) = 0;

    /// @brief Stores fullPath on an EXISTING flagged entry that has none yet,
    ///        so favorites/completed marks made before path recording become
    ///        navigable from the panels. Only touches entries with favorite
    ///        or completed set and an empty path. Returns true when it wrote
    ///        (the caller should then save).
    virtual bool BackfillPath(const char* fileName, const char* gameCode,
        const char* fullPath) = 0;

    /// @brief Whether any favorite/completed entry still lacks a path. Cheap
    ///        gate so the per-folder backfill scan is skipped once every mark
    ///        is navigable (the steady state on a large library).
    virtual bool HasUnpathedFlaggedEntry() const = 0;
    virtual void RecordLaunch(const char* fileName, const char* gameCode,
        const char* fullPath, const char* lastPlayedDateTime) = 0;
    virtual void RemoveEntry(const char* fileName, const char* gameCode = nullptr) = 0;

    /// @brief Credits the session opened by the last RecordLaunch with the
    ///        time elapsed until now (the next launcher boot). Sessions over
    ///        6 hours are discarded: that is a power-off, not a play session.
    /// @return True when play time was credited (the caller should save).
    virtual bool CloseOpenSession(const char* nowDateTime) = 0;

    /// @brief Unordered access to all entries, for building derived lists
    ///        (e.g. recents). Indices are only valid until the next mutation.
    virtual u32 GetEntryCount() const = 0;
    virtual const GameDataEntry& GetEntryByIndex(u32 index) const = 0;

    /// @brief Incremented on every mutation; cheap to poll for UI refresh.
    virtual u32 GetVersion() const = 0;

    /// @brief Serializes on the calling thread (so concurrent mutations can't
    ///        invalidate the data being written) and enqueues the SD write on
    ///        the given task queue.
    virtual void SaveAsync(TaskQueueBase* ioTaskQueue) = 0;
};

inline IGameDataService::~IGameDataService() { }
