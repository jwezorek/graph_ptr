#pragma once

#include <vector>
#include <tuple>
#include <stack>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <type_traits>

namespace gp {

    namespace detail {

        class graph {
        public:

            void insert_edge(void* u, void* v);
            void remove_edge(void* u, void* v);
            std::unordered_set<void*> collect();

        private:

            using adj_list = std::unordered_map<void*, int>;

            // insert an item of type T, default constructed, into a map mapping void*s to Ts
            // and return a reference to the T in the map...
            template <typename T>
            static T& create_mapping(std::unordered_map<void*, T>& m, void* key) {
                return m.insert(
                    std::pair<void*, T>{ key, {} }
                ).first->second;
            }

            adj_list& get_or_create(void* v);
            int& get_or_create_edge(void* ptr_u, void* ptr_v);
            void find_live_set(void* root, std::unordered_set<void*>& live);

            std::unordered_map<void*, adj_list> impl_;
        };

    }

    template<typename... Ts> 
    class graph_pool {

    public:

        template<typename T>
        class graph_ptr;

        template<typename T>
        class enable_self_graph_ptr
        {
            friend class graph_ptr<T>;

        public:
            const graph_ptr<T>& self_graph_ptr() const {
                return *self_;
            }

        private:
            std::unique_ptr<graph_ptr<T>> self_;
        };

        template<typename T>
        class graph_ptr {
            friend class graph_pool;
        public:
            graph_ptr() : 
                pool_{ nullptr }, u_{ nullptr }, v_{nullptr}
            {
            }

            template<typename U>
            graph_ptr(graph_ptr<U> u, graph_ptr v) :
                graph_ptr(u.pool_, u.v_, v.v_) {
            }

            graph_ptr(const graph_ptr& other) : 
                    graph_ptr(other.pool_, other.u_, other.v_) {
            }

            graph_ptr(graph_ptr&& other) noexcept:
                    pool_{ other.pool_ }, u_{ other.u_ }, v_{ other.v_ } { 
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

            graph_ptr& operator=( graph_ptr&& other) noexcept {
                if (&other != this) {
                    release();

                    pool_ = other.pool_;
                    u_ = other.u_;
                    v_ = other.v_;

                    other.wipe();
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

            using non_cost_type = std::remove_const_t<T>;

            void make_self_ptr() {
                if constexpr (std::is_base_of< enable_self_graph_ptr<T>, T>::value) {
                    static_cast<enable_self_graph_ptr<T>*>(v_)->self_ = std::unique_ptr<graph_ptr<T>>(
                        new graph_ptr<T>(pool_, v_, v_)
                    );
                }
            }

            void wipe() {
                pool_ = nullptr;
                u_ = nullptr;
                v_ = nullptr;
            }

            void release() {
                if (pool_ && v_)
                    pool_->graph_.remove_edge(u_, const_cast<non_cost_type*>(v_));
            }

            void grab() {
                pool_->graph_.insert_edge(u_, const_cast<non_cost_type*>(v_));
            }

            graph_ptr(graph_pool* gp, void* u, T* v) : pool_(gp), u_(u), v_(v) {
                grab();
                if (u_ != v_) {
                    make_self_ptr();
                }
            }

            graph_pool* pool_;
            void* u_;
            T* v_;
        };

        template<typename T, typename U, typename... Args>
        graph_ptr<T> make(graph_ptr<U> u, Args&&... args) {
            auto& p = std::get<std::vector<std::unique_ptr<T>>>(pools_);
            p.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
            auto* new_ptr = p.back().get();
            return graph_ptr(this, u.get(), new_ptr);
        }

        template<typename T, typename... Us>
        graph_ptr<T> make_root(Us&&... args) {
            return make<T>(graph_ptr<T>(), std::forward<Us>(args)...);
        }

        void collect() {
            auto live_set = graph_.collect();
            apply_to_pools(pools_, 
                [&live_set](auto& pool) {
                    pool.erase(
                        std::remove_if(pool.begin(), pool.end(),
                            [&live_set](const auto& un_ptr) -> bool {
                                return live_set.find(un_ptr.get()) == live_set.end();
                            }
                        ),
                        pool.end()
                    );
                }
            );
        }
        
        size_t size() const {
            size_t sz = 0;
            apply_to_pools(pools_,
                [&sz](const auto& p) {
                    sz += p.size();
                }
            );
            return sz;
        }

        template<typename T, typename U>
        static graph_ptr<T> const_pointer_cast(graph_ptr<U> p) {
            return  graph_ptr<T>(p.pool_, p.u_, const_cast<std::remove_const_t<U>*>(p.v_));
        }

    private:

        template<size_t I = 0, typename F, typename... Tp>
        static void apply_to_pools(const std::tuple<Tp...>& t, F func) {
            const auto& pool = std::get<I>(t);
            func(pool);
            if constexpr (I + 1 != sizeof...(Tp))
                apply_to_pools<I + 1>(t, func);
        }

        template<size_t I = 0, typename F, typename... Tp>
        static void apply_to_pools(std::tuple<Tp...>& t, F func)  {
            auto& pool = std::get<I>(t);
            func(pool);
            if constexpr (I + 1 != sizeof...(Tp))
                apply_to_pools<I + 1>(t, func);
        }

        detail::graph graph_;
        std::tuple<std::vector<std::unique_ptr<Ts>>...> pools_;
    };

};