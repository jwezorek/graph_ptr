#include <iostream>
#include <string>
#include "graph_ptr.hpp"

template <class T, class U, class V>
gptr::graph_root_ptr<T> make_cycle(gptr::ptr_graph& g, std::string str1, std::string str2, std::string str3) {
    auto t = g.make_root<T>(str1);
    auto u = g.make_root<U>(str2);
    auto v = g.make_root<V>(str3);

    t->ptr = gptr::graph_ptr<U>(t, u);
    u->ptr = gptr::graph_ptr<V>(u, v);
    v->ptr = gptr::graph_ptr<T>(v, t);

    return t;
}

struct B;
struct C;

struct A {

    A(std::string str = {}) : val(str)
    {}

    std::string val;
    gptr::graph_ptr<B> ptr;

};

struct B {

    B(std::string str = {}) : val(str)
    {}

    std::string val;
    gptr::graph_ptr<C> ptr;

};

struct C {

    C(std::string str = {}) : val(str)
    {}

    std::string val;
    gptr::graph_ptr<A> ptr;

};

struct D : gptr::enable_self_ptr<D> {

    D() {
    }

    D(gptr::ptr_graph& g, std::string str1, std::string str2, std::string str3) : gptr::enable_self_ptr<D>(g) {
        a = g.make<A>(self_ptr(), str1);
        b = g.make<B>(self_ptr(), str2);
        c = g.make<C>(self_ptr(), str3);

        c->ptr = gptr::graph_ptr<A>(c, a);
    }

    gptr::graph_ptr<A> a;
    gptr::graph_ptr<B> b;
    gptr::graph_ptr<C> c;
};

int main() {

    {
        gptr::ptr_graph g(100);
        auto cycle1 = make_cycle<A, B, C>(g, "foo", "bar", "baz");
        auto cycle2 = make_cycle<C, A, B>(g, "quux", "mumble", "???");

        auto d = g.make_root<D>(g, "a", "b", "c");

        std::cout << "built a graph of 2 three node cycles\n";
        std::cout << "and a root pointing to 3 nodes created with self_ptrs\n";
        std::cout << "current number of objects allocated: " << g.size() << "\n\n";

        std::cout << "resetting root of one of the cycles and the tree root...\n";
        cycle2.reset();
        d.reset();
        std::cout << "current number of objects allocated: " << g.size() << "\n\n";

        std::cout << g.debug_graph();
        std::cout << "\n";

        std::cout << "collecting...\n";

        g.collect();
        std::cout << "current number of objects allocated: " << g.size() << "\n\n";

        std::cout << g.debug_graph();
        std::cout << "\n";

    }

}
