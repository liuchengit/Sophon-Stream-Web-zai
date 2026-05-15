#pragma once

#include "httplib/httplib.h"

namespace sophon_stream {
namespace web {

/**
 * Register all API routes with the httplib server.
 * Includes CORS handling, JWT auth middleware, and all endpoint handlers.
 */
void registerRoutes(httplib::Server& server);

}  // namespace web
}  // namespace sophon_stream
