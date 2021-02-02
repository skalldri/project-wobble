# User Interface Library Design
The Embedded GUI library (EMGUI) is a library designed to decouple the user interface in an embedded system from the actual user display.

## Goals

The EMGUI library has several high-level goals

- Provide a view-model UI framework for embedded systems
- Fully decouple the view implementations from the model
- Single model, multiple views
  - Support multiple views accessing the model simultaneously
- Provide a set of simple UI primitives which can be implemented on a number of IO devices
- Tree-based menu structure
- Require no dynamic allocation of memory
- Support for compile-time menu validation and runtime menu generation

## Menu structure

The simplicity of the menu structure is dictated by the simplest view. At the time of designing, the simplest view is intended to be a 128x64 pixel black and white display. This display is extremely small, which makes it difficult to render more than one menu item on the screen at any one time.

The model is tree based, with parent nodes containing many child nodes. ??? Nodes in the tree can be marked as "renderable" in which case they are displayed as part of the menu, or as "invisible" where they are not rendered. ???

To avoid allocating a dynamic number of pointers to keep track of all children of a node, each node instead keeps track of its next sibling. Traversing the menu is not expected to be a frequent operation, and so the larger cost of traversing the linked list to find a particular node is considered acceptable.

Consider this example definition of a node:

```
typedef struct _menu_node {
    menu_node* parent;          // Pointer to the parent node            
    menu_node* next_sibling;    // Pointer to the next sibling node
    menu_node* first_child;     // Pointer to the first child

} menu_node;
```
Each node keeps track of its parent node. This ensures that the we can "back out" of any node in the list without having to search the tree to find the parent node. Each node also tracks its next sibling. All siblings of a single parent node are linked together, with the final sibling null-terminating the linked list.

Parent nodes only keep track of their first child. Finding other children is done by walking the linked list of siblings of the first child.

## Helper Library

The menu system relies heavily on linked lists. To assist with manipulating the variety of linked lists in the menu system, a linked-list helper library is provided which implements the following functions:

```
// Add a linked list node to an existing linked list. 
// "head" must not be null. All lists must have an existing 
// root node.
void linked_list_add(linked_list_node* head, linked_list_node* target);

```

## Dealing with Mutliple lists in a data structure

The data structures used by the menu system contain multiple linked lists. The linked list library needs to know which list to manipulate as is performs various operations. Since each link node points to the structure which contains the link rather than the link itself, information about the location of the link inside the structure must be passed into the linked list library.

There are several ways to do this:

- Provide a "link offset" to the library, so it can index into the list
  - While this works, I don't like it since it lacks elegance and is difficult for new programmers to understand
- Use a macro to define the "linked list name" which in reality is the member variable, which generates a set of template functions for accessing the linked list
  - I dislike this approach since it results in significant code bloat (lots of duplicated functions)
  - Code duplication can be reduced by only having shim functions 

Lets examine the macro approach further.

## Defining a linked list inside a data structure

When a data structure is defined that contains a linked list, it must be declared using a set of macros which generate template functions for accessing the linked lists inside the data structure.

Consider the following data structure

```

typedef struct _example_struct {
    linked_list_node* first_list;
    linked_list_node* second_list;
} example_struct

```

To generate the helper functions for this structure, we would need to also make the following macro calls:

```
DEFINE_LINKED_LIST_MEMBER(example_struct, first_list);
DEFINE_LINKED_LIST_MEMBER(example_struct, second_list);

```

These macros would expand to the following helper functions:

```

linked_list_node* get_node_example_struct_first_list(example_struct* s)
{
    return s->first_list;
}

linked_list_node* get_node_example_struct_second_list(example_struct* s)
{
    return s->first_list;
}

```

These functions are passed in as function pointers to the linked list library, which uses them to access the correct members in the structure that the lists point to. 


## Test implementation

To test the design of the view-model system, the test implementation implements two views: a console view and a 