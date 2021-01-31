# graph_ptr
A smart ptr implementation that can handle cycles, similar to herb sutter's deferred_ptr but less fancy.

The user must declare which types will be managed by these pointers when creating a pool object. The pool is parametrized on the types that may have cyclic dependencies on each other that we'd like to manage with smart pointers.

Then each smart pointer is created explicitly as represeting a directed edge of an object dependency graph: it is either a link between two objects or it is a root link, and the user can ask the pool to collect garbage which means deleting any object that cannot be reached from root links. If a graph_ptr is a link from an object of type U to and object of type V, its deferenced type is V and the type U is irrelavent as far as the graph_ptr is concerned. That is, the user objects being managed are the vertices of the graph, the graph_ptrs are the edges, but edges are directed -- the type of the pointer is the "to" type of the graph edge.
