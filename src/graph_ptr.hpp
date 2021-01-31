#pragma once

#include <vector>
#include <tuple>
#include <stack>
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
                if (--(j->second) == 0) {
                    a_list.erase(j);
                }
            }

            std::unordered_set<void*> collect() {
                std::unordered_set<void*> live;
                const auto& roots = impl_.at(nullptr);
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

            void find_live_set(void* root, std::unordered_set<void*>& live) {
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
            graph_ptr() : 
                pool_{ nullptr }, u_{ nullptr }, v_{nullptr}
            {}

            void set(void* u, graph_ptr& v) {
                release();

                pool_ = v.pool_;
                u_ = u;
                v_ = v.v_;

                grab();
            }

            graph_ptr(const graph_ptr& other) : 
                graph_ptr(other.pool_, other.u_, other.v_) {
            }

            graph_ptr(graph_ptr&& other) noexcept:
                pool_{ other.pool_ }, u_{ other.u_ }, v_{ other.v_ }
            { 
                other.wipe();
            }

            graph_ptr& operator=(const graph_ptr& other) {
                if (&other != this) {
                    release();

                    pool_ = other.pool_;
                    u_ = other.u_;
                    v_ = other.v_;

                    grab();
                }
                return *this;
            }

            graph_ptr& operator=( graph_ptr&& other) {
                if (&other != this) {
                    release();

                    pool_ = other.pool_;
                    u_ = other.u_;
                    v_ = other.v_;

                    other_.wipe();
                }
                return *this;
            }

            T* operator->() const { return v_; }
            T& operator*()  const { return *v_; }
            T* get() const { return v_; }
            explicit operator bool() const { return v_; }

            void reset() {
                release();
                wipe();
            }

            ~graph_ptr() {
                release();
            }

        private:

            void wipe() {
                pool_ = nullptr;
                u_ = nullptr;
                v_ = nullptr;
            }

            void release() {
                if (pool_ && v_)
                    pool_->graph_.remove_edge(u_, v_);
            }

            void grab() {
                pool_->graph_.insert_edge(u_, v_);
            }

            graph_ptr(graph_pool* gp, void* u, T* v) : pool_(gp), u_(u), v_(v) {
                grab();
            }

            graph_pool* pool_;
            void* u_;
            T* v_;
        };

        template <typename U, typename V>
        graph_ptr<V> make(const graph_ptr<U>& u, const graph_ptr<V>& v) {
            return graph_ptr(this, u.get(), v.get());
        }

        template <typename V>
        graph_ptr<V> make(void* u, const graph_ptr<V>& v) {
            return graph_ptr(this, u, v.get());
        }

        template<typename T, typename... Us>
        graph_ptr<T> make(void* owner, Us... args) {
            auto& p = std::get<std::vector<std::unique_ptr<T>>>(pools_);
            p.emplace_back(std::make_unique<T>(std::forward<Us...>(args...)));
            auto* new_ptr = p.back().get();
            return graph_ptr(this, owner, new_ptr);
        }

        template<typename T, typename... Us>
        graph_ptr<T> make_root(Us... args) {
            return make<T>(nullptr, std::forward<Us...>(args...));
        }

        void collect() {
            auto live_set = graph_.collect();
            collect_dead(pools_, live_set);
        }

    private:

        template<size_t I = 0, typename... Tp>
        void collect_dead(std::tuple<Tp...>& t, const std::unordered_set<void*>& live) {
            auto& pool = std::get<I>(t);
            pool.erase(
                std::remove_if(pool.begin(), pool.end(),
                    [&live](const auto& u_ptr) -> bool {
                        return live.find(u_ptr.get()) == live.end();
                    }
                ),
                pool.end()
            );
            if constexpr (I + 1 != sizeof...(Tp))
                collect_dead<I + 1>(t, live);
        }

        detail::graph graph_;
        std::tuple<std::vector<std::unique_ptr<Ts>>...> pools_;
    };

};