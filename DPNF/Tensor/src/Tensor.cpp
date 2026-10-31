#include "dpnf/tensor/Tensor.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

namespace dpnf::tensor {

std::vector<std::size_t> Tensor::compute_contiguous_strides(const std::vector<std::size_t>& shape) {
    if (shape.empty()) {
        return {};
    }
    std::vector<std::size_t> strides(shape.size(), 1);
    for (std::size_t i = shape.size() - 1; i > 0; --i) {
        strides[i - 1] = strides[i] * shape[i];
    }
    return strides;
}

Tensor::Tensor(std::vector<std::size_t> shape, float fill_value)
    : shape_(std::move(shape)),
      strides_(compute_contiguous_strides(shape_)),
      offset_(0),
      contiguous_(true) {
    if (shape_.empty()) {
        numel_ = 0;
        return;
    }
    numel_ = 1;
    for (std::size_t d : shape_) {
        numel_ *= d;
    }
    storage_ = std::make_shared<memory::PooledBuffer>(numel_ * sizeof(float), "tensor_f32");
    float* ptr = data();
    if (ptr != nullptr) {
        std::fill(ptr, ptr + numel_, fill_value);
    }
}

Tensor::Tensor(std::vector<std::size_t> shape, const std::vector<float>& values)
    : Tensor(std::move(shape), 0.0f) {
    const std::size_t copy_count = std::min(numel_, values.size());
    float* ptr = data();
    if (ptr != nullptr && copy_count > 0) {
        std::copy(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(copy_count), ptr);
    }
}

Tensor Tensor::zeros(std::vector<std::size_t> shape) {
    return Tensor(std::move(shape), 0.0f);
}

Tensor Tensor::ones(std::vector<std::size_t> shape) {
    return Tensor(std::move(shape), 1.0f);
}

Tensor Tensor::arange(std::size_t count, float start, float step) {
    Tensor t({count}, 0.0f);
    float* ptr = t.data();
    for (std::size_t i = 0; i < count; ++i) {
        ptr[i] = start + static_cast<float>(i) * step;
    }
    return t;
}

float* Tensor::data() noexcept {
    if (!storage_ || storage_->data() == nullptr) {
        return nullptr;
    }
    return static_cast<float*>(storage_->data()) + offset_;
}

const float* Tensor::data() const noexcept {
    if (!storage_ || storage_->data() == nullptr) {
        return nullptr;
    }
    return static_cast<const float*>(storage_->data()) + offset_;
}

float Tensor::at(const std::vector<std::size_t>& indices) const {
    std::size_t idx = 0;
    for (std::size_t i = 0; i < indices.size() && i < strides_.size(); ++i) {
        idx += indices[i] * strides_[i];
    }
    return data()[idx];
}

void Tensor::set(const std::vector<std::size_t>& indices, float val) {
    std::size_t idx = 0;
    for (std::size_t i = 0; i < indices.size() && i < strides_.size(); ++i) {
        idx += indices[i] * strides_[i];
    }
    data()[idx] = val;
}

float Tensor::item(std::size_t flat_index) const {
    return data()[flat_index];
}

void Tensor::set_item(std::size_t flat_index, float val) {
    data()[flat_index] = val;
}

Tensor Tensor::clone() const {
    Tensor out(shape_, 0.0f);
    if (contiguous_) {
        std::copy(data(), data() + numel_, out.data());
    } else {
        for (std::size_t i = 0; i < numel_; ++i) {
            out.data()[i] = data()[i];
        }
    }
    return out;
}

core::Result<Tensor> Tensor::reshape(std::vector<std::size_t> new_shape) const {
    std::size_t new_numel = new_shape.empty() ? 0 : 1;
    for (std::size_t d : new_shape) {
        new_numel *= d;
    }
    if (new_numel != numel_) {
        return core::Result<Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "Cannot reshape tensor of numel=" + std::to_string(numel_) +
                    " into shape with numel=" + std::to_string(new_numel),
                "Tensor"));
    }
    Tensor view = *this;
    view.shape_ = std::move(new_shape);
    view.strides_ = compute_contiguous_strides(view.shape_);
    return core::Result<Tensor>::success(std::move(view));
}

core::Result<Tensor> Tensor::transpose_2d() const {
    if (shape_.size() != 2) {
        return core::Result<Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "transpose_2d requires a 2D tensor, got ndim=" + std::to_string(shape_.size()),
                "Tensor"));
    }
    const std::size_t rows = shape_[0];
    const std::size_t cols = shape_[1];
    Tensor out({cols, rows}, 0.0f);
    for (std::size_t r = 0; r < rows; ++r) {
        for (std::size_t c = 0; c < cols; ++c) {
            out.set({c, r}, at({r, c}));
        }
    }
    return core::Result<Tensor>::success(std::move(out));
}

core::Result<Tensor> Tensor::slice_dim0(std::size_t start, std::size_t end) const {
    if (shape_.empty() || start >= end || end > shape_[0]) {
        return core::Result<Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "Invalid slice bounds [" + std::to_string(start) + ", " + std::to_string(end) + "]",
                "Tensor"));
    }
    std::vector<std::size_t> new_shape = shape_;
    new_shape[0] = end - start;
    Tensor view = *this;
    view.shape_ = std::move(new_shape);
    view.offset_ = offset_ + start * strides_[0];
    view.numel_ = (end - start) * strides_[0];
    return core::Result<Tensor>::success(std::move(view));
}

