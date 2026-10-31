#include "dpnf/neural/Module.h"

#include <cmath>

namespace dpnf::neural {

void Module::zero_grad() {
    for (auto* p : const_cast<Module*>(this)->parameters()) {
        p->zero_grad();
    }
}

std::size_t Module::parameter_count() const {
    std::size_t total = 0;
    for (const auto* p : const_cast<Module*>(this)->parameters()) {
        total += p->data().numel();
    }
    return total;
}

Linear::Linear(std::size_t in_features, std::size_t out_features)
    : in_features_(in_features),
      out_features_(out_features) {
    tensor::Tensor w({in_features, out_features}, 0.0f);
    const float scale = 1.0f / std::sqrt(static_cast<float>(in_features));
    for (std::size_t i = 0; i < w.numel(); ++i) {
        w.data()[i] = (static_cast<float>((i * 17 + 5) % 19) - 9.0f) * 0.1f * scale;
    }
    weight_ = autodiff::Variable(std::move(w), true);
    bias_ = autodiff::Variable(tensor::Tensor::zeros({1, out_features}), true);
}

autodiff::Variable Linear::forward(const autodiff::Variable& input) {
    auto mm = autodiff::Variable::matmul(input, weight_);
    // Broadcast bias across batch rows if batch == 1 or matching
    if (mm.data().numel() == bias_.data().numel()) {
        return autodiff::Variable::add(mm, bias_);
    }
    return mm;
}

std::vector<autodiff::Variable*> Linear::parameters() {
    return {&weight_, &bias_};
}

AdamWOptimizer::AdamWOptimizer(
    std::vector<autodiff::Variable*> params,
    float lr,
    float weight_decay)
    : params_(std::move(params)),
      lr_(lr),
      weight_decay_(weight_decay) {
    for (const auto* p : params_) {
        m_.push_back(tensor::Tensor::zeros(p->data().shape()));
        v_.push_back(tensor::Tensor::zeros(p->data().shape()));
    }
}

void AdamWOptimizer::zero_grad() {
    for (auto* p : params_) {
        p->zero_grad();
    }
}

void AdamWOptimizer::step() {
    ++step_count_;
    const float beta1 = 0.9f;
    const float beta2 = 0.999f;
    const float eps = 1e-8f;
    const float bc1 = 1.0f - std::pow(beta1, static_cast<float>(step_count_));
    const float bc2 = 1.0f - std::pow(beta2, static_cast<float>(step_count_));

    for (std::size_t idx = 0; idx < params_.size(); ++idx) {
        auto* p = params_[idx];
        if (!p->requires_grad()) continue;

        float* w = p->data().data();
        const float* g = p->grad().data();
        float* m = m_[idx].data();
        float* v = v_[idx].data();

        for (std::size_t i = 0; i < p->data().numel(); ++i) {
            w[i] -= lr_ * weight_decay_ * w[i];
            m[i] = beta1 * m[i] + (1.0f - beta1) * g[i];
            v[i] = beta2 * v[i] + (1.0f - beta2) * g[i] * g[i];
            const float m_hat = m[i] / bc1;
            const float v_hat = v[i] / bc2;
            w[i] -= lr_ * m_hat / (std::sqrt(v_hat) + eps);
        }
    }
}

TrainingSummary run_supervised_training_demo(std::size_t epochs) {
    Linear l1(4, 8);
    Linear l2(8, 2);

    std::vector<autodiff::Variable*> params;
    for (auto* p : l1.parameters()) params.push_back(p);
    for (auto* p : l2.parameters()) params.push_back(p);

    AdamWOptimizer opt(params, 0.04f, 1e-4f);

    autodiff::Variable x(tensor::Tensor({1, 4}, {1.0f, 0.5f, -0.5f, 2.0f}), false);
    autodiff::Variable y_target(tensor::Tensor({1, 2}, {0.8f, -0.4f}), false);

    float initial_loss = 0.0f;
    float final_loss = 0.0f;

    for (std::size_t e = 0; e < epochs; ++e) {
        opt.zero_grad();
        auto h = autodiff::Variable::relu(l1.forward(x));
        auto pred = l2.forward(h);
        auto loss = autodiff::Variable::mse_loss(pred, y_target);
        const float loss_val = loss.data().item(0);
        if (e == 0) initial_loss = loss_val;
        final_loss = loss_val;
        loss.backward();
        opt.step();
    }

    TrainingSummary summary{};
    summary.epochs = epochs;
    summary.initial_loss = initial_loss;
    summary.final_loss = final_loss;
    summary.converged = (final_loss < initial_loss * 0.1f);
    return summary;
}

} // namespace dpnf::neural
