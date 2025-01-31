#include "cache_filter.h"

namespace Envoy{
namespace Http{
    
CacheFilter::CacheFilter(){

}

void CacheFilter::onDestroy(){
}

FilterHeadersStatus CacheFilter::decodeHeaders(RequestHeaderMap&, bool){
    return FilterHeadersStatus::Continue;
}

FilterDataStatus CacheFilter::decodeData(Buffer::Instance&, bool){
    return FilterDataStatus::Continue;
}

FilterTrailersStatus CacheFilter::decodeTrailers(RequestTrailerMap&){
    return FilterTrailersStatus::Continue;
}

FilterHeadersStatus CacheFilter::encodeHeaders(ResponseHeaderMap&, bool){
    return FilterHeadersStatus::Continue;
}

FilterDataStatus CacheFilter::encodeData(Buffer::Instance&, bool){
    return FilterDataStatus::Continue;
}

FilterTrailersStatus CacheFilter::encodeTrailers(ResponseTrailerMap&){
    return FilterTrailersStatus::Continue;
}

Filter1xxHeadersStatus CacheFilter::encode1xxHeaders(ResponseHeaderMap&){
    return Filter1xxHeadersStatus::Continue;
}

FilterMetadataStatus CacheFilter::encodeMetadata(MetadataMap&){
    return FilterMetadataStatus::Continue;
}

void CacheFilter::setEncoderFilterCallbacks(StreamEncoderFilterCallbacks&){
}

void CacheFilter::setDecoderFilterCallbacks(StreamDecoderFilterCallbacks&){
}

} // namespace Http
} // namespace Envoy
