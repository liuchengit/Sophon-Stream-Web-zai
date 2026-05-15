#pragma once

/**
 * @file CORS.h
 * @brief CORS helper for httplib-based routes.
 *
 * Replaces the old Drogon CORSMiddleware with simple inline helpers
 * that can be called directly from route handlers.
 */

#include "httplib/httplib.h"
#include <string>

namespace sophon_stream {
namespace web {
namespace middleware {

/**
 * Set standard CORS headers on a response.
 * Allows all origins, common methods and headers.
 * @param res The HTTP response to modify.
 */
inline void setCORSHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    res.set_header("Access-Control-Max-Age", "86400");
    res.set_header("Access-Control-Allow-Credentials", "true");
    res.set_header("Access-Control-Expose-Headers", "Content-Length, Content-Type, X-Request-Id");
}

}  // namespace middleware
}  // namespace web
}  // namespace sophon_stream
