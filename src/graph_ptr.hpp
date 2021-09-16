#pragma once

#include <vector>
#include <tuple>
#include <stack>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <type_traits>
#include <algorithm>

namespace gp {

    namespace detail {

        class graph {
        public:

            void insert_edge(void* u, void* v);
            void remove_edge(void* u, void* v);
            std::unordered_set<void*> collect(const std::unordered_map<void*, int> roots);

        private:

            using adj_list = std::unordered_set<void*>;
            adj_list& get_or_create(void* v);
            void find_live_set(void* root, std::unordered_set<void*>& live);

            std::unordered_map<void*, adj_list> impl_;
        };

    }

    template<typename... Ts>
    class graph_pool {

    public:

        template<typename T>
        class base_graph_ptr;

        template<typename T>
        class graph_ptr;

        template<typename T>
        class graph_root_ptr;

        template<typename T>
        class enable_self_graph_ptr {
            friend class graph_ptr<T>;
            friend class graph_root_ptr<T>;
        public:
            const graph_ptr<T>& self_graph_ptr() const {
                return *self_;
            }

        private:
            std::unique_ptr<graph_ptr<T>> self_;
        };

        template<typename T>
        class base_graph_ptr {
        public:
            base_graph_ptr() : pool_(nullptr), v_(nullptr) {}
            base_graph_ptr(graph_pool* p, T* v) : pool_(p), v_(v) {
            }
            T* operator->() const { return v_; }
            T& operator*()  const { return *v_; }
            T* get() const { return v_; }
            explicit operator bool() const { return v_; }
        protected:
            graph_pool* pool_;
            T* v_;
        };

        template<typename T>
        class graph_ptr : public base_graph_ptr<T> {
            friend class graph_pool;
        public:

            using value_type = T;

            graph_ptr() :
                u_{ nullptr }, base_graph_ptr<T>()
            {
            }

            template<typename A, typename B>
            graph_ptr(const A& u, const B& v) :
                graph_ptr(u.pool_, u.v_, v.v_)
            {}

            graph_ptr(const graph_ptr& other) = delete;

            graph_ptr(graph_ptr&& other) noexcept :
                u_{ other.u_ }, base_graph_ptr<T>{ other.pool_, other.v_ } {
                other.wipe();
            }

            bool operator==(const graph_ptr& other) const {
                return u_ == other.u_ && this->v_ == other.v_;
            }

            graph_ptr& operator=(const graph_ptr& other) = delete;

            graph_ptr& operator=(graph_ptr&& other) noexcept {
                if (&other != this) {
                    release();

                    this->pool_ = other.pool_;
                    u_ = other.u_;
                    this->v_ = other.v_;

                    other.wipe();
                }
                return *this;
            }

            void reset() {
                release();
                wipe();
            }

            explicit operator bool() const { return this->v_; }

            ~graph_ptr() {
                release();
            }

        private:

            using non_const_type = std::remove_const_t<T>;

            void make_self_ptr() {
                if constexpr (std::is_base_of< enable_self_graph_ptr<T>, T>::value) {
                    std::unique_ptr<graph_ptr<T>>& self_ptr = static_cast<enable_self_graph_ptr<T>*>(this->v_)->self_;
                    if (!self_ptr.get()) {
                        static_cast<enable_self_graph_ptr<T>*>(this->v_)->self_ = std::unique_ptr<graph_ptr<T>>(
                            new graph_ptr<T>(this->pool_, this->v_, this->v_)
                            );
                    }
                }
            }

            void wipe() {
                this->pool_ = nullptr;
                u_ = nullptr;
                this->v_ = nullptr;
            }

            void release() {
                if (this->pool_ && this->v_)
                    this->pool_->graph_.remove_edge(u_, const_cast<non_const_type*>(this->v_));
            }

            void grab() {
                this->pool_->graph_.insert_edge(u_, const_cast<non_const_type*>(this->v_));
            }

            graph_ptr(graph_pool* gp, void* u, T* v) : u_(u), base_graph_ptr<T>(gp, v) {
                grab();
                if (u_ != this->v_) {
                    make_self_ptr();
                }
            }

            void* u_;
        };

