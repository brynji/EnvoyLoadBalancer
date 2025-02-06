#include <string>

#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"

#include "cache_filter.h"
#include "cache_filter_config.pb.h"

namespace Envoy {
namespace Server {
namespace Configuration {

class CacheFilterConfigFactory : public NamedHttpFilterConfigFactory {
public:
  absl::StatusOr<Http::FilterFactoryCb> createFilterFactoryFromProto(const Protobuf::Message& proto_config,
                                                     const std::string&,
                                                     FactoryContext& context) override {                                              
    return createFilter(dynamic_cast<const go::control::plane::CacheConfig&>(proto_config), context);
  }

  ProtobufTypes::MessagePtr createEmptyConfigProto() override {
    return ProtobufTypes::MessagePtr{new go::control::plane::CacheConfig()};
  }

  std::string name() const override { return "custom.cache"; }

private:
  Http::FilterFactoryCb createFilter(const go::control::plane::CacheConfig& proto_config, FactoryContext& context) {
    Http::CacheFilterConfigSharedPtr config = std::make_shared<Http::CacheFilterConfig>(Http::CacheFilterConfig(context.serverFactoryContext().threadLocal(),proto_config));
    
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
