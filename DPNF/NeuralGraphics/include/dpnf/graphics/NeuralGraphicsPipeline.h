#pragma once

#include "dpnf/core/Error.h"
#include "dpnf/tensor/Tensor.h"

#include <cstdint>
#include <string>

namespace dpnf::graphics {

// Explicit structure for graphics G-buffer inputs (Section 11)
struct NeuralGraphicsFrame {
    tensor::Tensor color{};          // [C, H, W] linear RGB
    tensor::Tensor albedo{};         // [3, H, W]
    tensor::Tensor normal{};         // [3, H, W] world/view space unit normals
    tensor::Tensor depth{};          // [1, H, W] normalized depth [0, 1]
    tensor::Tensor motion_vectors{}; // [2, H, W] pixel displacement (dx, dy)
    float exposure{1.0f};
    float jitter_x{0.0f};
    float jitter_y{0.0f};
    std::uint64_t frame_index{0};

    [[nodiscard]] core::Error validate() const;
};

struct ReconstructionMetrics {
    double psnr_db{0.0};
    double ssim{0.0};
    double mae{0.0};
};

class NeuralGraphicsPipeline {
public:
    // Image quality metrics (Section 12)
    static ReconstructionMetrics evaluate_quality(
        const tensor::Tensor& reconstructed,
        const tensor::Tensor& reference);

    // 2x Edge-aware Super Resolution reconstruction (Section 12)
    static core::Result<tensor::Tensor> super_resolve_2x(const tensor::Tensor& low_res);

    // Temporal motion-vector warping & disocclusion-aware history fusion (Section 13)
    static core::Result<tensor::Tensor> temporal_reconstruct(
        const NeuralGraphicsFrame& current_frame,
        const tensor::Tensor& history_color,
        const tensor::Tensor& history_depth,
        float blend_alpha = 0.85f);

    // Joint G-buffer (depth + normal + albedo) spatial denoiser (Section 14)
    static core::Result<tensor::Tensor> denoise_gbuffer(const NeuralGraphicsFrame& noisy_frame);

    // .dpnf binary model serialization (Section 33)
    static core::Error save_dpnf_model(
        const std::string& path,
        const std::string& model_name,
        const tensor::Tensor& weights);

    static core::Result<tensor::Tensor> load_dpnf_model(
        const std::string& path,
        std::string& out_model_name);
};

} // namespace dpnf::graphics
