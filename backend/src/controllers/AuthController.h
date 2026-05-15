#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace sophon_stream {
namespace web {
namespace controllers {

/**
 * Authentication controller (httplib version).
 * Routes are registered in Router.cc; this header provides the interface.
 */
class AuthController {
public:
    AuthController() = default;
};

}  // namespace controllers
}  // namespace web
}  // namespace sophon_stream