        template<typename T>
        class graph_root_ptr : public base_graph_ptr<T> {
            friend class graph_pool;
        public:

            using value_type = T;

            graph_root_ptr()
            {
            }

            graph_root_ptr(const graph_root_ptr& v) :
                graph_root_ptr(v.pool_, v.v_) {
            }

            graph_root_ptr(const graph_ptr<T>& v) :
                graph_root_ptr(v.pool_, v.v_) {
            }

            graph_root_ptr(graph_root_ptr&& other) noexcept :
                base_graph_ptr<T>(other.pool_, other.v_) {
                other.wipe();
            }

            bool operator==(const graph_root_ptr& other) const {
                return this->v_ == other.v_;
            }

            graph_root_ptr& operator=(const graph_root_ptr& other) {
                if (&other != this) {
                    release();

                    this->pool_ = other.pool_;
                    this->v_ = other.v_;

                    grab();
                }
                return *this;
            }

            graph_root_ptr& operator=(graph_root_ptr&& other) noexcept {
                if (&other != this) {
                    release();

                    this->pool_ = other.pool_;
                    this->v_ = other.v_;

                    other.wipe();
                }
                return *this;
            }

            explicit operator bool() const { return this->v_; }

            void reset() {
                release();
                wipe();
            }

            ~graph_root_ptr() {
                release();
            }

        private:

            void make_self_ptr() {
                if constexpr (std::is_base_of< enable_self_graph_ptr<T>, T>::value) {
                    std::unique_ptr<graph_ptr<T>>& self_ptr = static_cast<enable_self_graph_ptr<T>*>(this->v_)->self_;
                    if (!self_ptr.get()) {
                        static_cast<enable_self_graph_ptr<T>*>(this->v_)->self_ = std::unique_ptr<graph_ptr<T>>(
                            new graph_ptr<T>(this->pool_, this->v_, this->v_)
                            );
                    }
                }
            }

            using non_const_type = std::remove_const_t<T>;

            void wipe() {
                this->pool_ = nullptr;
                this->v_ = nullptr;
            }

            void release() {
                if (this->pool_ && this->v_)
                    this->pool_->remove_root(const_cast<non_const_type*>(this->v_));
            }

            void grab() {
                this->pool_->add_root(const_cast<non_const_type*>(this->v_));
            }

            graph_root_ptr(graph_pool* gp, T* v) : base_graph_ptr<T>(gp, v) {
                grab();
                make_self_ptr();
            }
        };

        template<typename T, typename U, typename... Args>
        graph_ptr<T> make(graph_ptr<U> u, Args&&... args) {
            auto& p = std::get<std::vector<std::unique_ptr<T>>>(pools_);
            p.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
            auto* new_ptr = p.back().get();
            return graph_ptr(this, u.get(), new_ptr);
        }

        template<typename T, typename... Args>
        graph_root_ptr<T> make_root(Args&&... args) {
            auto& p = std::get<std::vector<std::unique_ptr<T>>>(pools_);
            p.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
            auto* new_ptr = p.back().get();
            return graph_root_ptr(this, new_ptr);
        }

        void collect() {
            auto live_set = graph_.collect(roots_);
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
        static graph_root_ptr<T> const_pointer_cast(const graph_root_ptr<U>& p) {
            return  graph_root_ptr<T>(p.pool_, const_cast<std::remove_const_t<U>*>(p.v_));
        }

    private:

        template<size_t I = 0, typename F, typename T>
        static void apply_to_pools(T& t, F func) {
            auto& pool = std::get<I>(t);
            func(pool);
            if constexpr (I + 1 != std::tuple_size<T>::value)
                apply_to_pools<I + 1>(t, func);
        }

        void add_root(void* root) {
            auto it = roots_.find(root);
            if (it != roots_.end()) {
                it->second++;
            }
            else {
                roots_[root] = 1;
            }
        }

        void remove_root(void* root) {
            auto it = roots_.find(root);
            if (--it->second == 0) {
                roots_.erase(it);
            }
        }

        std::unordered_map<void*, int> roots_;
        detail::graph graph_;
        std::tuple<std::vector<std::unique_ptr<Ts>>...> pools_;
    };

};