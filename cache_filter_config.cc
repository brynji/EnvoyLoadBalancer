#include <string>

#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"

#include "cache_filter.h"

namespace Envoy {
namespace Server {
namespace Configuration {

class CacheFilterConfigFactory : public NamedHttpFilterConfigFactory {
public:
  absl::StatusOr<Http::FilterFactoryCb> createFilterFactoryFromProto(const Protobuf::Message&,
                                                     const std::string&,
                                                     FactoryContext& context) override {                                              
    return createFilter(context);
  }

  ProtobufTypes::MessagePtr createEmptyConfigProto() override {
    return ProtobufTypes::MessagePtr{new Envoy::ProtobufWkt::Struct()};
  }

  std::string name() const override { return "custom.cache"; }

private:
  Http::FilterFactoryCb createFilter(FactoryContext& context) {
    Http::CacheFilterConfigSharedPtr config = std::make_shared<Http::CacheFilterConfig>(Http::CacheFilterConfig(context.serverFactoryContext().threadLocal()));
    
    return [config](Http::FilterChainFactoryCallbacks& callbacks) -> void {
      auto filter = new Http::CacheFilter(config);
      callbacks.addStreamFilter(Http::StreamFilterSharedPtr{filter});
    };
  }
};

/**
 * Static registration for this sample filter. @see RegisterFactory.
 */
static Registry::RegisterFactory<CacheFilterConfigFactory, NamedHttpFilterConfigFactory> register_;

} // namespace Configuration
} // namespace Server
} // namespace Envoy