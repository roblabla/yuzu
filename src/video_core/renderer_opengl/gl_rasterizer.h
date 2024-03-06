// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/icl/interval_map.hpp>
#include <boost/range/iterator_range.hpp>
#include <glad/glad.h>

#include "common/common_types.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_cache.h"
#include "video_core/rasterizer_interface.h"
#include "video_core/renderer_opengl/gl_buffer_cache.h"
#include "video_core/renderer_opengl/gl_global_cache.h"
#include "video_core/renderer_opengl/gl_primitive_assembler.h"
#include "video_core/renderer_opengl/gl_rasterizer_cache.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/renderer_opengl/gl_shader_cache.h"
#include "video_core/renderer_opengl/gl_shader_gen.h"
#include "video_core/renderer_opengl/gl_shader_manager.h"
#include "video_core/renderer_opengl/gl_state.h"
#include "video_core/renderer_opengl/gl_stream_buffer.h"

namespace Core::Frontend {
class EmuWindow;
}

namespace OpenGL {

struct ScreenInfo;
struct DrawParameters;
struct FramebufferCacheKey;

class RasterizerOpenGL : public VideoCore::RasterizerInterface {
public:
    explicit RasterizerOpenGL(Core::Frontend::EmuWindow& renderer, ScreenInfo& info);
    ~RasterizerOpenGL() override;

    void DrawArrays() override;
    void Clear() override;
    void FlushAll() override;
    void FlushRegion(VAddr addr, u64 size) override;
    void InvalidateRegion(VAddr addr, u64 size) override;
    void FlushAndInvalidateRegion(VAddr addr, u64 size) override;
    bool AccelerateSurfaceCopy(const Tegra::Engines::Fermi2D::Regs::Surface& src,
                               const Tegra::Engines::Fermi2D::Regs::Surface& dst) override;
    bool AccelerateDisplay(const Tegra::FramebufferConfig& config, VAddr framebuffer_addr,
                           u32 pixel_stride) override;
    bool AccelerateDrawBatch(bool is_indexed) override;
    void UpdatePagesCachedCount(Tegra::GPUVAddr addr, u64 size, int delta) override;

    /// Maximum supported size that a constbuffer can have in bytes.
    static constexpr std::size_t MaxConstbufferSize = 0x10000;
    static_assert(MaxConstbufferSize % sizeof(GLvec4) == 0,
                  "The maximum size of a constbuffer must be a multiple of the size of GLvec4");

    static constexpr std::size_t MaxGlobalMemorySize = 0x10000;
    static_assert(MaxGlobalMemorySize % sizeof(float) == 0,
                  "The maximum size of a global memory must be a multiple of the size of float");

private:
    class SamplerInfo {
    public:
        OGLSampler sampler;

        /// Creates the sampler object, initializing its state so that it's in sync with the
        /// SamplerInfo struct.
        void Create();
        /// Syncs the sampler object with the config, updating any necessary state.
        void SyncWithConfig(const Tegra::Texture::TSCEntry& info);

    private:
        Tegra::Texture::TextureFilter mag_filter = Tegra::Texture::TextureFilter::Nearest;
        Tegra::Texture::TextureFilter min_filter = Tegra::Texture::TextureFilter::Nearest;
        Tegra::Texture::TextureMipmapFilter mip_filter = Tegra::Texture::TextureMipmapFilter::None;
        Tegra::Texture::WrapMode wrap_u = Tegra::Texture::WrapMode::ClampToEdge;
        Tegra::Texture::WrapMode wrap_v = Tegra::Texture::WrapMode::ClampToEdge;
        Tegra::Texture::WrapMode wrap_p = Tegra::Texture::WrapMode::ClampToEdge;
        bool uses_depth_compare = false;
        Tegra::Texture::DepthCompareFunc depth_compare_func =
            Tegra::Texture::DepthCompareFunc::Always;
        GLvec4 border_color = {};
        float min_lod = 0.0f;
        float max_lod = 16.0f;
        float lod_bias = 0.0f;
        float max_anisotropic = 1.0f;
    };

    struct FramebufferConfigState {
        bool using_color_fb{};
        bool using_depth_fb{};
        bool preserve_contents{};
        std::optional<std::size_t> single_color_target;

        bool operator==(const FramebufferConfigState& rhs) const {
            return std::tie(using_color_fb, using_depth_fb, preserve_contents,
                            single_color_target) == std::tie(rhs.using_color_fb, rhs.using_depth_fb,
                                                             rhs.preserve_contents,
                                                             rhs.single_color_target);
        }
        bool operator!=(const FramebufferConfigState& rhs) const {
            return !operator==(rhs);
        }
    };

