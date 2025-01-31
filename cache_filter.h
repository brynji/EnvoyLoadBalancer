#pragma once

#include "envoy/http/filter.h"

namespace Envoy{
namespace Http{

class CacheFilterConfig {
public:
  CacheFilterConfig(){}

  const std::string& key() const { return key_; }
  const std::string& val() const { return val_; }

private:
  const std::string key_;
  const std::string val_;
};

using CacheFilterConfigSharedPtr = std::shared_ptr<CacheFilterConfig>;

class CacheFilter : public StreamFilter{
public:
    CacheFilter();
    ~CacheFilter() = default;

    // Http::StreamFilterBase
    void onDestroy() override;

    // Http::StreamDecoderFilter
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
    StreamDecoderFilterCallbacks* decoder_callbacks_;
    StreamEncoderFilterCallbacks* encoder_callbacks_;
};


} // namespace Http
} // namespace Envoy