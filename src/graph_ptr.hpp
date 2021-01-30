#pragma once

#include <vector>
#include <tuple>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>

namespace gp {

    namespace detail {

        template<typename T>
        class graph
        {
        public:

        private:
        };

    }

    template<typename... Ts> 
    class graph_pool {

    public:

        template<typename T>
        class graph_ptr {
            friend class graph_pool;
        private:

            graph_ptr(graph_pool& gp, T* p) : pool_(gp), v_(p), u_(nullptr) {}

            T* v_;
            void* u_;
            graph_pool& pool_;
        };

        template<typename T, typename... Us>
        graph_ptr<T> make(Us... args) {
            auto& p = std::get<std::vector<std::unique_ptr<T>>>(pools_);
            p.emplace_back(std::make_unique<T>(std::forward<Us...>(args...)));
            return graph_ptr(*this, p.back().get());
        }

    private:

        std::unordered_set<void*> roots_;
        std::tuple<std::vector<std::unique_ptr<Ts>>...> pools_;
    };

};