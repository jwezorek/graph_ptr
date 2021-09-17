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

        using id_type = uint64_t;

        template<typename T>
        class slab {
        public:

            slab() : id_ptr_(nullptr) {
            }

            void initialize(id_type& curr_id, size_t capacity) {
                id_ptr_ = &curr_id;
                slab_.reserve(capacity);
                id_to_obj_.reserve(capacity);
                obj_to_id_.reserve(capacity);
            }

            template<typename... Args>
            id_type make(Args&&... args) { 
                slab_.emplace_back(std::forward<Args>(args)...);
                auto* obj_ptr = &(slab_.back());
                auto obj_id = ++(*id_ptr_);
                id_to_obj_[obj_id] = obj_ptr;
                obj_to_id_[obj_ptr] = obj_id;
                return obj_id;
            }

            const T* get(id_type obj_id) const {
                return id_to_obj_.at(obj_id);
            }

            T* get(id_type obj_id) {
                return id_to_obj_.at(obj_id);
            }

            void collect(const std::unordered_set<id_type>& live_set) { 
                if (slab_.empty())
                    return;
                
                auto front = slab_.begin();
                auto back = std::prev(slab_.end());
                
                while (front <= back) { 
                    bool back_is_alive = delete_dead_from_back(live_set, back);
                    if (back_is_alive && back > front && !is_live(live_set, front)) {
                        swap_items(front, back);
                    }
                    front++; 
                }
                slab_.resize(id_to_obj_.size()); 
            }

            bool contains(id_type obj_id) const {
                return id_to_obj_.find(obj_id) != id_to_obj_.end();
            }

            auto size() const {
                return slab_.size();
            }

            auto begin() const {
                return slab_.begin();
            }

            auto end() const {
                return slab_.end();
            }

        private:

            using iterator = typename std::vector<T>::iterator;

            void swap_items(iterator i, iterator j) { 
                T* i_ptr = &(*i);
                auto i_id = obj_to_id_.at(i_ptr);
                T* j_ptr = &(*j);
                auto j_id = obj_to_id_.at(j_ptr);

                std::swap( *i, *j );
                std::swap(obj_to_id_[i_ptr], obj_to_id_[j_ptr]);
                std::swap(id_to_obj_[i_id], id_to_obj_[j_id]); 
            }

            bool delete_dead_from_back(const std::unordered_set<id_type>& live_set, iterator& back) {
                bool past_begin = false;
                while (!past_begin && !is_live(live_set, back)) {
                    delete_item(back);
                    if (back != slab_.begin())
                        back--;
                    else
                        past_begin = true;
                }
                return !past_begin;
            }

            void delete_item(iterator i) {
                T* i_ptr = &(*i);
                auto i_id = obj_to_id_.at(i_ptr);
                obj_to_id_.erase(i_ptr);
                id_to_obj_.erase(i_id);
            }

            bool is_live(const std::unordered_set<id_type>& live_set, iterator it) {
                auto index = it - slab_.begin();
                T* obj = &(slab_[index]);
                auto id_iter = obj_to_id_.find(obj);
                if (id_iter == obj_to_id_.end())
                    return false;
                return live_set.find(id_iter->second) != live_set.end();
            }

            id_type* id_ptr_;
            std::vector<T> slab_;
            std::unordered_map<id_type, T*> id_to_obj_;
            std::unordered_map<T*, id_type> obj_to_id_;
        };

        class graph {
        public:

            void insert_edge(id_type u, id_type v);
            void remove_edge(id_type u, id_type v);
            std::unordered_set<id_type> collect(const std::unordered_map<id_type, int> roots);

        private:

            using adj_list = std::unordered_set<id_type>;
            adj_list& get_or_create(id_type v);
            void find_live_set(id_type root, std::unordered_set<id_type>& live);

            std::unordered_map<id_type, adj_list> impl_;
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
                return {}; // *self_;
            }

        private:
            //std::unique_ptr<graph_ptr<T>> self_; 
        };

        template<typename T>
        class base_graph_ptr {
        public:
            base_graph_ptr() : pool_(nullptr), v_id_( 0 ) {}
            base_graph_ptr(graph_pool* p, detail::id_type v) : pool_(p), v_id_(v) {
            }
            const T* operator->() const { return get(); }
            T* operator->()  { return get(); }
            T& operator*()  { return *get(); }
            const T& operator*()  const { return *get(); }
            T* get() { return pool_->get_slab<T>().get(v_id_); }
            const T* get() const { return pool_->get_slab<T>().get(v_id_); }
            explicit operator bool() const { return v_; }
        protected:
            graph_pool* pool_;
            detail::id_type v_id_;
        };

        template<typename T>
        class graph_ptr : public base_graph_ptr<T> {
            friend class graph_pool;
        public:

            using value_type = T;

            graph_ptr() :
                u_id_{ 0 }, base_graph_ptr<T>()
            {
            }

            template<typename A, typename B>
            graph_ptr(const A& u, const B& v) :
                graph_ptr(u.pool_, u.v_id_, v.v_id_)
            {}

            graph_ptr(const graph_ptr& other) = delete;

            graph_ptr(graph_ptr&& other) noexcept :
                u_id_{ other.u_id_ }, base_graph_ptr<T>{ other.pool_, other.v_id_ } {
                other.wipe();
            }

            bool operator==(const graph_ptr& other) const {
                return u_id_ == other.u_id_ && v_id_ == other.v_id_;
            }

            graph_ptr& operator=(const graph_ptr& other) = delete;

            graph_ptr& operator=(graph_ptr&& other) noexcept {
                if (&other != this) {
                    release();

                    this->pool_ = other.pool_;
                    this->u_id_ = other.u_id_;
                    this->v_id_ = other.v_id_;

                    other.wipe();
                }
                return *this;
            }

            void reset() {
                release();
                wipe();
            }

            explicit operator bool() const { return v_id_; }

            ~graph_ptr() {
                release();
            }

        private:

            using non_const_type = std::remove_const_t<T>;

            void make_self_ptr() { /*
                if constexpr (std::is_base_of< enable_self_graph_ptr<T>, T>::value) {
                    T* ptr = this->get(); 
                    std::unique_ptr<graph_ptr<T>>& self_ptr = static_cast<enable_self_graph_ptr<T>*>(ptr)->self_;
                    if (!self_ptr.get()) {
                        static_cast<enable_self_graph_ptr<T>*>(ptr)->self_ = std::unique_ptr<graph_ptr<T>>(
                            new graph_ptr<T>(this->pool_, this->v_id_, this->v_id_)
                        );
                    }
                }
                */
            }

            void wipe() {
                this->pool_ = nullptr;
                u_id_ = 0;
                this->v_id_ = 0;
            }

            void release() {
                if (this->pool_ && this->v_id_)
                    this->pool_->graph_.remove_edge( u_id_, this->v_id_ );
            }

            void grab() {
                this->pool_->graph_.insert_edge( u_id_, this->v_id_ );
            }

            graph_ptr(graph_pool* gp, detail::id_type u, detail::id_type v) : u_id_(u), base_graph_ptr<T>(gp, v) {
                grab();
                if ( u_id_ != this->v_id_ ) {
                    make_self_ptr();
                }
            }

            detail::id_type u_id_;
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
                graph_root_ptr(v.pool_, v.v_id_) {
            }

            graph_root_ptr(const graph_ptr<T>& v) :
                graph_root_ptr(v.pool_, v.v_id_) {
            }

            graph_root_ptr(graph_root_ptr&& other) noexcept :
                base_graph_ptr<T>(other.pool_, other.v_id_) {
                other.wipe();
            }

            bool operator==(const graph_root_ptr& other) const {
                return v_ == other.v_;
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

            explicit operator bool() const { return v_; }

            void reset() {
                release();
                wipe();
            }

            ~graph_root_ptr() {
                release();
            }

        private:

            void make_self_ptr() {
                /*
                if constexpr (std::is_base_of< enable_self_graph_ptr<T>, T>::value) {
                    std::unique_ptr<graph_ptr<T>>& self_ptr = static_cast<enable_self_graph_ptr<T>*>(this->v_)->self_;
                    if (!self_ptr.get()) {
                        static_cast<enable_self_graph_ptr<T>*>(this->v_)->self_ = std::unique_ptr<graph_ptr<T>>(
                            new graph_ptr<T>(this->pool_, this->v_, this->v_)
                            );
                    }
                }
                */
            }

            using non_const_type = std::remove_const_t<T>;

            void wipe() {
                this->pool_ = nullptr;
                this->v_id_ = 0;
            }

            void release() {
                if (this->pool_ && this->v_id_)
                    this->pool_->remove_root( this->v_id_ );
            }

            void grab() {
                this->pool_->add_root( this->v_id_ );
            }

            graph_root_ptr(graph_pool* gp, detail::id_type v) : base_graph_ptr<T>(gp, v) {
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
            auto new_id = get_slab<T>().make(std::forward<Args>(args)...);
            return graph_root_ptr<T>(this, new_id);
        }

        void collect() {
            auto live_set = graph_.collect(roots_);
            apply_to_pools(pools_,
                [&live_set](auto& pool) {
                    pool.collect(live_set);
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
            return  graph_root_ptr<T>( p.pool_, p.v_id_ );
        }

    private:

        template<size_t I = 0, typename F, typename T>
        static void apply_to_pools(T& t, F func) {
            auto& pool = std::get<I>(t);
            func(pool);
            if constexpr (I + 1 != std::tuple_size<T>::value)
                apply_to_pools<I + 1>(t, func);
        }

        void add_root(detail::id_type root) {
            auto it = roots_.find(root);
            if (it != roots_.end()) {
                it->second++;
            } else {
                roots_[root] = 1;
            }
        }

        void remove_root(detail::id_type root) {
            auto it = roots_.find(root);
            if (--it->second == 0) {
                roots_.erase(it);
            }
        }

        template<typename T>
        detail::slab<T>& get_slab() {
            return std::get<detail::slab<T>>(pools_);
        }

        std::unordered_map<detail::id_type, int> roots_;
        detail::graph graph_;
        std::tuple<detail::slab<Ts>...> pools_;
    };

};