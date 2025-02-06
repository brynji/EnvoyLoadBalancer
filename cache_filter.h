#pragma once

#include "envoy/http/filter.h"
#include "envoy/thread_local/thread_local.h"
#include "cache.h"
#include "request_coalescer.h"
#include "cache_filter_config.pb.h"

namespace Envoy{
namespace Http{

    //Structs used for cache and request coalescer
struct CacheKey{
    std::string host;
    std::string url;

    CacheKey() = default;
    CacheKey(const HeaderString& h, const HeaderString& u): host(h.getStringView()), url(u.getStringView()){}
    bool operator==(const CacheKey& other)const{ return host==other.host && url==other.url; }
};

struct KeyHash{
    std::size_t operator()(const Envoy::Http::CacheKey& key) const noexcept {
        std::size_t h1 = std::hash<std::string>{}(key.host);
        std::size_t h2 = std::hash<std::string>{}(key.url);
        return h1 ^ (h2 << 1);
    }
};

struct HeaderCollection {
    std::vector<std::pair<std::string, std::string>> headers;
};

struct TrailerCollection {
    std::vector<std::pair<std::string, std::string>> trailers;
};

struct CacheData{
    HeaderCollection headers;
    std::string data;
    TrailerCollection trailers;
};


class CacheFilterConfig {
public:
    /**
     * @param tls slot for cache for each worker thread
     */
    CacheFilterConfig(ThreadLocal::SlotAllocator& tls, const go::control::plane::CacheConfig& proto_config);

    Cache<CacheKey,CacheData,KeyHash>& cache() const;

    RequestCoalescer<CacheKey,CacheData,KeyHash>& coalescer() const;

private:
    const std::string key_;
    const std::string val_;

    std::unique_ptr<Envoy::ThreadLocal::TypedSlot<Cache<CacheKey,CacheData,KeyHash>>> slot;
    std::unique_ptr<RequestCoalescer<CacheKey,CacheData,KeyHash>> request_coalescer;
};

using CacheFilterConfigSharedPtr = std::shared_ptr<CacheFilterConfig>;

class CacheFilter : public StreamFilter, public std::enable_shared_from_this<CacheFilter>{
public:
    CacheFilter(CacheFilterConfigSharedPtr config);

    ~CacheFilter() override = default;

    void onDestroy() override;

    FilterHeadersStatus decodeHeaders(RequestHeaderMap&, bool) override;
    FilterDataStatus decodeData(Buffer::Instance&, bool) override;
    FilterTrailersStatus decodeTrailers(RequestTrailerMap&) override;

    FilterHeadersStatus encodeHeaders(ResponseHeaderMap&, bool) override;
    FilterDataStatus encodeData(Buffer::Instance&, bool) override;
    FilterTrailersStatus encodeTrailers(ResponseTrailerMap&) override;

    Filter1xxHeadersStatus encode1xxHeaders(ResponseHeaderMap&) override;
    FilterMetadataStatus encodeMetadata(MetadataMap&) override;
    void setEncoderFilterCallbacks(StreamEncoderFilterCallbacks&) override;
    void setDecoderFilterCallbacks(StreamDecoderFilterCallbacks&) override;
private:
    void continueDecoding();

    /**
     * Function to process data from request coalescer
     * @param data response to be sent
     */
    void processRequest(std::shared_ptr<CacheData> data)const;

    /**
     * Function to sent back response read from cache
     * @param dataFromCache data read from cache
     */
    void serveFromCache(CacheData& dataFromCache)const;

    /**
     * Check is request is cachable, currently empty
     * @return true if request with given headers is cachable
     */
    bool isCachable(RequestHeaderMap&) const;

    /**
     * Used to save data to cache
     * @param keyToSave key to save
     * @param dataToSave data to save
     */
    void saveToCache(const CacheKey& keyToSave, const CacheData& dataToSave) const;

    StreamDecoderFilterCallbacks* decoder_callbacks_;
    StreamEncoderFilterCallbacks* encoder_callbacks_;

    CacheFilterConfigSharedPtr config;
    bool shouldSaveToCache = false; //should response be saved to cahce
    bool isMainRequest = false; //main request means that it is first request in request coalescing and others may be waiting for response
    bool finishedWork = false; //marks if filter did everything needed before destroying, used to mark if there are no other request waiting for this response
    CacheKey key;
    CacheData cacheData;
};


} // namespace Http
} // namespace Envoy
