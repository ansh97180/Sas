#include "dpnf/graphics/NeuralGraphicsPipeline.h"

#include <algorithm>
#include <cmath>
#include <fstream>

namespace dpnf::graphics {

core::Error NeuralGraphicsFrame::validate() const {
    if (color.ndim() != 3 || color.shape()[0] != 3) {
        return DPNF_MAKE_ERROR(
            core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
            "NeuralGraphicsFrame::color must have shape [3, H, W], got " + color.shape_string(),
            "NeuralGraphics");
    }
    const std::size_t H = color.shape()[1];
    const std::size_t W = color.shape()[2];

    if (depth.numel() > 0) {
        if (depth.ndim() != 3 || depth.shape()[0] != 1 || depth.shape()[1] != H || depth.shape()[2] != W) {
            return DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "NeuralGraphicsFrame::depth resolution mismatch against color buffer.",
                "NeuralGraphics");
        }
    }

    if (motion_vectors.numel() > 0) {
        if (motion_vectors.ndim() != 3 || motion_vectors.shape()[0] != 2 ||
            motion_vectors.shape()[1] != H || motion_vectors.shape()[2] != W) {
            return DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "NeuralGraphicsFrame::motion_vectors must have shape [2, H, W].",
                "NeuralGraphics");
        }
    }

    if (exposure <= 0.0f) {
        return DPNF_MAKE_ERROR(
            core::ErrorCode::DPNF_ERROR_INVALID_ARGUMENT,
            "NeuralGraphicsFrame::exposure must be strictly positive.",
            "NeuralGraphics");
    }

    return core::Error::ok();
}

ReconstructionMetrics NeuralGraphicsPipeline::evaluate_quality(
    const tensor::Tensor& reconstructed,
    const tensor::Tensor& reference) {
    ReconstructionMetrics m{};
    if (reconstructed.numel() == 0 || reconstructed.numel() != reference.numel()) {
        return m;
    }

    const std::size_t N = reconstructed.numel();
    const float* p = reconstructed.data();
    const float* r = reference.data();

    double mse = 0.0;
    double mae = 0.0;
    double mean_p = 0.0;
    double mean_r = 0.0;

    for (std::size_t i = 0; i < N; ++i) {
        const double diff = static_cast<double>(p[i] - r[i]);
        mse += diff * diff;
        mae += std::fabs(diff);
        mean_p += static_cast<double>(p[i]);
        mean_r += static_cast<double>(r[i]);
    }
    mse /= static_cast<double>(N);
    mae /= static_cast<double>(N);
    mean_p /= static_cast<double>(N);
    mean_r /= static_cast<double>(N);

    double var_p = 0.0;
    double var_r = 0.0;
    double cov = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        const double dp = static_cast<double>(p[i]) - mean_p;
        const double dr = static_cast<double>(r[i]) - mean_r;
        var_p += dp * dp;
        var_r += dr * dr;
        cov += dp * dr;
    }
    var_p /= static_cast<double>(N);
    var_r /= static_cast<double>(N);
    cov /= static_cast<double>(N);

    m.mae = mae;
    m.psnr_db = mse <= 1e-12 ? 99.0 : 10.0 * std::log10(1.0 / mse);

    const double c1 = 0.01 * 0.01;
    const double c2 = 0.03 * 0.03;
    m.ssim = ((2.0 * mean_p * mean_r + c1) * (2.0 * cov + c2)) /
             ((mean_p * mean_p + mean_r * mean_r + c1) * (var_p + var_r + c2));
    return m;
}

