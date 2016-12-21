#pragma once

#include <deps/cpptoml.h>

#include <polysync/descriptor/catalog.hpp>

namespace polysync { namespace descriptor {

extern std::vector<Type> fromToml( std::shared_ptr<cpptoml::table>,
                                   const std::string& path = std::string() );

}} // namespace polysync::descriptor
