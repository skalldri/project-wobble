#include "linked_list.h"

// Add node "node" to the linked list that starts at "head". Use the provided accessor function
// to access the member variables containing the list elements
void linked_list_add_node(void* head, void* node, linked_list_member_accessor accessor)
{
    linked_list_node* current_node = accessor(head);

    // Fast-forward through the list until we find the tail
    // TODO: potentially cache the tail to speed this up?
    // TODO: potentially add circular linked list detection?
    while (current_node->next != NULL)
    {
        current_node = accessor(current_node->next);
    } 


    // current_node is now the tail of the list. Insert the new node
    current_node->next = node;

    // Set the "next" pointer in the newly added node to NULL since this is the new tail
    current_node = accessor(node);
    current_node->next = NULL;
}

// Remove the node "node" from the linked list that starts at "head". Use the provided accessor function
// to access the member variables containing the list elements
void linked_list_remove_node(void* head, void* node, linked_list_member_accessor accessor)
{
    linked_list_node* current_node = accessor(head);

    // Special case: deleting the head of the list
    if (head == node)
    {

    }

    // Fast-forward through the list until:
    // - The current node is NULL. This means the list was empty when we started
    // - The next node is NULL. This means we reached the end of the list and didn't find the node to remove
    // - The next node is the node we want. 
    while ((current_node != NULL) && (current_node->next != NULL) && (current_node->next != node))
    {
        current_node = accessor(current_node->next);
    }

    // List appears empty
    if (current_node == NULL)
    {
        // TODO: error
        return;
    }

    // Didn't find the node in the list
    if (current_node->next == NULL)
    {
        // TODO: error
        return;
    }

    // Remove the node by "hopping over it"
    current_node->next = accessor(current_node->next)->next;
}