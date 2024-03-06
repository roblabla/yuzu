// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <utility>
#include <glad/glad.h>
#include "common/common_types.h"
#include "video_core/renderer_opengl/gl_shader_util.h"

namespace OpenGL {

class OGLTexture : private NonCopyable {
public:
    OGLTexture() = default;

    OGLTexture(OGLTexture&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLTexture() {
        Release();
    }

    OGLTexture& operator=(OGLTexture&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLSampler : private NonCopyable {
public:
    OGLSampler() = default;

    OGLSampler(OGLSampler&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLSampler() {
        Release();
    }

    OGLSampler& operator=(OGLSampler&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLShader : private NonCopyable {
public:
    OGLShader() = default;

    OGLShader(OGLShader&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLShader() {
        Release();
    }

    OGLShader& operator=(OGLShader&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    void Create(const char* source, GLenum type);

    void Release();

    GLuint handle = 0;
};

class OGLProgram : private NonCopyable {
public:
    OGLProgram() = default;

    OGLProgram(OGLProgram&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLProgram() {
        Release();
    }

    OGLProgram& operator=(OGLProgram&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    template <typename... T>
    void Create(bool separable_program, T... shaders) {
        if (handle != 0)
            return;
        handle = GLShader::LoadProgram(separable_program, shaders...);
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void CreateFromSource(const char* vert_shader, const char* geo_shader, const char* frag_shader,
                          bool separable_program = false);

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLPipeline : private NonCopyable {
public:
    OGLPipeline() = default;
    OGLPipeline(OGLPipeline&& o) noexcept : handle{std::exchange<GLuint>(o.handle, 0)} {}

    ~OGLPipeline() {
        Release();
    }
    OGLPipeline& operator=(OGLPipeline&& o) noexcept {
        handle = std::exchange<GLuint>(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLBuffer : private NonCopyable {
public:
    OGLBuffer() = default;

    OGLBuffer(OGLBuffer&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLBuffer() {
        Release();
    }

    OGLBuffer& operator=(OGLBuffer&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLSync : private NonCopyable {
public:
    OGLSync() = default;

    OGLSync(OGLSync&& o) noexcept : handle(std::exchange(o.handle, nullptr)) {}

    ~OGLSync() {
        Release();
    }
    OGLSync& operator=(OGLSync&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, nullptr);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLsync handle = 0;
};

class OGLVertexArray : private NonCopyable {
public:
    OGLVertexArray() = default;

    OGLVertexArray(OGLVertexArray&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLVertexArray() {
        Release();
    }

    OGLVertexArray& operator=(OGLVertexArray&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

class OGLFramebuffer : private NonCopyable {
public:
    OGLFramebuffer() = default;

    OGLFramebuffer(OGLFramebuffer&& o) noexcept : handle(std::exchange(o.handle, 0)) {}

    ~OGLFramebuffer() {
        Release();
    }

    OGLFramebuffer& operator=(OGLFramebuffer&& o) noexcept {
        Release();
        handle = std::exchange(o.handle, 0);
        return *this;
    }

    /// Creates a new internal OpenGL resource and stores the handle
    void Create();

    /// Deletes the internal OpenGL resource
    void Release();

    GLuint handle = 0;
};

} // namespace OpenGL
