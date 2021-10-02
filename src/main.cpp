
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

    struct cell {
        void* value_;
        size_t ref_count_;
        bool gc_mark_;
        std::unordered_map<obj_id_t, size_t> adj_list_;

        cell() : value_(nullptr), ref_count_(0), gc_mark_(false)
        {}
    };

    template<typename T>
    struct slab_item_t {
        cell* cell_ptr;
        T value;

        slab_item_t() : cell_ptr(nullptr) {

        }

        template<typename... Args>
        slab_item_t(cell* c, Args&&... args) :
            cell_ptr(c),
            value( std::forward<Args>(args)... )
        {
        }

    };

    class obj_store {
        enum class func_enum {
            collect,
            destroy,
            get_size
        };
    public:
        obj_store() : slab_(nullptr), fn_(), init_(0) {}
        obj_store(const obj_store& os) = delete;
        obj_store(obj_store&& os) noexcept : slab_(os.slab_), fn_(os.fn_), init_(os.init_) {
            os.slab_ = nullptr;
            os.fn_ = {};
        }
        obj_store& operator=(const obj_store& os) = delete;
        obj_store& operator=(obj_store&& os) noexcept {
            slab_ = os.slab_;
            fn_ = os.fn_;
            os.slab_ = nullptr;
            os.fn_ = {};
            return *this;
        }

        obj_store(size_t initial_capacity) : init_(initial_capacity), slab_(nullptr)
        {}

         template<typename T, typename... Args>
         T* emplace(cell* cell_ptr, Args&&... args) {
             if (!slab_) {
                 initialize<T>();
             }

             slab<slab_item_t<T>>* slab_ptr = static_cast<slab<slab_item_t<T>>*>(slab_);
             slab_item_t<T>* slab_item_ptr = slab_ptr->emplace(cell_ptr, std::forward<Args>(args)...);
             T* val_ptr = &(slab_item_ptr->value);
             cell_ptr->value_ = val_ptr;

             return val_ptr;
         }

         void collect() {
             fn_(func_enum::collect, slab_);
         }

         size_t size() const {
             return fn_(func_enum::get_size, slab_);
         }

         ~obj_store() {
             if (slab_) {
                 fn_(func_enum::destroy, slab_);
                 slab_ = nullptr;
             }
         }

    private:

        template<typename T>
        void initialize() {
            slab_ = new slab<slab_item_t<T>>(
                init_,
                [](const slab_item_t<T>& slab_item)->bool {
                    return !slab_item.cell_ptr->gc_mark_;
                },
                [](slab_item_t<T>& slab_item)->void {
                    slab_item.cell_ptr->value_ = static_cast<void*>(&(slab_item.value));
                }
                );
            fn_ = [](func_enum cmd, void* ptr)->size_t {
                auto slab_ptr = static_cast<slab<slab_item_t<T>>*>(ptr);
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
            };
        }

        size_t init_;
        void* slab_;
        std::function<size_t(func_enum, void*)> fn_;
    };

    class graph_obj_store {

    public:

        graph_obj_store(size_t initial_capacity) : initial_capacity_(initial_capacity) {

        }

        template<typename T, typename... Args>
        T* emplace(cell* cell_ptr, Args&&... args) {
            auto type_key = std::type_index(typeid(T));
            auto iter = type_stores_.find(type_key);

            if (iter == type_stores_.end()) {
                auto [i, success] = type_stores_.insert(std::pair<std::type_index, obj_store>(type_key, std::move(obj_store())));
                iter = i;
            }
            obj_store& objs = iter->second;
            T* new_val_ptr = objs.emplace<T>(cell_ptr, std::forward<Args>(args)...);
            return new_val_ptr;
        }

        size_t size() const {
            size_t sz = 0;
            
            for (const auto& [key, val] : type_stores_) {
                sz += val.size();
            }

            return sz;
        }

        void collect() {
            for (auto& [key, val] : type_stores_) {
                val.collect();
            }
        }

    private:
        size_t initial_capacity_;
        std::unordered_map<std::type_index, obj_store> type_stores_;
    };

}

struct A {
    int a1;
    int a2;
    A(int i = 0, int j = 0) : a1(i), a2(j) {
    }
    ~A() {
        std::cout << "A destructor" << "\n";
    }
};

struct B {
    std::string foo;

    B(std::string f = "") :foo(f) {
    }

    B(B&& b) : foo(std::move(b.foo)) {
        b.foo = "<moved>";
    }
    B(const B& b) = delete;
    B& operator=(B&& b) noexcept {
        foo = std::move(b.foo);
        return *this;
    }

    ~B() {
        if (foo != "<moved>") {
            std::cout << "B destructor: " << foo << "\n";
        }
    }
};



int main() {

    {
        gptr::graph_obj_store gos(100);

        gptr::cell c1;
        gptr::cell c2;
        gptr::cell c3;
        gptr::cell c4;

        A* a1 = gos.emplace<A>(&c1, 42, 17);

        B* b1 = gos.emplace<B>(&c2, "foobar");
        B* b2 = gos.emplace<B>(&c3, "baz");
        B* b3 = gos.emplace<B>(&c4, "quux");

        std::cout << gos.size() << "\n";
    }

}
