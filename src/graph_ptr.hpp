#pragma once

#include <vector>
#include <tuple>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>

namespace gp {

    namespace detail {

        class graph {
        public:

            void insert_edge(void* u, void* v) {
                auto& count = get_or_create_edge(u, v);
                ++count;
            }

            void remove_edge(void* u, void* v) {
                auto i = impl_.find(u);
                if (i == impl_.end())
                    return;
                auto& a_list = i->second;
                auto j = a_list.find(v);
                if (j == a_list.end())
                    return;
                if (--j->second == 0) {
                    a_list.erase(j);
                }
            }

        private:

            using adj_list = std::unordered_map<void*, int>;

            template <typename T>
            static T& create_mapping(std::unordered_map<void*, T>& m, void* key) {
                std::pair<void*, T> p;
                p.first = key;
                auto result = m.insert(p);
                auto iter = result.first;
                return iter->second;
            }

            adj_list& get_or_create(void* v) {
                auto iter = impl_.find(v);
                if (iter != impl_.end()) {
                    return iter->second;
                } else {
                    return create_mapping(impl_, v);
                }
            }

            int& get_or_create_edge(void* ptr_u, void* ptr_v) {
                auto& u = get_or_create(ptr_u);
                auto& v = get_or_create(ptr_v);
                auto iter = u.find(ptr_v);
                if (iter != u.end()) {
                    return iter->second;
                }  else {
                    return create_mapping(u, ptr_v);
                }
            }

            std::unordered_map<void*, adj_list> impl_;
        };

    }

    template<typename... Ts> 
    class graph_pool {

    public:

        template<typename T>
        class graph_ptr {
            friend class graph_pool;
        public:

            ~graph_ptr() {
                pool_.graph_.remove_edge(u_, v_);
            }

        private:

            graph_ptr(graph_pool& gp, void* u, T* v) : pool_(gp), u_(u), v_(v) {}

            graph_pool& pool_;
            void* u_;
            T* v_;
        };

        template<typename T, typename... Us>
        graph_ptr<T> make(void* owner, Us... args) {
            auto& p = std::get<std::vector<std::unique_ptr<T>>>(pools_);
            p.emplace_back(std::make_unique<T>(std::forward<Us...>(args...)));
            auto* new_ptr = p.back().get();
            graph_.insert_edge(owner, new_ptr);
            return graph_ptr(*this, owner, new_ptr);
        }

        template<typename T, typename... Us>
        graph_ptr<T> make_root(Us... args) {
            return make<T>(nullptr, std::forward<Us...>(args...));
        }

    private:

        detail::graph graph_;
        std::tuple<std::vector<std::unique_ptr<Ts>>...> pools_;
    };

};