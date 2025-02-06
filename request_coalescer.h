#pragma once

#include <unordered_map>
#include "source/common/common/thread.h"

#include <iostream>

template<typename Key, typename Value, typename KeyHash>
class RequestCoalescer {
public:
    /**
     * Checks if the is any request with the same key sent to the server
     * @param key Key to match same requests together
     * @param weakObj used for checks if object is still valid before calling callbacks
     * @param callback function to call after the data are available. If passed data are nullptr, it means something
     * went wrong with the original request, and you have to send request to the server yourself.
     * @return True if this is the first request of this type and request should be sent to the server.
     * After this, callCallbacks should always be called. If return is False, this request should wait as another
     * request with same was already sent to the server.
     */
    bool coalesceRequest(Key& key, std::weak_ptr<void> weakObj, std::function<void(std::shared_ptr<Value>)> callback) {
        mutex.lock();

        auto it = pendingRequests.find(key);
        if (it == pendingRequests.end()) {
            pendingRequests[key];
            mutex.unlock();
            return true;
        } else {
            it->second.emplace_back(weakObj,callback);
        }

        mutex.unlock();
        return false;
    }

    /**
     * Passes response data to all requests that are waiting.
     * @param key Key to match same requests together
     * @param value Response that it to be shared with other requests of the same key.
     */
    void callCallbacks(Key& key, std::shared_ptr<Value> value) {
        mutex.lock();

        auto it = pendingRequests.find(key);
        for (auto & entry : it->second) {
            if (!entry.weakObj.expired())
                entry.callback(value);
        }
        pendingRequests.erase(key);

        mutex.unlock();
    }

    /**
     * Should be called when coalesceRequests returned true, but for some reason you are not able to call callCallbacks with response from the server.
     * This is used to stop requests with same key from waiting forever.
     * @param key Key to match same requests together
     */
    void promoteNextRequest(Key& key) {
        mutex.lock();

        auto it = pendingRequests.find(key);
        while (!it->second.empty()) {
            if (!it->second.back().weakObj.expired()) {
                it->second.back().callback(nullptr);
                it->second.pop_back();
                mutex.unlock();
                return;
            }
            it->second.pop_back();
        }
        pendingRequests.erase(key);
        mutex.unlock();
    }
private:
    struct Entry {
        std::weak_ptr<void> weakObj;
        std::function<void(std::shared_ptr<Value>)> callback;
    };
    std::unordered_map<Key,std::vector<Entry>,KeyHash> pendingRequests;
    Envoy::Thread::MutexBasicLockable mutex;
};