    /**
     * Configures the color and depth framebuffer states.
     * @param use_color_fb If true, configure color framebuffers.
     * @param using_depth_fb If true, configure the depth/stencil framebuffer.
     * @param preserve_contents If true, tries to preserve data from a previously used framebuffer.
     * @param single_color_target Specifies if a single color buffer target should be used.
     * @returns If depth (first) or stencil (second) are being stored in the bound zeta texture
     * (requires using_depth_fb to be true)
     */
    std::pair<bool, bool> ConfigureFramebuffers(
        OpenGLState& current_state, bool use_color_fb = true, bool using_depth_fb = true,
        bool preserve_contents = true, std::optional<std::size_t> single_color_target = {});

    /// Configures the current constbuffers to use for the draw command.
    void SetupConstBuffers(Tegra::Engines::Maxwell3D::Regs::ShaderStage stage, const Shader& shader,
                           GLuint program_handle, BaseBindings base_bindings);

    /// Configures the current global memory entries to use for the draw command.
    void SetupGlobalRegions(Tegra::Engines::Maxwell3D::Regs::ShaderStage stage,
                            const Shader& shader, GLenum primitive_mode,
                            BaseBindings base_bindings);

    /// Configures the current textures to use for the draw command.
    void SetupTextures(Tegra::Engines::Maxwell3D::Regs::ShaderStage stage, const Shader& shader,
                       GLuint program_handle, BaseBindings base_bindings);

    /// Syncs the viewport and depth range to match the guest state
    void SyncViewport(OpenGLState& current_state);

    /// Syncs the clip enabled status to match the guest state
    void SyncClipEnabled(
        const std::array<bool, Tegra::Engines::Maxwell3D::Regs::NumClipDistances>& clip_mask);

    /// Syncs the clip coefficients to match the guest state
    void SyncClipCoef();

    /// Syncs the cull mode to match the guest state
    void SyncCullMode();

    /// Syncs the primitve restart to match the guest state
    void SyncPrimitiveRestart();

    /// Syncs the depth test state to match the guest state
    void SyncDepthTestState();

    /// Syncs the stencil test state to match the guest state
    void SyncStencilTestState();

    /// Syncs the blend state to match the guest state
    void SyncBlendState();

    /// Syncs the LogicOp state to match the guest state
    void SyncLogicOpState();

    /// Syncs the the color clamp state
    void SyncFragmentColorClampState();

    /// Syncs the alpha coverage and alpha to one
    void SyncMultiSampleState();

    /// Syncs the scissor test state to match the guest state
    void SyncScissorTest(OpenGLState& current_state);

    /// Syncs the transform feedback state to match the guest state
    void SyncTransformFeedback();

    /// Syncs the point state to match the guest state
    void SyncPointState();

    /// Syncs Color Mask
    void SyncColorMask();

    /// Syncs the polygon offsets
    void SyncPolygonOffset();

    /// Check asserts for alpha testing.
    void CheckAlphaTests();

    /// Check for extension that are not strictly required
    /// but are needed for correct emulation
    void CheckExtensions();

    OpenGLState state;

    RasterizerCacheOpenGL res_cache;
    ShaderCacheOpenGL shader_cache;
    GlobalRegionCacheOpenGL global_cache;

    Core::Frontend::EmuWindow& emu_window;

    ScreenInfo& screen_info;

    std::unique_ptr<GLShader::ProgramManager> shader_program_manager;
    std::map<std::array<Tegra::Engines::Maxwell3D::Regs::VertexAttribute,
                        Tegra::Engines::Maxwell3D::Regs::NumVertexAttributes>,
             OGLVertexArray>
        vertex_array_cache;

    std::map<FramebufferCacheKey, OGLFramebuffer> framebuffer_cache;
    FramebufferConfigState current_framebuffer_config_state;
    std::pair<bool, bool> current_depth_stencil_usage{};

    std::array<SamplerInfo, Tegra::Engines::Maxwell3D::Regs::NumTextureSamplers> texture_samplers;

    static constexpr std::size_t STREAM_BUFFER_SIZE = 128 * 1024 * 1024;
    OGLBufferCache buffer_cache;
    PrimitiveAssembler primitive_assembler{buffer_cache};
    GLint uniform_buffer_alignment;

    std::size_t CalculateVertexArraysSize() const;

    std::size_t CalculateIndexBufferSize() const;

    /// Updates and returns a vertex array object representing current vertex format
    GLuint SetupVertexFormat();

    void SetupVertexBuffer(GLuint vao);

    DrawParameters SetupDraw();

    void SetupShaders(GLenum primitive_mode);

    void SetupCachedFramebuffer(const FramebufferCacheKey& fbkey, OpenGLState& current_state);

    enum class AccelDraw { Disabled, Arrays, Indexed };
    AccelDraw accelerate_draw = AccelDraw::Disabled;

    using CachedPageMap = boost::icl::interval_map<u64, int>;
    CachedPageMap cached_pages;
};

} // namespace OpenGL