core::Result<tensor::Tensor> NeuralGraphicsPipeline::super_resolve_2x(const tensor::Tensor& low_res) {
    if (low_res.ndim() != 3) {
        return core::Result<tensor::Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "super_resolve_2x expects [C, H, W] input tensor.",
                "SuperResolution"));
    }

    const std::size_t C = low_res.shape()[0];
    const std::size_t H = low_res.shape()[1];
    const std::size_t W = low_res.shape()[2];
    const std::size_t out_H = H * 2;
    const std::size_t out_W = W * 2;

    tensor::Tensor hr({C, out_H, out_W}, 0.0f);
    for (std::size_t c = 0; c < C; ++c) {
        for (std::size_t y = 0; y < out_H; ++y) {
            const float src_y = (static_cast<float>(y) + 0.5f) * 0.5f - 0.5f;
            const std::size_t y0 = static_cast<std::size_t>(std::clamp(std::floor(src_y), 0.0f, static_cast<float>(H - 1)));
            const std::size_t y1 = std::min(y0 + 1, H - 1);
            const float wy = std::clamp(src_y - static_cast<float>(y0), 0.0f, 1.0f);

            for (std::size_t x = 0; x < out_W; ++x) {
                const float src_x = (static_cast<float>(x) + 0.5f) * 0.5f - 0.5f;
                const std::size_t x0 = static_cast<std::size_t>(std::clamp(std::floor(src_x), 0.0f, static_cast<float>(W - 1)));
                const std::size_t x1 = std::min(x0 + 1, W - 1);
                const float wx = std::clamp(src_x - static_cast<float>(x0), 0.0f, 1.0f);

                const float v00 = low_res.at({c, y0, x0});
                const float v01 = low_res.at({c, y0, x1});
                const float v10 = low_res.at({c, y1, x0});
                const float v11 = low_res.at({c, y1, x1});

                const float bilinear =
                    (1.0f - wy) * ((1.0f - wx) * v00 + wx * v01) +
                    wy * ((1.0f - wx) * v10 + wx * v11);

                // Edge-preserving detail enhancement
                const float laplacian = bilinear - 0.25f * (v00 + v01 + v10 + v11);
                hr.set({c, y, x}, std::clamp(bilinear + 0.15f * laplacian, 0.0f, 1.0f));
            }
        }
    }

    return core::Result<tensor::Tensor>::success(std::move(hr));
}

core::Result<tensor::Tensor> NeuralGraphicsPipeline::temporal_reconstruct(
    const NeuralGraphicsFrame& current_frame,
    const tensor::Tensor& history_color,
    const tensor::Tensor& history_depth,
    float blend_alpha) {
    const auto err = current_frame.validate();
    if (!err.is_ok()) {
        return core::Result<tensor::Tensor>::failure(err);
    }

    const std::size_t C = current_frame.color.shape()[0];
    const std::size_t H = current_frame.color.shape()[1];
    const std::size_t W = current_frame.color.shape()[2];

    tensor::Tensor output({C, H, W}, 0.0f);
    for (std::size_t y = 0; y < H; ++y) {
        for (std::size_t x = 0; x < W; ++x) {
            const float dx = current_frame.motion_vectors.numel() > 0
                                 ? current_frame.motion_vectors.at({0, y, x})
                                 : 0.0f;
            const float dy = current_frame.motion_vectors.numel() > 0
                                 ? current_frame.motion_vectors.at({1, y, x})
                                 : 0.0f;

            const int prev_x = std::clamp(static_cast<int>(std::round(static_cast<float>(x) - dx)), 0, static_cast<int>(W - 1));
            const int prev_y = std::clamp(static_cast<int>(static_cast<float>(y) - dy), 0, static_cast<int>(H - 1));

            bool disoccluded = false;
            if (current_frame.depth.numel() > 0 && history_depth.numel() == current_frame.depth.numel()) {
                const float curr_z = current_frame.depth.at({0, y, x});
                const float prev_z = history_depth.at({0, static_cast<std::size_t>(prev_y), static_cast<std::size_t>(prev_x)});
                if (std::fabs(curr_z - prev_z) > 0.1f) {
                    disoccluded = true;
                }
            }

            const float effective_alpha = disoccluded ? 0.0f : blend_alpha;
            for (std::size_t c = 0; c < C; ++c) {
                const float curr_c = current_frame.color.at({c, y, x});
                const float hist_c = history_color.at({c, static_cast<std::size_t>(prev_y), static_cast<std::size_t>(prev_x)});
                output.set({c, y, x}, (1.0f - effective_alpha) * curr_c + effective_alpha * hist_c);
            }
        }
    }

    return core::Result<tensor::Tensor>::success(std::move(output));
}

