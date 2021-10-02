#pragma once

#include <stdexcept>
#include <any>
#include <unordered_map>
#include <tuple>

namespace gptr {

    class graph_pool;

    template<typename T>
    class graph_ptr;

    template<typename T>
    class graph_root_ptr {
        friend graph_pool;

    public:
        graph_root_ptr() : pool_(nullptr), v_(0){
        }
        
        const T* operator->() const { return get(); }
        T* operator->() { return get(); }
        T& operator*() { return *get(); }
        const T& operator*()  const { return *get(); }
        T* get() { return pool_->get<T>(v_); }
        const T* get() const { return pool_->get<T>(v_); }
        explicit operator bool() const { return v_; }
        
    private:

        graph_root_ptr(graph_pool& pool, size_t id) : pool_(&pool), v_(id) {
        }

        graph_pool* pool_;
        size_t v_;
    };

    class graph_pool {

    public:

        template<typename T, typename U, typename... Args>
        graph_ptr<T> make(graph_ptr<U> u, Args&&... args) {
        }

        template<typename T, typename... Args>
        graph_root_ptr<T> make_root_ptr(Args&&... args) {
            return graph_root_ptr<T>( *this, insert_obj<T>(std::forward<Args>(args)...));
        }

        void collect() {
        }

        size_t size() const {
        }

        graph_pool(size_t initial_capacity = 1024) : id_(0) {
        }

   // private:

        template<typename T>
        struct cell {
            T value_;
            size_t id_;
            bool marked_;

            cell()
            {}

            template<typename... Args>
            cell(size_t id, Args&&... args) :
                value_(std::forward<Args>(args)...),
                id_(id),
                marked_(false)
            {}

            // std::any needs to be copy constructible...
            cell(const cell<T>&) {
                throw std::runtime_error("graph_ptr cells cannot be copied");
            }
        };

        template<typename T, typename... Args>
        size_t insert_obj(Args&&... args) {
            auto id = ++id_;
            std::any c;
            c.emplace<cell<T>>(id, std::forward<Args>(args)...);
            id_to_cells_[id] = std::move(c);
            return id;
        }

        template<typename T>
        T* get(size_t id) {
            auto& any_cell = id_to_cells_.at(id);
            return &(std::any_cast<cell<T>&>(any_cell).value_);
        }

        size_t id_;
        std::unordered_map<size_t, std::any> id_to_cells_;
    };
}