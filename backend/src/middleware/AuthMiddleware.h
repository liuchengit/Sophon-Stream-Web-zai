#pragma once

/**
 * @file AuthMiddleware.h
 * @brief Legacy auth middleware - now delegates to middleware/Auth.h.
 *
 * This file exists for backward compatibility. The actual JWT auth
 * logic is in middleware/Auth.h (httplib-based).
 * The Drogon HttpMiddleware class has been removed.
 */

#include "middleware/Auth.h"

// The AuthMiddleware class from Drogon has been replaced by
// the checkAuth() function in middleware/Auth.h.
// Route handlers should call:
//   auto auth = middleware::checkAuth(req);
//   if (!auth.valid) { ... return 401 ... }

namespace sophon_stream {
namespace web {
namespace middleware {

// Empty - Drogon middleware removed.
// Use checkAuth() from Auth.h instead.

}  // namespace middleware
}  // namespace web
}  // namespace sophon_stream