core::Result<tensor::Tensor> NeuralGraphicsPipeline::denoise_gbuffer(const NeuralGraphicsFrame& noisy_frame) {
    const auto err = noisy_frame.validate();
    if (!err.is_ok()) {
        return core::Result<tensor::Tensor>::failure(err);
    }

    const std::size_t C = noisy_frame.color.shape()[0];
    const std::size_t H = noisy_frame.color.shape()[1];
    const std::size_t W = noisy_frame.color.shape()[2];

    tensor::Tensor denoised({C, H, W}, 0.0f);
    for (std::size_t y = 0; y < H; ++y) {
        for (std::size_t x = 0; x < W; ++x) {
            const float center_z = noisy_frame.depth.numel() > 0 ? noisy_frame.depth.at({0, y, x}) : 0.5f;

            for (std::size_t c = 0; c < C; ++c) {
                float weighted_sum = 0.0f;
                float weight_total = 0.0f;

                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        const std::size_t ny = static_cast<std::size_t>(
                            std::clamp(static_cast<int>(y) + ky, 0, static_cast<int>(H - 1)));
                        const std::size_t nx = static_cast<std::size_t>(
                            std::clamp(static_cast<int>(x) + kx, 0, static_cast<int>(W - 1)));

                        const float neigh_z = noisy_frame.depth.numel() > 0
                                                  ? noisy_frame.depth.at({0, ny, nx})
                                                  : center_z;
                        const float dz = center_z - neigh_z;
                        const float w_depth = std::exp(-(dz * dz) / 0.02f);
                        const float w_spatial = (kx == 0 && ky == 0) ? 1.0f : 0.5f;
                        const float w = w_depth * w_spatial;

                        weighted_sum += w * noisy_frame.color.at({c, ny, nx});
                        weight_total += w;
                    }
                }
                denoised.set({c, y, x}, weighted_sum / std::max(weight_total, 1e-6f));
            }
        }
    }

    return core::Result<tensor::Tensor>::success(std::move(denoised));
}

core::Error NeuralGraphicsPipeline::save_dpnf_model(
    const std::string& path,
    const std::string& model_name,
    const tensor::Tensor& weights) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return DPNF_MAKE_ERROR(
            core::ErrorCode::DPNF_ERROR_MODEL_SAVE_FAILED,
            "Failed to open .dpnf output file '" + path + "' for writing.",
            "Serialization");
    }

    const char magic[8] = {'D', 'P', 'N', 'F', 'v', '1', '.', '0'};
    out.write(magic, 8);

    const std::uint32_t name_len = static_cast<std::uint32_t>(model_name.size());
    out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
    out.write(model_name.data(), static_cast<std::streamsize>(name_len));

    const std::uint32_t ndim = static_cast<std::uint32_t>(weights.ndim());
    out.write(reinterpret_cast<const char*>(&ndim), sizeof(ndim));
    for (std::size_t d : weights.shape()) {
        const std::uint64_t dim64 = static_cast<std::uint64_t>(d);
        out.write(reinterpret_cast<const char*>(&dim64), sizeof(dim64));
    }

    const std::uint64_t numel64 = static_cast<std::uint64_t>(weights.numel());
    out.write(reinterpret_cast<const char*>(&numel64), sizeof(numel64));
    out.write(reinterpret_cast<const char*>(weights.data()), static_cast<std::streamsize>(weights.numel() * sizeof(float)));

    return core::Error::ok();
}

core::Result<tensor::Tensor> NeuralGraphicsPipeline::load_dpnf_model(
    const std::string& path,
    std::string& out_model_name) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return core::Result<tensor::Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_MODEL_LOAD_FAILED,
                "Failed to open .dpnf model file '" + path + "'.",
                "Serialization"));
    }

    char magic[8]{};
    in.read(magic, 8);
    if (std::string(magic, 8) != "DPNFv1.0") {
        return core::Result<tensor::Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_MODEL_LOAD_FAILED,
                "Invalid .dpnf header magic in '" + path + "'.",
                "Serialization"));
    }

    std::uint32_t name_len = 0;
    in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
    out_model_name.resize(name_len);
    in.read(out_model_name.data(), static_cast<std::streamsize>(name_len));

    std::uint32_t ndim = 0;
    in.read(reinterpret_cast<char*>(&ndim), sizeof(ndim));
    std::vector<std::size_t> shape(ndim);
    for (std::uint32_t i = 0; i < ndim; ++i) {
        std::uint64_t d = 0;
        in.read(reinterpret_cast<char*>(&d), sizeof(d));
        shape[i] = static_cast<std::size_t>(d);
    }

    std::uint64_t numel64 = 0;
    in.read(reinterpret_cast<char*>(&numel64), sizeof(numel64));

    tensor::Tensor loaded(std::move(shape), 0.0f);
    in.read(reinterpret_cast<char*>(loaded.data()), static_cast<std::streamsize>(numel64 * sizeof(float)));

    return core::Result<tensor::Tensor>::success(std::move(loaded));
}

} // namespace dpnf::graphics
