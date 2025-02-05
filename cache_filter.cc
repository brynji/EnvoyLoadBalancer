#include "cache_filter.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/http/header_map_impl.h"

namespace Envoy{
namespace Http{

void CacheFilter::onDestroy(){
}

FilterHeadersStatus CacheFilter::decodeHeaders(RequestHeaderMap& headers, bool){
    //Check if cache is possible
    if (!isCachable(headers)) {

        return FilterHeadersStatus::Continue;
    }
    //Check cache
    key = CacheKey(headers.Host()->value(),headers.Path()->value());
    const auto fromCache = config->cache().lookup(key);
    if (fromCache == nullptr) {
        //Not in cache, but cachable
        saveToCache = true;
        return FilterHeadersStatus::Continue;
    }
    //Serving from cache
    //Headers
    ResponseHeaderMapPtr responseHeaders = ResponseHeaderMapImpl::create();
    for (auto & h : fromCache->headers.headers){
        responseHeaders->addCopy(LowerCaseString(h.first), h.second);
    }
    decoder_callbacks_->encodeHeaders(std::move(responseHeaders),false,"serving_from_cache");

    //Data
    std::unique_ptr<Buffer::Instance> buffer = std::make_unique<Buffer::OwnedImpl>();
    buffer->add(fromCache->data);

    if (fromCache->trailers.trailers.empty()) {
        decoder_callbacks_->encodeData(*buffer,true);
    } else {
        decoder_callbacks_->encodeData(*buffer,false);
        //Trailers
        ResponseTrailerMapPtr responseTrailers = ResponseTrailerMapImpl::create();
        for (auto & h : fromCache->trailers.trailers) responseTrailers->addCopy(LowerCaseString(h.first), h.second);
        decoder_callbacks_->encodeTrailers(std::move(responseTrailers));
    }

    return FilterHeadersStatus::StopIteration;
}

FilterDataStatus CacheFilter::decodeData(Buffer::Instance&, bool){
    return FilterDataStatus::Continue;
}

FilterTrailersStatus CacheFilter::decodeTrailers(RequestTrailerMap&){
    return FilterTrailersStatus::Continue;
}

FilterHeadersStatus CacheFilter::encodeHeaders(ResponseHeaderMap& headers, bool){
    if (!saveToCache)
        return FilterHeadersStatus::Continue;

    //Prepare headers for cache
    HeaderCollection headersCollection;

    auto callback = [&headersCollection](const HeaderEntry& header) -> HeaderMap::Iterate {
        headersCollection.headers.emplace_back(std::string(header.key().getStringView()), std::string(header.value().getStringView()));
        return Envoy::Http::HeaderMap::Iterate::Continue;
    };

    headers.iterate(callback);
    cacheData.headers = headersCollection;

    return FilterHeadersStatus::Continue;
}

FilterDataStatus CacheFilter::encodeData(Buffer::Instance& data, bool endStream){
    if (!saveToCache)
        return FilterDataStatus::Continue;

    cacheData.data.append(data.toString());

    if (endStream)
        save();

    return FilterDataStatus::Continue;
}

FilterTrailersStatus CacheFilter::encodeTrailers(ResponseTrailerMap& trailers){
    if (!saveToCache)
        return FilterTrailersStatus::Continue;

    TrailerCollection trailerCollection;

    auto callback = [&trailerCollection](const HeaderEntry& trailer) -> HeaderMap::Iterate {
        trailerCollection.trailers.emplace_back(std::string(trailer.key().getStringView()), std::string(trailer.value().getStringView()));
        return Envoy::Http::HeaderMap::Iterate::Continue;
    };

    trailers.iterate(callback);
    cacheData.trailers = trailerCollection;

    save();

    return FilterTrailersStatus::Continue;
}

Filter1xxHeadersStatus CacheFilter::encode1xxHeaders(ResponseHeaderMap&){
    return Filter1xxHeadersStatus::Continue;
}

FilterMetadataStatus CacheFilter::encodeMetadata(MetadataMap&){
    return FilterMetadataStatus::Continue;
}

void CacheFilter::setEncoderFilterCallbacks(StreamEncoderFilterCallbacks& encoder_callbacks){
    encoder_callbacks_ = &encoder_callbacks;
}

void CacheFilter::setDecoderFilterCallbacks(StreamDecoderFilterCallbacks& decoder_callbacks){
    decoder_callbacks_ = &decoder_callbacks;
}

bool CacheFilter::isCachable(RequestHeaderMap&)const{
    return true;
}

void CacheFilter::save() const {
    config->cache().save(key,cacheData);
}

} // namespace Http
} // namespace Envoy
