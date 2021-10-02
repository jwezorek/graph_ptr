#include <cstdlib>
#include <vector>
#include <optional>
#include <iostream>
#include "graph_ptr.hpp"
/*
struct A;
struct B;
struct C;

using graph_pool = gp::graph_pool<A, B, C>;

template <typename T>
using graph_ptr = graph_pool::graph_ptr<T>;

template <typename T>
using graph_root_ptr = graph_pool::graph_root_ptr<T>;

template <typename T>
using enable_self_graph_ptr = graph_pool::enable_self_graph_ptr<T>;

struct A : public enable_self_graph_ptr<A> {
    A() {}
    A(int num) : val_(num) { }

    void insert_link(graph_root_ptr<B>& b_ptr) {
        if (!contains_link(b_ptr))
            links_.push_back( graph_ptr<B>(self_graph_ptr(), b_ptr)); 
    }

    bool contains_link(const graph_root_ptr<B>& b_ptr) {
        for (const auto& link : links_)
            if (link.get() == b_ptr.get())
                return true;
        return false;
    }

private:
    int val_;
    std::vector<graph_ptr<B>> links_;
};


struct B : public enable_self_graph_ptr<B> {
    B() {}
    B(std::string msg) : val_(msg) { }
    B(B&&) = default;
    B& operator=(B&& a) = default;

    void insert_link(graph_root_ptr<C>& c_ptr) { 
        if (!contains_link(c_ptr))
            links_.push_back(graph_ptr<C>(self_graph_ptr(), c_ptr)); 
    }

    bool contains_link(const graph_root_ptr<C>& c_ptr) {
        for (const auto& link : links_)
            if (link.get() ==c_ptr.get())
                return true;
        return false;
    }

private:
    std::string val_;
    std::vector<graph_ptr<C>> links_;
};

struct C : public enable_self_graph_ptr<C> {
    C() {}
    C(float val) : val_(val) { }

    void insert_link(graph_root_ptr<A>& a_ptr) { 
        if (!contains_link(a_ptr))
            links_.push_back(graph_ptr<A>(self_graph_ptr(), a_ptr)); 
    }

    bool contains_link(const graph_root_ptr<A>& a_ptr) {
        for (const auto& link : links_)
            if (link.get() == a_ptr.get())
                return true;
        return false;
    }

private:
    float val_;
    std::vector<graph_ptr<A>> links_;
};

struct roots_t {
    std::vector<graph_root_ptr<A>> a;
    std::vector<graph_root_ptr<B>> b;
    std::vector<graph_root_ptr<C>> c;

    std::tuple<int, int, int> sizes() const {
        return {
           static_cast<int>( a.size() ),
           static_cast<int>( b.size() ),
           static_cast<int>( c.size() )
        };
    }
};

template <typename T, typename U>
void add_random_link(std::vector<graph_root_ptr<T>>& src, std::vector<graph_root_ptr<U>>& dst, int initial) {
    if (initial == src.size())
        return;
    auto u = initial + std::rand() % (src.size() - initial);
    auto v = std::rand() % dst.size();
    auto& u_ptr = src[u];
    auto& v_ptr = dst[v];
    u_ptr->insert_link(v_ptr);
}

void add_n_random_verts(graph_pool& p, roots_t& roots, int n) {
    for (int i = 0; i < n; i++) {
        int vert_type = std::rand() % 3;
        switch (vert_type) {
        case 0:
            roots.a.push_back(p.make_root<A>(42));
            break;
        case 1:
            roots.b.push_back(p.make_root<B>("bar"));
            break;
        case 2:
            roots.c.push_back(p.make_root<C>( 5.0f ));
            break;
        }
    }
}

void add_n_random_edges(graph_pool& p, roots_t& roots, int n, std::optional<std::tuple<int, int, int>> init = {}) {

    std::tuple<int, int, int> initial = init ? init.value() : std::tuple<int, int, int>{0, 0, 0};
    auto [init_a, init_b, init_c] = initial;

    for (int i = 0; i < n; ++i) {
        int vert_type = std::rand() % 3;
        switch (vert_type) {
            case 0:
                add_random_link(roots.a, roots.b, init_a);
                break;
            case 1:
                add_random_link(roots.b, roots.c, init_b);
                break;
            case 2:
                add_random_link(roots.c, roots.a, init_c);
                break;
        }
    }
}

void erase_n_random_roots(roots_t& roots, int n) {
    for (int i = 0; i < n; ++i) {
        int vert_type = std::rand() % 3;
        switch (vert_type) {
            case 0:
                if (!roots.a.empty())
                    roots.a.erase(roots.a.begin());
                break;
            case 1:
                if (!roots.b.empty())
                    roots.b.erase(roots.b.begin());
                break;
            case 2:
                if (!roots.c.empty())
                    roots.c.erase(roots.c.begin());
                break;
        }
    }
}

void fill_graph(graph_pool& p, roots_t& roots, int verts, int edges) {
    add_n_random_verts(p, roots, verts);
    add_n_random_edges(p, roots, edges);
}

 void speed_test(int num_verts, int num_edges, int batch_sz, int num_batches, int delete_amnt, int relink_amnt) {
     graph_pool p;
     roots_t roots;

     fill_graph(p, roots, num_verts, num_edges);

     for (int j = 0; j < num_batches; ++j) {
         std::cout << j + 1 << " : ";
         for (int i = 0; i < batch_sz; ++i) {
             erase_n_random_roots(roots, delete_amnt);
             auto starts = roots.sizes();
             add_n_random_verts(p, roots, delete_amnt);
             add_n_random_edges(p, roots, relink_amnt, starts);
         }
         auto sz_before = p.size();
         p.collect();
         auto number_collected = sz_before - p.size();
         std::cout << number_collected << "\n";
     }
 }
 */