#pragma once

/**
 * @file CORSMiddleware.h
 * @brief Legacy CORS middleware - now delegates to middleware/CORS.h.
 *
 * This file exists for backward compatibility. The actual CORS
 * logic is in middleware/CORS.h (httplib-based).
 * The Drogon HttpMiddleware class has been removed.
 */

#include "middleware/CORS.h"

// The CORSMiddleware class from Drogon has been replaced by
// the setCORSHeaders() function in middleware/CORS.h.
// Route handlers should call:
//   middleware::setCORSHeaders(res);

namespace sophon_stream {
namespace web {
namespace middleware {

// Empty - Drogon middleware removed.
// Use setCORSHeaders() from CORS.h instead.

}  // namespace middleware
}  // namespace web
}  // namespace sophon_stream
