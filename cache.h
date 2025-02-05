#pragma once

#include "envoy/thread_local/thread_local_object.h"

#include <unordered_map>
#include <vector>

namespace Envoy{
namespace Http{

template <typename Key, typename Value, typename Hash>
class Cache : public Envoy::ThreadLocal::ThreadLocalObject{
    public:
    Cache(uint capacity): capacity(capacity){
        data.reserve(capacity);
        indexMap.reserve(capacity);
    }

    Value* lookup(const Key& key){
        auto it = indexMap.find(key);
        if (it != indexMap.end()) {
            return &(data[it->second].value);
        }
        return nullptr;
    }

    void save(const Key& key, const Value& value){
        //Save data
        if (indexMap.size() < capacity) {
            const uint index = indexMap.size();
            indexMap[key] = index;
            data.emplace_back(key, value);
        } else {
            //Remove oldest
            indexMap.erase(data[oldestEntry].key);
            //Add new
            data[oldestEntry] = {key, value};
            indexMap[key] = oldestEntry;
            oldestEntry = (oldestEntry + 1) % capacity;
        }
    }

    private:
    struct Entry {
        Key key;
        Value value;
    };

    std::unordered_map<Key, uint, Hash> indexMap;
    std::vector<Entry> data;
    uint capacity;
    uint oldestEntry = 0;
};

} // namespace Http
} // namespace Envoy