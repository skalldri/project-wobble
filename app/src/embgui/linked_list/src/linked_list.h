#pragma once

/*
* asdasd
* asdasd
*
*/

typedef struct _linked_list_node {
    void* next;

    // More data structure contents below

} linked_list_node;

// A function pointer definition used by the linked list library.
// The library uses these functions to index into user-defined structures to 
// find linked_list_node* members.
// This accessor function is defined for each structure / member combination
// by the DEFINE_LINKED_LIST_MEMBER() macro.
typedef void* (linked_list_member_accessor)(void* s);

// A macro to define the function name for a linked_list_member_accessor.
// Commonly used in linked list library macros to get a function pointer
#define LINKED_LIST_MEMBER_ACCESSOR_NAME(s, m) \
    get_node_ ## s ## _ ## m

// A macro to define a linked_list_member_accessor function
// This function is a helper method that must be available to the linked list library
#define DEFINE_LINKED_LIST_MEMBER(s, m) \
    linked_list_node* LINKED_LIST_MEMBER_ACCESSOR_NAME(s, m)(void* in) \
    { \
        return ((s*)in)->m; \
    }

// The library function for adding a node to an existing linked list.
// This function takes the linked_list_member_accessor function as an argument,
// and passes it into any functions it calls that need it.
void linked_list_add_node(void* head, void* node, linked_list_member_accessor accessor);

// The library function for removing a node from an existing linked list.
// This function takes the linked_list_member_accessor function as an argument,
// and passes it into any functions it calls that need it.
void linked_list_remove_node(void* head, void* node, linked_list_member_accessor accessor);
