#pragma once

#include "dpnf/autodiff/Variable.h"

#include <cstddef>
#include <string>
#include <vector>

namespace dpnf::neural {

class Module {
public:
    virtual ~Module() = default;
    virtual autodiff::Variable forward(const autodiff::Variable& input) = 0;
    virtual std::vector<autodiff::Variable*> parameters() = 0;

    void zero_grad();
    [[nodiscard]] std::size_t parameter_count() const;
};

class Linear : public Module {
public:
    Linear(std::size_t in_features, std::size_t out_features);

    autodiff::Variable forward(const autodiff::Variable& input) override;
    std::vector<autodiff::Variable*> parameters() override;

    [[nodiscard]] const autodiff::Variable& weight() const noexcept { return weight_; }
    [[nodiscard]] const autodiff::Variable& bias() const noexcept { return bias_; }

private:
    std::size_t in_features_{0};
    std::size_t out_features_{0};
    autodiff::Variable weight_{};
    autodiff::Variable bias_{};
};

class AdamWOptimizer {
public:
    AdamWOptimizer(
        std::vector<autodiff::Variable*> params,
        float lr = 1e-2f,
        float weight_decay = 1e-4f);

    void zero_grad();
    void step();

private:
    std::vector<autodiff::Variable*> params_{};
    float lr_{1e-2f};
    float weight_decay_{1e-4f};
    std::size_t step_count_{0};
    std::vector<tensor::Tensor> m_{};
    std::vector<tensor::Tensor> v_{};
};

struct TrainingSummary {
    std::size_t epochs{0};
    float initial_loss{0.0f};
    float final_loss{0.0f};
    bool converged{false};
};

TrainingSummary run_supervised_training_demo(std::size_t epochs = 25);

} // namespace dpnf::neural
