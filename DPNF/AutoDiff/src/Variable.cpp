#include "dpnf/autodiff/Variable.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace dpnf::autodiff {

Variable::Variable(tensor::Tensor data, bool requires_grad)
    : node_(std::make_shared<VariableNode>()) {
    node_->data = std::move(data);
    node_->requires_grad = requires_grad;
    if (requires_grad) {
        node_->grad = tensor::Tensor::zeros(node_->data.shape());
    }
}

void Variable::zero_grad() {
    if (node_ && node_->requires_grad) {
        node_->grad = tensor::Tensor::zeros(node_->data.shape());
    }
}

void Variable::backward() {
    if (!node_) return;

    std::vector<std::shared_ptr<VariableNode>> topo;
    std::unordered_set<VariableNode*> visited;

    std::function<void(const std::shared_ptr<VariableNode>&)> build_topo =
        [&](const std::shared_ptr<VariableNode>& n) {
            if (!n || visited.count(n.get()) > 0) return;
            visited.insert(n.get());
            for (const auto& p : n->parents) {
                build_topo(p);
            }
            topo.push_back(n);
        };

    build_topo(node_);

    node_->grad = tensor::Tensor::ones(node_->data.shape());

    for (auto it = topo.rbegin(); it != topo.rend(); ++it) {
        if ((*it)->backward_fn) {
            (*it)->backward_fn();
        }
    }
}

Variable Variable::add(const Variable& a, const Variable& b) {
    auto out_tensor = a.data().add(b.data()).value();
    const bool req = a.requires_grad() || b.requires_grad();
    Variable out(std::move(out_tensor), req);
    if (req) {
        auto na = a.node();
        auto nb = b.node();
        auto nout = out.node();
        std::weak_ptr<VariableNode> wout = nout;
        nout->parents = {na, nb};
        nout->backward_fn = [na, nb, wout]() {
            auto locked = wout.lock();
            if (!locked) return;
            if (na->requires_grad) {
                na->grad = na->grad.add(locked->grad).value();
            }
            if (nb->requires_grad) {
                nb->grad = nb->grad.add(locked->grad).value();
            }
        };
    }
    return out;
}

Variable Variable::sub(const Variable& a, const Variable& b) {
    auto out_tensor = a.data().sub(b.data()).value();
    const bool req = a.requires_grad() || b.requires_grad();
    Variable out(std::move(out_tensor), req);
    if (req) {
        auto na = a.node();
        auto nb = b.node();
        auto nout = out.node();
        std::weak_ptr<VariableNode> wout = nout;
        nout->parents = {na, nb};
        nout->backward_fn = [na, nb, wout]() {
            auto locked = wout.lock();
            if (!locked) return;
            if (na->requires_grad) {
                na->grad = na->grad.add(locked->grad).value();
            }
            if (nb->requires_grad) {
                nb->grad = nb->grad.sub(locked->grad).value();
            }
        };
    }
    return out;
}

Variable Variable::mul(const Variable& a, const Variable& b) {
    auto out_tensor = a.data().mul(b.data()).value();
    const bool req = a.requires_grad() || b.requires_grad();
    Variable out(std::move(out_tensor), req);
    if (req) {
        auto na = a.node();
        auto nb = b.node();
        auto nout = out.node();
        std::weak_ptr<VariableNode> wout = nout;
        nout->parents = {na, nb};
        nout->backward_fn = [na, nb, wout]() {
            auto locked = wout.lock();
            if (!locked) return;
            if (na->requires_grad) {
                auto da = locked->grad.mul(nb->data).value();
                na->grad = na->grad.add(da).value();
            }
            if (nb->requires_grad) {
                auto db = locked->grad.mul(na->data).value();
                nb->grad = nb->grad.add(db).value();
            }
        };
    }
    return out;
}

Variable Variable::relu(const Variable& x) {
    tensor::Tensor out_t(x.data().shape(), 0.0f);
    for (std::size_t i = 0; i < x.data().numel(); ++i) {
        out_t.data()[i] = std::max(0.0f, x.data().data()[i]);
    }
    const bool req = x.requires_grad();
    Variable out(std::move(out_t), req);
    if (req) {
        auto nx = x.node();
        auto nout = out.node();
        std::weak_ptr<VariableNode> wout = nout;
        nout->parents = {nx};
        nout->backward_fn = [nx, wout]() {
            auto locked = wout.lock();
            if (!locked) return;
            if (nx->requires_grad) {
                for (std::size_t i = 0; i < nx->data.numel(); ++i) {
                    if (nx->data.data()[i] > 0.0f) {
                        nx->grad.data()[i] += locked->grad.data()[i];
                    }
                }
            }
        };
    }
    return out;
}

Variable Variable::matmul(const Variable& a, const Variable& b) {
    auto out_t = a.data().matmul(b.data()).value();
    const bool req = a.requires_grad() || b.requires_grad();
    Variable out(std::move(out_t), req);
    if (req) {
        auto na = a.node();
        auto nb = b.node();
        auto nout = out.node();
        std::weak_ptr<VariableNode> wout = nout;
        nout->parents = {na, nb};
        nout->backward_fn = [na, nb, wout]() {
            auto locked = wout.lock();
            if (!locked) return;
            if (na->requires_grad) {
                auto bt = nb->data.transpose_2d().value();
                auto da = locked->grad.matmul(bt).value();
                na->grad = na->grad.add(da).value();
            }
            if (nb->requires_grad) {
                auto at = na->data.transpose_2d().value();
                auto db = at.matmul(locked->grad).value();
                nb->grad = nb->grad.add(db).value();
            }
        };
    }
    return out;
}

Variable Variable::mse_loss(const Variable& pred, const Variable& target) {
    auto diff = pred.data().sub(target.data()).value();
    auto sq = diff.mul(diff).value();
    const float loss_val = sq.mean();
    tensor::Tensor loss_t({1}, loss_val);

    const bool req = pred.requires_grad();
    Variable out(std::move(loss_t), req);
    if (req) {
        auto np = pred.node();
        auto nt = target.node();
        auto nout = out.node();
        std::weak_ptr<VariableNode> wout = nout;
        nout->parents = {np};
        nout->backward_fn = [np, nt, wout]() {
            auto locked = wout.lock();
            if (!locked) return;
            if (np->requires_grad) {
                const float scale = (2.0f / static_cast<float>(np->data.numel())) * locked->grad.data()[0];
                auto d = np->data.sub(nt->data).value().scale(scale);
                np->grad = np->grad.add(d).value();
            }
        };
    }
    return out;
}

double Variable::max_numerical_gradient_error(
    const std::function<Variable(const Variable&)>& scalar_fn,
    Variable input,
    float eps) {
    input.zero_grad();
    Variable out = scalar_fn(input);
    out.backward();
    const tensor::Tensor analytical = input.grad().clone();

    double max_err = 0.0;
    for (std::size_t i = 0; i < input.data().numel(); ++i) {
        const float orig = input.data().data()[i];

        tensor::Tensor plus_t = input.data().clone();
        plus_t.data()[i] = orig + eps;
        const float f_plus = scalar_fn(Variable(plus_t, false)).data().mean();

        tensor::Tensor minus_t = input.data().clone();
        minus_t.data()[i] = orig - eps;
        const float f_minus = scalar_fn(Variable(minus_t, false)).data().mean();

        const double num_grad = static_cast<double>(f_plus - f_minus) / (2.0 * static_cast<double>(eps));
        const double err = std::fabs(num_grad - static_cast<double>(analytical.data()[i]));
        if (err > max_err) {
            max_err = err;
        }
    }
    return max_err;
}

} // namespace dpnf::autodiff
