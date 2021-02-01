#include "graph_ptr.hpp"
#include <iostream>
#include <string>

struct A;
struct B;
struct C;

using graph_pool = gp::graph_pool<A, B, C>;

template <typename T>
using graph_ptr = graph_pool::graph_ptr<T>;

int count = 0;

struct A {
    A(std::string msg, int num) :  msg_(msg), num_(num) { ++count;}
    void set( graph_ptr<B>& b_ptr) { b_ptr_.set(this, b_ptr); }
    ~A() { --count;  std::cout << "  destroying A{ " << msg_ << " }\n"; }
private:
    std::string msg_;
    graph_ptr<B> b_ptr_;
    int num_;
};

struct B {
    B(std::string msg) : msg_(msg) { ++count;}
    void set( graph_ptr<C>& c_ptr) { c_ptr_.set(this, c_ptr);}
    ~B() { --count; std::cout << "  destroying B{ " << msg_ << " }\n"; }
private:
    std::string msg_;
    graph_ptr<C> c_ptr_;
};

struct C {
    C(std::string msg) : msg_(msg) {  ++count; }
    void set( graph_ptr<A>& a_ptr) {  a_ptr_.set(this, a_ptr); }
    ~C() { --count;  std::cout << "  destroying C{ " << msg_ << " }\n"; }
private:
    std::string msg_;
    graph_ptr<A> a_ptr_;
};



graph_ptr<A> make_cycle(graph_pool& p, std::string a_msg, std::string b_msg, std::string c_msg) {

    graph_ptr<A> a = p.make_root<A>(a_msg, 42);
    graph_ptr<B> b = p.make_root<B>(b_msg);
    graph_ptr<C> c = p.make_root<C>(c_msg);

    a->set(b);
    b->set(c);
    c->set(a);

    return a;
}

int main() { 
    std::cout << "making graph\n";
    {
        graph_pool p;

        {
            auto cycle1 = make_cycle(p, "foo", "bar", "mumble");
            auto cycle2 = make_cycle(p, "foo2", "bar2", "mumble2");

            std::cout << "count after making two cycles that have local roots is " << count << ".\n";

            cycle2.reset();

            std::cout << "after resetting cycle2, count is still " << count << " because we have not collected garbage\n";
            std::cout << "okay, collect...\n";

            p.collect();

            std::cout << "after collecting, count is  " << count << ".\n";
            std::cout << "cycles leaving scope...\n";
        }

        std::cout << "graph pool leaving scope...\n";
    }

    std::cout << "done\n";

    return 0;
}