core::Result<Tensor> Tensor::add(const Tensor& other) const {
    if (numel_ != other.numel_) {
        return core::Result<Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "Tensor::add shape mismatch: " + shape_string() + " vs " + other.shape_string(),
                "Tensor"));
    }
    Tensor out(shape_, 0.0f);
    const float* a = data();
    const float* b = other.data();
    float* c = out.data();
    for (std::size_t i = 0; i < numel_; ++i) {
        c[i] = a[i] + b[i];
    }
    return core::Result<Tensor>::success(std::move(out));
}

core::Result<Tensor> Tensor::sub(const Tensor& other) const {
    if (numel_ != other.numel_) {
        return core::Result<Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "Tensor::sub shape mismatch: " + shape_string() + " vs " + other.shape_string(),
                "Tensor"));
    }
    Tensor out(shape_, 0.0f);
    const float* a = data();
    const float* b = other.data();
    float* c = out.data();
    for (std::size_t i = 0; i < numel_; ++i) {
        c[i] = a[i] - b[i];
    }
    return core::Result<Tensor>::success(std::move(out));
}

core::Result<Tensor> Tensor::mul(const Tensor& other) const {
    if (numel_ != other.numel_) {
        return core::Result<Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "Tensor::mul shape mismatch: " + shape_string() + " vs " + other.shape_string(),
                "Tensor"));
    }
    Tensor out(shape_, 0.0f);
    const float* a = data();
    const float* b = other.data();
    float* c = out.data();
    for (std::size_t i = 0; i < numel_; ++i) {
        c[i] = a[i] * b[i];
    }
    return core::Result<Tensor>::success(std::move(out));
}

core::Result<Tensor> Tensor::div(const Tensor& other) const {
    if (numel_ != other.numel_) {
        return core::Result<Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "Tensor::div shape mismatch: " + shape_string() + " vs " + other.shape_string(),
                "Tensor"));
    }
    Tensor out(shape_, 0.0f);
    const float* a = data();
    const float* b = other.data();
    float* c = out.data();
    for (std::size_t i = 0; i < numel_; ++i) {
        c[i] = a[i] / (std::fabs(b[i]) < 1e-12f ? 1e-12f : b[i]);
    }
    return core::Result<Tensor>::success(std::move(out));
}

Tensor Tensor::scale(float scalar) const {
    Tensor out(shape_, 0.0f);
    const float* a = data();
    float* c = out.data();
    for (std::size_t i = 0; i < numel_; ++i) {
        c[i] = a[i] * scalar;
    }
    return out;
}

Tensor Tensor::clamp(float min_val, float max_val) const {
    Tensor out(shape_, 0.0f);
    const float* a = data();
    float* c = out.data();
    for (std::size_t i = 0; i < numel_; ++i) {
        c[i] = std::clamp(a[i], min_val, max_val);
    }
    return out;
}

core::Result<Tensor> Tensor::matmul(const Tensor& other) const {
    if (shape_.size() != 2 || other.shape_.size() != 2 || shape_[1] != other.shape_[0]) {
        return core::Result<Tensor>::failure(
            DPNF_MAKE_ERROR(
                core::ErrorCode::DPNF_ERROR_INVALID_TENSOR,
                "Incompatible 2D shapes for matmul: " + shape_string() + " and " + other.shape_string(),
                "Tensor"));
    }
    const std::size_t M = shape_[0];
    const std::size_t K = shape_[1];
    const std::size_t N = other.shape_[1];

    Tensor out({M, N}, 0.0f);
    const float* A = data();
    const float* B = other.data();
    float* C = out.data();

    for (std::size_t m = 0; m < M; ++m) {
        for (std::size_t k = 0; k < K; ++k) {
            const float a_mk = A[m * K + k];
            for (std::size_t n = 0; n < N; ++n) {
                C[m * N + n] += a_mk * B[k * N + n];
            }
        }
    }
    return core::Result<Tensor>::success(std::move(out));
}

float Tensor::sum() const {
    if (numel_ == 0) return 0.0f;
    const float* p = data();
    float acc = 0.0f;
    for (std::size_t i = 0; i < numel_; ++i) {
        acc += p[i];
    }
    return acc;
}

float Tensor::mean() const {
    if (numel_ == 0) return 0.0f;
    return sum() / static_cast<float>(numel_);
}

float Tensor::max_val() const {
    if (numel_ == 0) return 0.0f;
    const float* p = data();
    return *std::max_element(p, p + numel_);
}

float Tensor::min_val() const {
    if (numel_ == 0) return 0.0f;
    const float* p = data();
    return *std::min_element(p, p + numel_);
}

float Tensor::l2_norm() const {
    if (numel_ == 0) return 0.0f;
    const float* p = data();
    float sq = 0.0f;
    for (std::size_t i = 0; i < numel_; ++i) {
        sq += p[i] * p[i];
    }
    return std::sqrt(sq);
}

std::string Tensor::shape_string() const {
    std::ostringstream oss;
    oss << "[";
    for (std::size_t i = 0; i < shape_.size(); ++i) {
        oss << shape_[i] << (i + 1 < shape_.size() ? ", " : "");
    }
    oss << "]";
    return oss.str();
}

} // namespace dpnf::tensor
