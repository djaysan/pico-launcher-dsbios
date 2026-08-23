#pragma once
#include "core/task/TaskQueue.h"

class IRomBrowserItemViewModel
{
public:
    virtual ~IRomBrowserItemViewModel() = default;

    virtual void Activate() = 0;
    virtual void ShowGameInfo() = 0;
    virtual void ToggleFavorite() = 0;
    /// @brief Opens the delete confirmation for this item (long press X).
    virtual void RequestDelete() = 0;
    virtual void SetIndex(int index) = 0;
    /// @brief Item index this view model is currently bound to, -1 when
    ///        unbound. Views are pooled: input state armed on one item must
    ///        be revalidated against this after any rebind.
    virtual int GetIndex() const = 0;
    virtual void SetQueueTask(QueueTask<void> queueTask) = 0;
    virtual void CancelQueueTask() = 0;
    virtual void DisposeQueueTaskWhenComplete() = 0;

protected:
    IRomBrowserItemViewModel() = default;
};
