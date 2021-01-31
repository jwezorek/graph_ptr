#include "graph_ptr.hpp"
#include <iostream>
#include <string>

struct a {
    a(std::string msg) : msg_(msg) {}
    std::string msg_;

    ~a() {
        std::cout << "  destroying A{ " << msg_ << " }\n";
    }
};

struct b {
    b(std::string msg) : msg_(msg) {}
    std::string msg_;

    ~b() {
        std::cout << "  destroying B{ " << msg_ << " }\n";
    }
};

struct c {

};

int main() { 
    std::cout << "making graph\n";
    {
        gp::graph_pool<a, b, c> p;

        {
            auto a_ptr = p.make_root<a>(std::string("foo"));
            auto b_ptr = p.make_root<b>(std::string("bar"));
        }
    }
    std::cout << "done\n";

    return 0;
}