#pragma once

#include "envoy/http/filter.h"
#include "envoy/thread_local/thread_local.h"
#include "cache.h"

namespace Envoy{
namespace Http{

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
                return h1 ^ (h2 << 1); // Combine hashes
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
  CacheFilterConfig(ThreadLocal::SlotAllocator& tls):
    slot(ThreadLocal::TypedSlot<Cache<CacheKey,CacheData,KeyHash>>::makeUnique(tls)){
      slot->set([](Event::Dispatcher&){
          return std::make_shared<Envoy::Http::Cache<CacheKey,CacheData,KeyHash>>(3); //TODO Connect this site to config
      });
    }

  const std::string& key() const { return key_; }
  const std::string& val() const { return val_; }
  Cache<CacheKey,CacheData,KeyHash>& cache() const { return slot->get().ref(); }

private:
  const std::string key_;
  const std::string val_;

  std::unique_ptr<Envoy::ThreadLocal::TypedSlot<Cache<CacheKey,CacheData,KeyHash>>> slot;
};

using CacheFilterConfigSharedPtr = std::shared_ptr<CacheFilterConfig>;

class CacheFilter : public StreamFilter{
public:
    CacheFilter(CacheFilterConfigSharedPtr config): config(config) {}
    ~CacheFilter() override = default;
    // Http::StreamFilterBase
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
    bool isCachable(RequestHeaderMap&) const;
    void save() const;

    StreamDecoderFilterCallbacks* decoder_callbacks_;
    StreamEncoderFilterCallbacks* encoder_callbacks_;

    CacheFilterConfigSharedPtr config;
    bool saveToCache = false;
    CacheKey key;
    CacheData cacheData;
};


} // namespace Http
} // namespace Envoy