# ============================================================================
# Sophon-Stream Web Management System - Multi-stage Docker Build
# Stage 1: Frontend build (Node.js 18)
# Stage 2: Backend build (Ubuntu 22.04)
# Stage 3: Runtime (minimal)
# ============================================================================

# ── Stage 1: Frontend Build ────────────────────────────────────────────────
FROM node:18-alpine AS frontend-builder

WORKDIR /build/frontend

# Install dependencies first (better layer caching)
COPY frontend/package.json frontend/package-lock.json* ./
RUN npm ci --prefer-offline 2>/dev/null || npm install

# Copy frontend source and build
COPY frontend/ ./
RUN npm run build

# ── Stage 2: Backend Build ─────────────────────────────────────────────────
FROM ubuntu:22.04 AS backend-builder

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# Install build dependencies
RUN apt-get update -qq && apt-get install -y -qq \
    build-essential \
    cmake \
    gcc \
    g++ \
    pkg-config \
    git \
    libsqlite3-dev \
    libssl-dev \
    zlib1g-dev \
    libboost-all-dev \
    uuid-dev \
    curl \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Build drogon from source
RUN git clone --depth 1 https://github.com/an-tao/trantor.git /tmp/trantor && \
    mkdir -p /tmp/trantor/build && \
    cmake -S /tmp/trantor -B /tmp/trantor/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build /tmp/trantor/build -j"$(nproc)" && \
    cmake --install /tmp/trantor/build && \
    rm -rf /tmp/trantor

RUN git clone --depth 1 https://github.com/drogonframework/drogon.git /tmp/drogon && \
    mkdir -p /tmp/drogon/build && \
    cmake -S /tmp/drogon -B /tmp/drogon/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DBUILD_TESTING=OFF \
        -DBUILD_EXAMPLES=OFF && \
    cmake --build /tmp/drogon/build -j"$(nproc)" && \
    cmake --install /tmp/drogon/build && \
    rm -rf /tmp/drogon

# Build jwt-cpp
RUN git clone --depth 1 https://github.com/Thalhammer/jwt-cpp.git /tmp/jwt-cpp && \
    mkdir -p /tmp/jwt-cpp/build && \
    cmake -S /tmp/jwt-cpp -B /tmp/jwt-cpp/build \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DJWT_CMAKE_FILES_INSTALL_DIR=/usr/lib/cmake/jwt-cpp && \
    cmake --build /tmp/jwt-cpp/build && \
    cmake --install /tmp/jwt-cpp/build && \
    rm -rf /tmp/jwt-cpp

# Build nlohmann_json
RUN git clone --depth 1 https://github.com/nlohmann/json.git /tmp/json && \
    mkdir -p /tmp/json/build && \
    cmake -S /tmp/json -B /tmp/json/build \
        -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build /tmp/json/build && \
    cmake --install /tmp/json/build && \
    rm -rf /tmp/json

# Build spdlog
RUN git clone --depth 1 https://github.com/gabime/spdlog.git /tmp/spdlog && \
    mkdir -p /tmp/spdlog/build && \
    cmake -S /tmp/spdlog -B /tmp/spdlog/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build /tmp/spdlog/build -j"$(nproc)" && \
    cmake --install /tmp/spdlog/build && \
    rm -rf /tmp/spdlog

# Build backend
WORKDIR /build/backend
COPY backend/ ./

# Create sophon-stream SDK stub directories (real SDK would be mounted or installed)
RUN mkdir -p /opt/sophon-stream/include /opt/sophon-stream/lib

RUN mkdir -p build && \
    cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DSOPHON_STREAM_INCLUDE_DIR=/opt/sophon-stream/include \
        -DSOPHON_STREAM_LIB_DIR=/opt/sophon-stream/lib && \
    cmake --build build -j"$(nproc)"

# ── Stage 3: Runtime ──────────────────────────────────────────────────────
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# Install runtime dependencies only
RUN apt-get update -qq && apt-get install -y -qq \
    libsqlite3-0 \
    libssl3 \
    zlib1g \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN groupadd -r sophon && \
    useradd -r -g sophon -d /app -s /sbin/nologin sophon

# Copy backend binary and shared libraries
COPY --from=backend-builder /usr/local/lib/libdrogon*.so* /usr/local/lib/
COPY --from=backend-builder /usr/local/lib/libtrantor*.so* /usr/local/lib/
COPY --from=backend-builder /build/backend/build/sophon-stream-web /app/bin/
COPY --from=backend-builder /build/backend/config/config.json /app/config/

# Copy frontend assets
COPY --from=frontend-builder /build/frontend/dist /app/frontend/

# Update frontend path in config
RUN sed -i 's|"./frontend/dist"|"./frontend"|g' /app/config/config.json

# Update shared library cache
RUN ldconfig

# Create data, logs, plugins directories
RUN mkdir -p /app/data /app/logs /app/plugins /app/migrations && \
    chown -R sophon:sophon /app

WORKDIR /app

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=5s --start-period=15s --retries=3 \
    CMD curl -f http://localhost:8080/api/v1/health || exit 1

# Run as non-root user
USER sophon

# Start the application
CMD ["/app/bin/sophon-stream-web", "--config=/app/config/config.json"]
