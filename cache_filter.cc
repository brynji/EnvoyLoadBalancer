#include "cache_filter.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/http/header_map_impl.h"

namespace Envoy{
namespace Http{

CacheFilterConfig::CacheFilterConfig(ThreadLocal::SlotAllocator& tls):
    slot(ThreadLocal::TypedSlot<Cache<CacheKey,CacheData,KeyHash>>::makeUnique(tls)){
    slot->set([](Event::Dispatcher&){
        return std::make_shared<Envoy::Http::Cache<CacheKey,CacheData,KeyHash>>(3); //TODO Connect this site to config
    });
    request_coalescer = std::make_unique<RequestCoalescer<CacheKey,CacheData,KeyHash>>();
}

Cache<CacheKey,CacheData,KeyHash>& CacheFilterConfig::cache() const { return slot->get().ref(); }

RequestCoalescer<CacheKey,CacheData,KeyHash>& CacheFilterConfig::coalescer() const { return *request_coalescer; }

CacheFilter::CacheFilter(CacheFilterConfigSharedPtr config): config(config) {}

void CacheFilter::onDestroy(){
    if(finishedWork)
        return;

    if (isMainRequest)
        config->coalescer().promoteNextRequest(key);
}

FilterHeadersStatus CacheFilter::decodeHeaders(RequestHeaderMap& headers, bool){
    //Check if cache is possible
    if (!isCachable(headers)) {
        return FilterHeadersStatus::Continue;
    }

    //Check cache
    key = CacheKey(headers.Host()->value(),headers.Path()->value());
    auto fromCache = config->cache().lookup(key);

    //Serving from cache
    if (fromCache != nullptr) {
        serveFromCache(*fromCache);
        return FilterHeadersStatus::StopIteration;
    }

    //Not in cache, but cachable
    shouldSaveToCache = true;

    //true when this is main request that should continue
    isMainRequest = config->coalescer().coalesceRequest(key, weak_from_this(), [this,callbacks = decoder_callbacks_](std::shared_ptr<CacheData> data) {
        if (data == nullptr) //promotion to main request
            callbacks->dispatcher().post([this]{continueDecoding();});
        else
            callbacks->dispatcher().post([this,data]{processRequest(data);});
    });

    if (isMainRequest){
        return FilterHeadersStatus::Continue;
    }

    return FilterHeadersStatus::StopIteration;
}

FilterDataStatus CacheFilter::decodeData(Buffer::Instance&, bool){
    return FilterDataStatus::Continue;
}

FilterTrailersStatus CacheFilter::decodeTrailers(RequestTrailerMap&){
    return FilterTrailersStatus::Continue;
}

FilterHeadersStatus CacheFilter::encodeHeaders(ResponseHeaderMap& headers, bool endStream){
    if (!shouldSaveToCache)
        return FilterHeadersStatus::Continue;

    //Prepare headers for cache
    HeaderCollection headersCollection;

    auto callback = [&headersCollection](const HeaderEntry& header) -> HeaderMap::Iterate {
        headersCollection.headers.emplace_back(std::string(header.key().getStringView()), std::string(header.value().getStringView()));
        return Envoy::Http::HeaderMap::Iterate::Continue;
    };

    headers.iterate(callback);
    cacheData.headers = headersCollection;

    if (endStream) {
        saveToCache(key,cacheData);
        if (isMainRequest)
            config->coalescer().callCallbacks(key,std::make_shared<CacheData>(cacheData));
        finishedWork = true;
    }

    return FilterHeadersStatus::Continue;
}

FilterDataStatus CacheFilter::encodeData(Buffer::Instance& data, bool endStream){
    if (!shouldSaveToCache)
        return FilterDataStatus::Continue;

    cacheData.data.append(data.toString());

    if (endStream) {
        saveToCache(key,cacheData);
        if (isMainRequest)
            config->coalescer().callCallbacks(key,std::make_shared<CacheData>(cacheData));
        finishedWork = true;
    }

    return FilterDataStatus::Continue;
}

FilterTrailersStatus CacheFilter::encodeTrailers(ResponseTrailerMap& trailers){
    if (!shouldSaveToCache)
        return FilterTrailersStatus::Continue;

    TrailerCollection trailerCollection;

    auto callback = [&trailerCollection](const HeaderEntry& trailer) -> HeaderMap::Iterate {
        trailerCollection.trailers.emplace_back(std::string(trailer.key().getStringView()), std::string(trailer.value().getStringView()));
        return Envoy::Http::HeaderMap::Iterate::Continue;
    };

    trailers.iterate(callback);
    cacheData.trailers = trailerCollection;

    saveToCache(key,cacheData);
    if (isMainRequest)
        config->coalescer().callCallbacks(key,std::make_shared<CacheData>(cacheData));
    finishedWork = true;

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

void CacheFilter::continueDecoding(){
    isMainRequest = true;
    decoder_callbacks_->continueDecoding();
}

void CacheFilter::processRequest(std::shared_ptr<CacheData> data)const {
        saveToCache(key,*data);
        serveFromCache(*data);
}

void CacheFilter::serveFromCache(CacheData& dataFromCache)const{
    ResponseHeaderMapPtr responseHeaders = ResponseHeaderMapImpl::create();
    for (auto & h : dataFromCache.headers.headers){
        responseHeaders->addCopy(LowerCaseString(h.first), h.second);
    }

    if (dataFromCache.data.empty() && dataFromCache.trailers.trailers.empty()) {
        decoder_callbacks_->encodeHeaders(std::move(responseHeaders),true,"serving_from_cache");
        return;
    }
    decoder_callbacks_->encodeHeaders(std::move(responseHeaders),false,"serving_from_cache");

    //Data
    std::unique_ptr<Buffer::Instance> buffer = std::make_unique<Buffer::OwnedImpl>();
    buffer->add(dataFromCache.data);

    if (dataFromCache.trailers.trailers.empty()) {
        decoder_callbacks_->encodeData(*buffer,true);
    } else {
        decoder_callbacks_->encodeData(*buffer,false);
        //Trailers
        ResponseTrailerMapPtr responseTrailers = ResponseTrailerMapImpl::create();
        for (auto & h : dataFromCache.trailers.trailers) responseTrailers->addCopy(LowerCaseString(h.first), h.second);
        decoder_callbacks_->encodeTrailers(std::move(responseTrailers));
    }
}

bool CacheFilter::isCachable(RequestHeaderMap&)const{
    return true;
}

void CacheFilter::saveToCache(const CacheKey& keyToSave,const CacheData& dataToSave) const {
    config->cache().save(keyToSave,dataToSave);
}

} // namespace Http
} // namespace Envoy
