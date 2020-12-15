#include "linked_list.h"

// Define a custom data structure to use for the test
typedef struct _test_struct {
    linked_list_node* forwards;
    linked_list_node* backwards;
} test_struct;

DEFINE_LINKED_LIST_MEMBER(test_struct, forwards);
DEFINE_LINKED_LIST_MEMBER(test_struct, backwards);

// The root node of the linked list
linked_list_node head;

// Other nodes in the list
linked_list_node list[10];

void main()
{
    w
}