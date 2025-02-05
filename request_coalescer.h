#pragma once

#include <condition_variable>
#include <unordered_map>

#include "envoy/common/key_value_store.h"

template<typename Key, typename Value, typename KeyHash>
class RequestCoalescer {
public:
    std::shared_ptr<Value> coalesceRequest(Key& key) {
        std::unique_lock lock(mutex);

        auto it = pendingRequests.find(key);
        if (it == pendingRequests.end()) {
            pendingRequests[key];
            return nullptr;
        }

        it->second.numberOfWaiters +=1;

        if (!it->second.isReady)
            it->second.cond.wait(lock,[it]{return it->second.isReady;});

        //This means this is the last waiting thread and request can be deleted
        if (it->second.numberOfWaiters <= 1) {
            auto ret = it->second.value;
            pendingRequests.erase(key);
            return ret;
        }

        it->second.numberOfWaiters -= 1;
        return it->second.value;
    }

    void addDataToRequest(Key& key, Value& value) {
        std::lock_guard lock(mutex);

        auto it = pendingRequests.find(key);
        if (it->second.numberOfWaiters==0) {
            pendingRequests.erase(key);
            return;
        }

        it->second.value = std::make_shared<Value>(value);
        it->second.isReady = true;
        it->second.cond.notify_all();
    }
private:
    struct Entry {
        std::shared_ptr<Value> value;
        bool isReady = false;
        int numberOfWaiters = 0;
        std::condition_variable cond;
    };
    std::unordered_map<Key,Entry,KeyHash> pendingRequests;
    std::mutex mutex;
};
