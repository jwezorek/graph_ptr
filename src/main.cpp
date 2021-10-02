
#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <tuple>
#include <typeinfo>
#include <typeindex>

namespace gptr {
    template <typename T>
    using should_collect_cb = std::function<bool(const T&)>;

    template <typename T>
    using on_moved_cb = std::function<void(T&)>;
    
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
        size_t ref_count_;
        bool gc_mark_;
        std::unordered_map<obj_id_t, size_t> adj_list_;

        cell() : value_(nullptr), ref_count_(0), gc_mark_(false)
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
                any_slab s(
                    initial_capacity_,
                    should_collect_cb< slab_item_t<T>>([](const slab_item_t<T>& si) {
                        return si.cell_ptr->gc_mark_;
                    }),
                    on_moved_cb< slab_item_t<T>>([](slab_item_t<T>& si) {
                        si.cell_ptr->value_ = &(si.value);
                    })
                );
                auto [i, success] = type_to_slab_.insert(std::pair<std::type_index, any_slab>(type_key, std::move(s)));
                iter = i;
            }
            any_slab& objs = iter->second;
            slab_item_t<T>* new_slab_item_ptr = objs.emplace<slab_item_t<T>>(cell_ptr, std::forward<Args>(args)...);
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
    
}

struct A {
    int a1;
    int a2;
    A(int i = 0, int j = 0) : a1(i), a2(j) {
    }
    ~A() noexcept {
        std::cout << "A destructor" << "\n";
    }
};

struct B {
    std::string foo;

    B(std::string f = "") :foo(f) {
    }

    B(B&& b) : foo(std::move(b.foo)) {
    }

    B(const B& b) = delete;
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
        gptr::graph_obj_store gos(100);

        gptr::cell c1;
        gptr::cell c2;
        gptr::cell c3;

        gos.emplace<A>(&c1, 12, 34);
        gos.emplace<A>(&c2, 42, 42);
        gos.emplace<B>(&c3, "hello there");

        std::cout << "destructors\n";
    }

}
