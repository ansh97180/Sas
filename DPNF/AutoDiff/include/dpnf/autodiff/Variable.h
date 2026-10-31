#pragma once

#include "dpnf/tensor/Tensor.h"

#include <functional>
#include <memory>
#include <vector>

namespace dpnf::autodiff {

struct VariableNode {
    tensor::Tensor data{};
    tensor::Tensor grad{};
    bool requires_grad{false};
    std::vector<std::shared_ptr<VariableNode>> parents{};
    std::function<void()> backward_fn{};
};

class Variable {
public:
    Variable() = default;
    Variable(tensor::Tensor data, bool requires_grad = false);

    [[nodiscard]] const tensor::Tensor& data() const noexcept { return node_->data; }
    [[nodiscard]] tensor::Tensor& data() noexcept { return node_->data; }
    [[nodiscard]] const tensor::Tensor& grad() const noexcept { return node_->grad; }
    [[nodiscard]] tensor::Tensor& grad() noexcept { return node_->grad; }
    [[nodiscard]] bool requires_grad() const noexcept { return node_->requires_grad; }

    void zero_grad();
    void backward();

    static Variable add(const Variable& a, const Variable& b);
    static Variable sub(const Variable& a, const Variable& b);
    static Variable mul(const Variable& a, const Variable& b);
    static Variable relu(const Variable& x);
    static Variable matmul(const Variable& a, const Variable& b);
    static Variable mse_loss(const Variable& pred, const Variable& target);

    // Numerical gradient checker using central finite differences
    static double max_numerical_gradient_error(
        const std::function<Variable(const Variable&)>& scalar_fn,
        Variable input,
        float eps = 1e-3f);

    [[nodiscard]] std::shared_ptr<VariableNode> node() const noexcept { return node_; }

private:
    explicit Variable(std::shared_ptr<VariableNode> n) : node_(std::move(n)) {}
    std::shared_ptr<VariableNode> node_{};
};

} // namespace dpnf::autodiff
