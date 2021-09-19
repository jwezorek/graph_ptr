#include <cstdlib>
#include <vector>
#include "graph_ptr.hpp"

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


struct A {
    A() {}
    A(int num) : num_(num) { }
    void set(graph_root_ptr<A>& self, graph_root_ptr<B>& b_ptr) { b_ptr_ = graph_ptr<B>(self, b_ptr); }

private:
    std::string msg_;
    graph_ptr<B> b_ptr_;
    int num_;
};


struct B {
    B() {}
    B(std::string msg) : msg_(msg) { }
    B(B&&) = default;
    B& operator=(B&& a) = default;
    void set(graph_root_ptr<B>& self, graph_root_ptr<C>& c_ptr) {  c_ptr_ = graph_ptr<C>(self, c_ptr); }
private:
    std::string msg_;
    graph_ptr<C> c_ptr_;
};

struct C {
    C() {}
    C(float val) : val_(val) { }
    void set(graph_root_ptr<C>& self, graph_root_ptr<A>& a_ptr) { a_ptr_ = graph_ptr<A>(self, a_ptr); }
private:
    float val_;
    graph_ptr<A> a_ptr_;
};

struct roots_t {
    std::vector<graph_root_ptr<A>> a;
    std::vector<graph_root_ptr<B>> b;
    std::vector<graph_root_ptr<C>> c;
};

void fill_graph(graph_pool& p, roots_t& roots, int verts, int edges) {
    for (int i = 0; i < verts; i++) {
        int vert_type = std::rand() % 3;
        switch (vert_type) {
        case 0:
            roots.a.push_back(p.make_root<A>(42));
            break;
        case 1:
            roots.b.push_back(p.make_root<B>("bar"));
            break;
        case 2:
            roots.c.push_back(p.make_root<C>(5.0));
            break;
        }
    }

    for (int i = 0; i < edges; ++i) {
        int vert_type = std::rand() % 3;
        switch (vert_type) {
        case 0:
            break;
        }
    }
}

 void speed_test(int num_verts, int num_edges, int collection_feq, int delete_amnt, int n) {
     graph_pool p;
     roots_t roots;

     fill_graph(p, roots, num_verts, num_edges);

     for (int i = 0; i < n; ++i) {

     }

 }