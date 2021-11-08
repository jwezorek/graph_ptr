# graph_ptr
A smart pointer implementation that can handle cycles, similar to herb sutter's deferred_ptr but less fancy.

The user must create the objects to be managed by the smart pointers using a `graph_obj_store` which serves as a factory and store over which mark&sweep can be run. Internally `graph_obj_store` packs the objects in type erased vectors.

Then each smart pointer is created explicitly as representing a directed edge of an object dependency graph: it is either a link between two objects or it is a root link, and the user can ask the pool to collect garbage which means deleting any object that cannot be reached from root links. root pointers and non-root pointers are separate types. Root pointers have value semantics i.e. they are reference counted and may be passed around by copying etc. Non-root pointers (`graph_ptr<T>` in the code) are move-only. 

If a graph_ptr is a link from an object of type U to and object of type V, its deferenced type is V and the type U is irrelavent as far as the graph_ptr is concerned. That is, the user objects being managed are the vertices of the graph, the graph_ptrs are the edges, but edges are directed -- the type of the pointer is the "to" type of the graph edge.

(this is in progress ... but sort of works right now)
