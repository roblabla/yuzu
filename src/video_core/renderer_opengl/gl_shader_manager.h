// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <glad/glad.h>

#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/gl_state.h"
#include "video_core/renderer_opengl/maxwell_to_gl.h"

namespace OpenGL::GLShader {

using Tegra::Engines::Maxwell3D;

/// Uniform structure for the Uniform Buffer Object, all vectors must be 16-byte aligned
// NOTE: Always keep a vec4 at the end. The GL spec is not clear whether the alignment at
//       the end of a uniform block is included in UNIFORM_BLOCK_DATA_SIZE or not.
//       Not following that rule will cause problems on some AMD drivers.
struct MaxwellUniformData {
    void SetFromRegs(const Maxwell3D::State::ShaderStageInfo& shader_stage);
    alignas(16) GLvec4 viewport_flip;
    struct alignas(16) {
        GLuint instance_id;
        GLuint flip_stage;
        GLfloat y_direction;
    };
    struct alignas(16) {
        GLuint enabled;
        GLuint func;
        GLfloat ref;
        GLuint padding;
    } alpha_test;
};
static_assert(sizeof(MaxwellUniformData) == 48, "MaxwellUniformData structure size is incorrect");
static_assert(sizeof(MaxwellUniformData) < 16384,
              "MaxwellUniformData structure must be less than 16kb as per the OpenGL spec");

class ProgramManager {
public:
    ProgramManager() {
        pipeline.Create();
    }

    void UseProgrammableVertexShader(GLuint program) {
        vs = program;
    }

    void UseProgrammableGeometryShader(GLuint program) {
        gs = program;
    }

    void UseProgrammableFragmentShader(GLuint program) {
        fs = program;
    }

    void UseTrivialGeometryShader() {
        gs = 0;
    }

    void ApplyTo(OpenGLState& state) {
        UpdatePipeline();
        state.draw.shader_program = 0;
        state.draw.program_pipeline = pipeline.handle;
        state.geometry_shaders.enabled = (gs != 0);
    }

private:
    void UpdatePipeline() {
        // Avoid updating the pipeline when values have no changed
        if (old_vs == vs && old_fs == fs && old_gs == gs)
            return;
        // Workaround for AMD bug
        glUseProgramStages(pipeline.handle,
                           GL_VERTEX_SHADER_BIT | GL_GEOMETRY_SHADER_BIT | GL_FRAGMENT_SHADER_BIT,
                           0);

        glUseProgramStages(pipeline.handle, GL_VERTEX_SHADER_BIT, vs);
        glUseProgramStages(pipeline.handle, GL_GEOMETRY_SHADER_BIT, gs);
        glUseProgramStages(pipeline.handle, GL_FRAGMENT_SHADER_BIT, fs);

        // Update the old values
        old_vs = vs;
        old_fs = fs;
        old_gs = gs;
    }

    OGLPipeline pipeline;
    GLuint vs{}, fs{}, gs{};
    GLuint old_vs{}, old_fs{}, old_gs{};
};

} // namespace OpenGL::GLShader
