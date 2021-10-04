
#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <tuple>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <stack>

namespace gptr {/*
    template <typename T>
    using should_collect_cb = std::function<bool(const T&)>;

    template <typename T>
    using on_moved_cb = std::function<void(T&)>;
    */

    template <typename T>
    using should_collect_cb = bool(*)(const T&);

    template <typename T>
    using on_moved_cb = void(*)(T&);

    template<typename T>
    class slab {
    public:
        slab() {

        }

        slab(size_t initial_capacity, should_collect_cb<T> sc, on_moved_cb<T> om) :
            is_dead_(sc), on_moved_(om) {
            impl_.reserve(initial_capacity);
        }

        template<typename... Args>
        T* emplace(Args&&... args) {
            impl_.emplace_back(std::forward<Args>(args)...);
            return &(impl_.back());
        }

        void collect() {
            if (impl_.empty())
                return;

            auto front = impl_.begin();
            auto back = std::prev(impl_.end());

            size_t total_collected = 0;
            while (front <= back) {
                auto [back_is_alive, num_collected] = delete_dead_from_back(back);
                total_collected += num_collected;
                if (back_is_alive && back > front && is_dead_(*front)) {
                    std::swap( std::move(*front), std::move(*back) );

                    on_moved_(*front);
                }
                front++;
            }

            impl_.resize(impl_.size() - total_collected);
        }

        size_t size() const {
            return impl_.size();
        }

        using iterator = typename std::vector<T>::iterator;
        using const_iterator = typename std::vector<T>::const_iterator;

        iterator begin() {
            return impl_.begin();
        }

        iterator end() {
            return impl_.end();
        }

        const_iterator begin() const {
            return impl_.begin();
        }

        const_iterator end() const {
            return impl_.end();
        }

    private:

        std::tuple<bool, size_t> delete_dead_from_back(iterator& back) {
            size_t num_collected = 0;
            bool past_begin = false;
            while (!past_begin && is_dead_(*back)) {
                if (back != impl_.begin()) {
                    --back;
                }
                else {
                    past_begin = true;
                }
                ++num_collected;
            }
            return { !past_begin, num_collected };
        }

        std::vector<T> impl_;
        should_collect_cb<T> is_dead_;
        on_moved_cb<T> on_moved_;
    };

    using obj_id_t = size_t;

    class any_slab {
        enum class func_enum {
            collect,
            destroy,
            get_size
        };
    public:
        
        any_slab(const any_slab& os) = delete;
        any_slab(any_slab&& os) noexcept : slab_(os.slab_), fn_(os.fn_) {
            os.slab_ = nullptr;
            os.fn_ = {};
        }
        any_slab& operator=(const any_slab& os) = delete;
        any_slab& operator=(any_slab&& os) noexcept {
            slab_ = os.slab_;
            fn_ = os.fn_;
            os.slab_ = nullptr;
            os.fn_ = {};
            return *this;
        }

        template<typename T>
        any_slab(size_t initial_capacity, should_collect_cb<T> is_dead_fn, on_moved_cb<T> on_moved_fn) :
            slab_(
                new slab<T>(initial_capacity, is_dead_fn, on_moved_fn)
            ),
            fn_(
                [](func_enum cmd, void* ptr)->size_t {
                    auto slab_ptr = static_cast<slab<T>*>(ptr);
                    switch (cmd) {
                    case func_enum::collect:
                        slab_ptr->collect();
                        return 0;

                    case func_enum::get_size:
                        return slab_ptr->size();

                    case func_enum::destroy:
                        delete slab_ptr;
                        return 0;
                    };
                    return 0;
                }
            )
        { }

         template<typename T, typename... Args>
         T* emplace(Args&&... args) {
             slab<T>* slab_ptr = static_cast<slab<T>*>(slab_);
             return slab_ptr->emplace( std::forward<Args>(args)... );
         }

         void collect() {
             fn_(func_enum::collect, slab_);
         }

         size_t size() const {
             return fn_(func_enum::get_size, slab_);
         }

         ~any_slab() {
             if (slab_) {
                 fn_(func_enum::destroy, slab_);
                 slab_ = nullptr;
             }
         }

    private:

        void* slab_;
        std::function<size_t(func_enum, void*)> fn_;
    };

    struct cell {
        void* value_;
        bool gc_mark_;
        std::unordered_map<obj_id_t, size_t> adj_list_;

        cell(void* value = nullptr) : value_(value), gc_mark_(false)
        {}
    };
    
    class graph_obj_store {

        template<typename T>
        struct slab_item_t {
            cell* cell_ptr;
            T value;
            
            slab_item_t() : cell_ptr(nullptr) {
            }

