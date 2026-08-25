#pragma once
#include "core/SharedPtr.h"
#include "services/settings/AppSettings.h"

class SdFolder;
class RomBrowserStateMachine;
class RomBrowserViewModel;
class FileInfo;
class TaskQueueBase;
class ICoverRepository;
class IIconRepository;
class IBannerRepository;
class ICheatRepository;
class IGameDataService;
class IBgmService;

class IRomBrowserController
{
public:
    virtual ~IRomBrowserController() = 0;

    virtual void NavigateUp() = 0;
    virtual void NavigateToPath(const TCHAR* name) = 0;
    /// @brief gameCode is the file's internal game code when the caller has
    ///        it loaded, or nullptr; it keys the game's persisted data.
    virtual void LaunchFile(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void LaunchRandomGame() = 0;
    /// @brief The guides button: opens the reader straight on the highlighted
    ///        entry's guide, or the guides list when it has none.
    virtual void ShowGuides() = 0;
    virtual void HideGuides() = 0;
    /// @brief Points the guide path at one file inside the guides folder.
    virtual void SetGuidePath(const char* guideFileName) = 0;
    /// @brief Path of the guide being read, or of the highlighted entry's own
    ///        guide before one is picked; empty when it has none.
    virtual const char* GetGuidePath() const = 0;
    /// @brief Stored reading position for the guide at GetGuidePath, or 0.
    virtual u32 GetGuideReadOffset() const = 0;
    /// @brief Stores the reading position for the guide at GetGuidePath.
    virtual void SetGuideReadOffset(u32 offset) = 0;
    virtual void ToggleFavorite(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void ToggleCompleted(const FileInfo& fileInfo, const char* gameCode = nullptr) = 0;
    virtual void ToggleFavoritesFilter() = 0;
    virtual bool IsFavoritesFilterEnabled() const = 0;
    virtual void ToggleCompletedFilter() = 0;
    virtual bool IsCompletedFilterEnabled() const = 0;
    virtual void ShowGameInfo(const FileInfo& fileInfo) = 0;
    virtual void HideGameInfo() = 0;
    virtual void ShowDisplaySettings() = 0;
    virtual void HideDisplaySettings() = 0;
    virtual void ShowRecents() = 0;
    virtual void HideRecents() = 0;
    virtual void ShowFavorites() = 0;
    virtual void HideFavorites() = 0;
    virtual void ShowStatistics() = 0;
    virtual void HideStatistics() = 0;
    /// @brief Whether the highlighted entry can be deleted at all. Folders and
    ///        support files cannot, so the app bar dims its delete button rather
    ///        than offering one that does nothing.
    virtual bool CanDeleteSelected() const = 0;
    virtual void RequestDeleteSelected() = 0;
    /// @brief Whether the highlighted entry can be hidden: folders only. A rom
    ///        already has delete, and hiding loose support files would only
    ///        make the card harder to reason about from a computer.
    virtual bool CanHideSelected() const = 0;
    /// @brief Opens the same confirmation sheet as delete, in hide mode.
    virtual void RequestHideSelected() = 0;
    /// @brief Whether the open confirmation is hiding rather than deleting.
    virtual bool IsHideConfirm() const = 0;
    /// @brief Clears the hidden attribute on every entry of the current folder
    ///        the user could have hidden. The only way back: a hidden folder is
    ///        not in the list to press Y on again.
    virtual void UnhideAll() = 0;
    virtual void CancelDelete() = 0;
    virtual void ConfirmDelete() = 0;
    /// @brief File that will be deleted. Deliberately a dedicated buffer, NOT
    ///        _triggerFileInfo: the launch flows overwrite that one, and a
    ///        delete must never act on anything but the file the user saw.
    virtual const char* GetDeleteRomFileName() const = 0;
    /// @brief Save file belonging to the game being deleted, empty when there
    ///        is none. Shown so the user knows it is KEPT - saves are never
    ///        deleted. Valid while the delete confirmation is shown.
    virtual const char* GetDeleteSaveFileName() const = 0;
    virtual void GotoSettingsScreen() = 0;

    virtual void Update() = 0;

    /// @brief Records that the last list move was a big jump by L/R, so the
    ///        letter chip shows for those and not for a d-pad step.
    virtual void NotifyBigStepJump() = 0;
    /// @brief Returns whether a big jump has happened since the last call, and
    ///        clears the flag.
    virtual bool ConsumeBigStepJump() = 0;

    virtual const SdFolder& GetSdFolder() const = 0;

    virtual const RomBrowserStateMachine& GetStateMachine() const = 0;

    virtual const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() = 0;

    virtual TaskQueueBase* GetIoTaskQueue() const = 0;
    virtual TaskQueueBase* GetBgTaskQueue() const = 0;
    virtual const ICoverRepository& GetCoverRepository() const = 0;
    virtual const IIconRepository& GetIconRepository() const = 0;
    virtual const IBannerRepository& GetBannerRepository() const = 0;
    virtual const ICheatRepository& GetCheatRepository() const = 0;
    virtual IGameDataService* GetGameDataService() = 0;

    virtual const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const = 0;

    virtual void SetRomBrowserDisplaySettings(
        const RomBrowserDisplaySettings& romBrowserDisplaySettings) = 0;

    /// @brief DS Lite backlight level (0..3), or -1 when the user never
    ///        picked one (the firmware's level is left untouched then).
    virtual int GetBacklightLevel() const = 0;
    virtual void SetBacklightLevel(int level) = 0;

    virtual const FileInfo& GetTriggerFileInfo() const = 0;
};

inline IRomBrowserController::~IRomBrowserController() { }
