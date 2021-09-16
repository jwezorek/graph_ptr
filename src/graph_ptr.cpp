#include "graph_ptr.hpp"

using id_type = gp::detail::id_type;

void gp::detail::graph::insert_edge(id_type id_u, id_type id_v) {
    auto& u = get_or_create(id_u);
    auto& v = get_or_create(id_v);
    auto iter = u.find(id_v);
    if (iter == u.end()) {
        u.insert( id_v );
    }
}

void gp::detail::graph::remove_edge(id_type u, id_type v) {
    auto i = impl_.find(u);
    if (i == impl_.end())
        return;
    auto& a_list = i->second;
    auto j = a_list.find(v);
    if (j != a_list.end())
        a_list.erase(j);
}

std::unordered_set<id_type> gp::detail::graph::collect(const std::unordered_map<id_type, int> roots) {
    std::unordered_set<id_type> live;
    for (auto [root, count] : roots) {
        find_live_set(root, live);
    }

    for (auto it = impl_.begin(); it != impl_.end();) {
        if (live.find(it->first) == live.end()) {
            it = impl_.erase(it);
        } else {
            it++;
        }
    }

    return live;
}

gp::detail::graph::adj_list& gp::detail::graph::get_or_create(id_type v) {
    auto iter = impl_.find(v);
    if (iter != impl_.end()) {
        return iter->second;
    } else {
        return impl_.insert(
            std::pair<id_type, gp::detail::graph::adj_list>{ v, {} }
        ).first->second;
    }
}

void gp::detail::graph::find_live_set(id_type root, std::unordered_set<id_type>& live) {
    std::stack<id_type> stack;
    stack.push(root);
    while (!stack.empty()) {
        auto current = stack.top();
        stack.pop();

        if (live.find(current) != live.end())
            continue;
        live.insert(current);

        const auto& neighbors = impl_.at(current);
        for (const auto& neighbor : neighbors) {
            stack.push(neighbor);
        }
    }
}

