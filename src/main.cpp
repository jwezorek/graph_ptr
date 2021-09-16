#include "graph_ptr.hpp"
#include <iostream>
#include <string>

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
    A(std::string msg, int num) :  msg_(msg), num_(num) { }
    void set(graph_root_ptr<A>& self, graph_root_ptr<B>& b_ptr) { b_ptr_ = graph_ptr<B>(self, b_ptr); }
    ~A() { std::cout << "  destroying A{ " << msg_ << " }\n"; }

private:
    std::string msg_;
    graph_ptr<B> b_ptr_;
    int num_;
};


struct B  {
    B() {}
    B(std::string msg) : msg_(msg) { }
    void set(graph_root_ptr<B>& self, graph_root_ptr<C>& c_ptr) {
        c_ptr_ = graph_ptr<C>(self, c_ptr );
    }
    ~B() { std::cout << "  destroying B{ " << msg_ << " }\n"; }
private:
    std::string msg_;
    graph_ptr<C> c_ptr_;
};

struct C  {
    C() {}
    C(std::string msg) : msg_(msg) { }
    void set(graph_root_ptr<C>& self, graph_root_ptr<A>& a_ptr) {  a_ptr_ = graph_ptr<A>(self, a_ptr); }
    ~C() { std::cout << "  destroying C{ " << msg_ << " }\n"; }
private:
    std::string msg_;
    graph_ptr<A> a_ptr_;
};



graph_root_ptr<A> make_cycle(graph_pool& p, std::string a_msg, std::string b_msg, std::string c_msg) {

    auto a = p.make_root<A>(a_msg, 42);

    /*
    auto b = p.make_root<B>(b_msg);
    auto c = p.make_root<C>(c_msg);
    a->set(a, b);
    b->set(b, c);
    c->set(c, a);

    auto c_a = graph_pool::const_pointer_cast<const A>(a);
    */
    return a;
}


int main() { 
    std::cout << "making graph\n";
    {
        /*
        graph_pool p;

        {
            auto cycle1 = make_cycle(p, "foo", "bar", "mumble");
            auto cycle2 = make_cycle(p, "foo2", "bar2", "mumble2");

            std::cout << "count after making two cycles that have local roots is " << p.size() << ".\n";

            cycle2.reset();

            std::cout << "after resetting cycle2, count is still " << p.size() << " because we have not collected garbage\n";
            std::cout << "okay, collect...\n";

            p.collect();

            std::cout << "after collecting, count is  " << p.size() << ".\n";
            std::cout << "cycles leaving scope...\n";
        }

        std::cout << "graph pool leaving scope...\n";
        */
    }

    std::cout << "done\n";

    return 0;
}