            template<typename... Args>
            slab_item_t(cell* c, Args&&... args) :
                cell_ptr(c),
                value(std::forward<Args>(args)...) {
            }
        };

    public:

        graph_obj_store(size_t initial_capacity) : initial_capacity_(initial_capacity) {
        }

        template<typename T, typename... Args>
        T* emplace(cell* cell_ptr, Args&&... args) {
            auto type_key = std::type_index(typeid(T));
            auto iter = type_to_slab_.find(type_key);

            if (iter == type_to_slab_.end()) {
                auto [i, success] = type_to_slab_.insert(
                    std::pair<std::type_index, any_slab>(
                        type_key, 
                        any_slab(
                            initial_capacity_,
                            should_collect_cb< slab_item_t<T>>(
                                [](const slab_item_t<T>& si) {
                                    return si.cell_ptr->gc_mark_;
                                }
                            ),
                            on_moved_cb< slab_item_t<T>>(
                                [](slab_item_t<T>& si) {
                                    si.cell_ptr->value_ = &(si.value);
                                }
                            )
                        )
                    )
                );
                iter = i;
            }
            any_slab& objs = iter->second;
            slab_item_t<T>* new_slab_item_ptr = objs.emplace<slab_item_t<T>>(cell_ptr, std::forward<Args>(args)...);
            cell_ptr->value_ = &(new_slab_item_ptr->value);
            return &(new_slab_item_ptr->value);
        }

        size_t size() const {
            size_t sz = 0;
            
            for (const auto& [key, val] : type_to_slab_) {
                sz += val.size();
            }

            return sz;
        }

        void collect() {
            for (auto& [key, val] : type_to_slab_) {
                val.collect();
            }
        }

    private:
        size_t initial_capacity_;
        std::unordered_map<std::type_index, any_slab> type_to_slab_;
    };

    using obj_id_t = size_t;

    template<typename T>
    class graph_ptr;

    class ptr_graph;

    template<typename T>
    class graph_root_ptr  {
        friend ptr_graph;
    public:

        using value_type = T;

        graph_root_ptr() : obj_id_t(0), ptr_graph_(nullptr) {
        }

        graph_root_ptr(const graph_root_ptr& v) :
            graph_root_ptr(v.ptr_graph_, v.v_) {
        }

        graph_root_ptr(const graph_ptr<T>& v) :
            graph_root_ptr(v.ptr_graph_, v.v_) {
        }

        graph_root_ptr(graph_root_ptr&& other) noexcept : ptr_graph_(other.ptr_graph_), v_(other.v_) {
            other.wipe();
        }

        bool operator==(const graph_root_ptr& other) const {
            return v_ == other.v_;
        }

        graph_root_ptr& operator=(const graph_root_ptr& other) {
            if (&other != this) {
                release();
                this->ptr_graph_ = other.ptr_graph_;
                this->v_ = other.v_;
                grab();
            }
            return *this;
        }

        graph_root_ptr& operator=(graph_root_ptr&& other) noexcept {
            if (&other != this) {
                release();
                this->ptr_graph_ = other.ptr_graph_;
                this->v_ = other.v_;
                other.wipe();
            }
            return *this;
        }

        const T* operator->() const { return get(); }
        T* operator->() { return get(); }
        T& operator*() { return *get(); }
        const T& operator*()  const { return *get(); }
        T* get() { return ptr_graph_->get<T>(v_); }
        const T* get() const { return ptr_graph_->get<T>(v_); }
        explicit operator bool() const { return v_; }

        void reset() {
            release();
            wipe();
        }

        ~graph_root_ptr() {
            release();
        }

    private:

        using non_const_type = std::remove_const_t<T>;

        void wipe() {
            this->ptr_graph_ = nullptr;
            this->v_ = 0;
        }

        void release() {
            if (this->ptr_graph_ && this->v_)
                this->ptr_graph_->remove_root(this->v_);
        }

        void grab() {
            this->ptr_graph_->insert_root(this->v_);
        }

        graph_root_ptr(ptr_graph* gp, obj_id_t v) : ptr_graph_(gp), v_(v) {
            grab();
        }

        obj_id_t v_;
        ptr_graph* ptr_graph_;
    };

    template<typename T>
    class graph_ptr {
        friend class ptr_graph;
    public:

        using value_type = T;

        graph_ptr() :
            u_{ 0 }, v_{ 0 }, ptr_graph_(nullptr)
        {
        }

        template<typename A, typename B>
        graph_ptr(const A& u, const B& v) :
            graph_ptr(u.ptr_graph_, u.v_, v.v_)
        {}

        graph_ptr(const graph_ptr& other) = delete;

        graph_ptr(graph_ptr&& other) noexcept :
            ptr_graph_(other.ptr_graph_), u_( other.u_ ), v_(other.v_) {
            other.wipe();
        }

