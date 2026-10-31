#pragma once

#include "dpnf/core/Error.h"
#include "dpnf/memory/MemoryPool.h"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace dpnf::tensor {

enum class DataType {
    Float32,
    Float16,
    BFloat16,
    Int32,
    UInt8
};

enum class DeviceType {
    CPU,
    Vulkan
};

class Tensor {
public:
    Tensor() = default;
    explicit Tensor(std::vector<std::size_t> shape, float fill_value = 0.0f);
    Tensor(std::vector<std::size_t> shape, const std::vector<float>& values);

    static Tensor zeros(std::vector<std::size_t> shape);
    static Tensor ones(std::vector<std::size_t> shape);
    static Tensor arange(std::size_t count, float start = 0.0f, float step = 1.0f);

    [[nodiscard]] const std::vector<std::size_t>& shape() const noexcept { return shape_; }
    [[nodiscard]] const std::vector<std::size_t>& strides() const noexcept { return strides_; }
    [[nodiscard]] std::size_t ndim() const noexcept { return shape_.size(); }
    [[nodiscard]] std::size_t numel() const noexcept { return numel_; }
    [[nodiscard]] bool is_contiguous() const noexcept { return contiguous_; }
    [[nodiscard]] DataType dtype() const noexcept { return dtype_; }
    [[nodiscard]] DeviceType device() const noexcept { return device_; }

    [[nodiscard]] float* data() noexcept;
    [[nodiscard]] const float* data() const noexcept;

    [[nodiscard]] float at(const std::vector<std::size_t>& indices) const;
    void set(const std::vector<std::size_t>& indices, float val);

    [[nodiscard]] float item(std::size_t flat_index) const;
    void set_item(std::size_t flat_index, float val);

    [[nodiscard]] Tensor clone() const;
    [[nodiscard]] core::Result<Tensor> reshape(std::vector<std::size_t> new_shape) const;
    [[nodiscard]] core::Result<Tensor> transpose_2d() const;
    [[nodiscard]] core::Result<Tensor> slice_dim0(std::size_t start, std::size_t end) const;

    // Elementwise arithmetic
    [[nodiscard]] core::Result<Tensor> add(const Tensor& other) const;
    [[nodiscard]] core::Result<Tensor> sub(const Tensor& other) const;
    [[nodiscard]] core::Result<Tensor> mul(const Tensor& other) const;
    [[nodiscard]] core::Result<Tensor> div(const Tensor& other) const;
    [[nodiscard]] Tensor scale(float scalar) const;
    [[nodiscard]] Tensor clamp(float min_val, float max_val) const;

    // Matrix operations
    [[nodiscard]] core::Result<Tensor> matmul(const Tensor& other) const;

    // Reductions
    [[nodiscard]] float sum() const;
    [[nodiscard]] float mean() const;
    [[nodiscard]] float max_val() const;
    [[nodiscard]] float min_val() const;
    [[nodiscard]] float l2_norm() const;

    [[nodiscard]] std::string shape_string() const;

private:
    static std::vector<std::size_t> compute_contiguous_strides(const std::vector<std::size_t>& shape);

    std::vector<std::size_t> shape_{};
    std::vector<std::size_t> strides_{};
    std::size_t offset_{0};
    std::size_t numel_{0};
    bool contiguous_{true};
    DataType dtype_{DataType::Float32};
    DeviceType device_{DeviceType::CPU};
    std::shared_ptr<memory::PooledBuffer> storage_{};
};

} // namespace dpnf::tensor
