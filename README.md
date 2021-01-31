# graph_ptr
A smart ptr implementation that can handle cycles, similar to herb sutter's deferred_ptr but less fancy.

The user must declare which types will be managed by these pointers when creating a pool object. The pool is parametrized on the types that may have cyclic dependencies on each other that we'd like to manage with smart pointers.

Then each smart pointer is created explicitly as a directed edge of an object dependency graph: it is either a link between two objects or it is a root link.