        bool operator==(const graph_ptr& other) const {
            return u_ == other.u_ && v_ == other.v_;
        }

        graph_ptr& operator=(const graph_ptr& other) = delete;

        graph_ptr& operator=(graph_ptr&& other) noexcept {
            if (&other != this) {
                release();

                this->ptr_graph_ = other.ptr_graph_;
                this->u_ = other.u_;
                this->v_ = other.v_;

                other.wipe();
            }
            return *this;
        }

        void reset() {
            release();
            wipe();
        }

        explicit operator bool() const { return v_ }

        ~graph_ptr() {
            release();
        }

    private:

        using non_const_type = std::remove_const_t<T>;

        void wipe() {
            ptr_graph_ = nullptr;
            u_ = 0;
            v_ = 0;
        }

        void release() {
            if (ptr_graph_ && v_)
                ptr_graph_->remove_edge(u_, v_);
        }

        void grab() {
            ptr_graph_->insert_edge(u_, v_);
        }

        graph_ptr(ptr_graph* pg, obj_id_t u, obj_id_t v_) : ptr_graph_(pg), u_(u), v_(v) {
            grab();
        }

        obj_id_t u_;
        obj_id_t v_;
        ptr_graph* ptr_graph_;
    };

    class ptr_graph {

        template<typename T>  friend class graph_root_ptr;

    public:
        ptr_graph(size_t initial_capacity) : obj_store_(initial_capacity), id_(0) {
            id_to_cell_[0] = cell(nullptr);
        }

        template<typename T, typename... Args>
        graph_root_ptr<T> make_root(Args&&... args) {
            auto id = make_new_cell<T>(std::forward<Args>(args)...);
            insert_root(id);
            return graph_root_ptr<T>(this, id);
        }

        template<typename T, typename U, typename... Args>
        graph_ptr<T> make(graph_ptr<U> u, Args&&... args) {
            auto id = make_new_cell<T>(std::forward<Args>(args)...);
            insert_edge(u.v_, id);
            return graph_ptr(this, u.v_, id);
        }

        void collect() {
            for (auto& [id, cell] : id_to_cell_) {
                cell.gc_mark_ = false;
            }

            std::stack<obj_id_t> stack;
            stack.push(0);
            
            while (!stack.empty()) {
                auto id = stack.top();
                stack.pop();
                auto& cell = id_to_cell_[id];
                cell.gc_mark_ = true;
                for (const auto& [id, count] : cell.adj_list_) {
                    stack.push(id);
                }
            }

            obj_store_.collect();
        }

    private:

        template<typename T, typename... Args>
        obj_id_t make_new_cell(Args&&... args) {
            auto id = ++id_;
            auto [iter, success] = id_to_cell_.insert({ id, cell(nullptr) });
            obj_store_.emplace<T>(&(iter->second), std::forward<Args>(args)...);
            return id;
        }

        void insert_root(obj_id_t v) {
            insert_edge(0, v);
        }

        void remove_root(obj_id_t v) {
            remove_edge(0, v);
        }

        void insert_edge(obj_id_t u_id, obj_id_t v_id) {
            cell& u = id_to_cell_[u_id];
            auto iter = u.adj_list_.find(v_id);
            if (iter == u.adj_list_.end()) {
                u.adj_list_[v_id] = 1;
            }  else {
                iter->second++;
            }
        }

        void remove_edge(obj_id_t u_id, obj_id_t v_id) {
            cell& u = id_to_cell_[u_id];
            auto iter = u.adj_list_.find(v_id);
            iter->second--;
            if (iter->second == 0)
                u.adj_list_.erase(iter);
        }

        template <typename T>
        T* get(obj_id_t v) {
            return id_to_cell_[v].value_;
        }

        obj_id_t id_;
        graph_obj_store obj_store_;
        std::unordered_map<obj_id_t, cell> id_to_cell_;
    };
    
}

struct A {
    int a1;
    int a2;
    A(int i = 0, int j = 0) : a1(i), a2(j) {
    }
    ~A() noexcept {
        std::cout << "A destructor: " << a1 << " " << a2 << "\n";
    }
};

struct B {
    std::string foo;

    B(std::string f = "") :foo(f) {
    }

    B(B&& b) : foo(std::move(b.foo)) {
    }

    B(const B& b) = delete;
    B& operator=(B&) = delete;

    B& operator=(B&& b) noexcept {
        foo = std::move(b.foo);
        return *this;
    }

    ~B() noexcept {
        std::cout << "B destructor: " << foo << "\n";
    }
};



int main() {

    {
        gptr::ptr_graph pg(100);
        auto a1 = pg.make_root<A>(42, 42);
        auto b1 = pg.make_root<B>("foobar");
    }

}
