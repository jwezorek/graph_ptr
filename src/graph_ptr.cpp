#include "graph_ptr.hpp"

void gp::detail::graph::insert_edge(void* u, void* v) {
    auto& count = get_or_create_edge(u, v);
    ++count;
}

void gp::detail::graph::remove_edge(void* u, void* v) {
    auto i = impl_.find(u);
    if (i == impl_.end())
        return;
    auto& a_list = i->second;
    auto j = a_list.find(v);
    if (j == a_list.end())
        return;
    if (--(j->second) == 0) {
        a_list.erase(j);
    }
}

std::unordered_set<void*> gp::detail::graph::collect() {
    std::unordered_set<void*> live;
    const auto& roots = impl_.at(nullptr);
    for (auto [root, count] : roots) {
        find_live_set(root, live);
    }

    for (auto it = impl_.begin(); it != impl_.end();) {
        if (live.find(it->first) == live.end()) {
            it = impl_.erase(it);
        }
        else {
            it++;
        }
    }

    return live;
}

gp::detail::graph::adj_list& gp::detail::graph::get_or_create(void* v) {
    auto iter = impl_.find(v);
    if (iter != impl_.end()) {
        return iter->second;
    }
    else {
        return create_mapping(impl_, v);
    }
}

int& gp::detail::graph::get_or_create_edge(void* ptr_u, void* ptr_v) {
    auto& u = get_or_create(ptr_u);
    auto& v = get_or_create(ptr_v);
    auto iter = u.find(ptr_v);
    if (iter != u.end()) {
        return iter->second;
    }
    else {
        return create_mapping(u, ptr_v);
    }
}

void gp::detail::graph::find_live_set(void* root, std::unordered_set<void*>& live) {
    std::stack<void*> stack;
    stack.push(root);
    while (!stack.empty()) {
        auto current = stack.top();
        stack.pop();

        if (live.find(current) != live.end())
            continue;
        live.insert(current);

        const auto& neighbors = impl_.at(current);
        for (const auto& [neighbor, count] : neighbors) {
            stack.push(neighbor);
        }
    }
}

