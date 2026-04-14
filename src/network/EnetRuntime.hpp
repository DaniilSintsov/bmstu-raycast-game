#pragma once

#include <mutex>

#include <enet/enet.h>

namespace doomlike::network {

class EnetRuntime {
public:
    static bool acquire()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (refCount_ == 0 && enet_initialize() != 0) {
            return false;
        }
        ++refCount_;
        return true;
    }

    static void release()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (refCount_ == 0) {
            return;
        }

        --refCount_;
        if (refCount_ == 0) {
            enet_deinitialize();
        }
    }

private:
    static inline std::mutex mutex_ {};
    static inline int refCount_ {0};
};

}  // namespace doomlike::network

