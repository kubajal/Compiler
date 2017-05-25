/*Compilerbau, Sommersemester 2017, Gruppe 20: Wilhelm Bomke, Jakub Jalowiec & David Bachorska  */
#include "syntree.h"
#include <stdio.h>

int syntreeInit(syntree_t* self)
{
    self->first = malloc(sizeof(struct syntree_nlist_elem));
    if(self->first == NULL)
    {
		fprintf(stderr, "Error: initializing global nodes list failed @ syntreeInit. Not enough memory?\n");
        return 1;
    }

    self->first->next = NULL;
    self->first->node = NULL;
    self->last = self->first;
    self->freeID = 1;

    return 0;
}

struct syntree_node *find_by_id(const syntree_t *self, syntree_nid i)
{
    struct syntree_nlist_elem *it = self->first;
    while(it != self->last)
    {
        if(it->node->ID == i)
            return it->node;
        it = it->next;
    }
    return NULL;
}

syntree_nid syntreeNodeNumber(syntree_t* self, int number)
{
    syntree_nid new_id = self->freeID++;
    self->last->next = malloc(sizeof(struct syntree_nlist_elem));
    if(self->last->next == NULL)
    {
		fprintf(stderr, "Error: creating a new node list element failed @ syntreeNodeNumber. Not enough memory?\n");
		return ERROR_ID;
    }

	self->last->node = malloc(sizeof(struct syntree_node));
	if(self->last->node == NULL)
	{
		fprintf(stderr, "Error: creating a new node failed @ syntreeNodeNumber. Not enough memory?\n");
		return ERROR_ID;
	}

    self->last->node->ID = new_id;
    self->last->node->value = number;
    self->last->node->parent = NULL;
    self->last->node->first = malloc(sizeof(struct syntree_nlist_elem));
    if(self->last->node->first == NULL)
    {
		fprintf(stderr, "Error: initalizing a new node's children list @ syntreeNodeNumber. Not enough memory?\n");
		return ERROR_ID;
    }
    self->last->node->last = self->last->node->first;
    self->last->node->first->next = NULL;
    self->last->node->first->node = NULL;

    self->last = self->last->next;
	self->last->next = NULL;
	self->last->node = NULL;

    return new_id;
}

syntree_nid syntreeNodeTag(syntree_t* self, syntree_nid id)
{
    syntree_nid parent = syntreeNodeNumber(self,  -1);

	if(parent == ERROR_ID)
	{
		fprintf(stderr, "Error: creating the parent node failed @ syntreeNodeTag.\n");
		return ERROR_ID;
	}

	if(syntreeNodeAppend(self, parent, id) == ERROR_ID)
	{
		fprintf(stderr, "Error: linking nodes failed @ syntreeNodeTag.\n");
		return ERROR_ID;
	}

    return parent;
}

syntree_nid syntreeNodePrepend(syntree_t* self, syntree_nid elem, syntree_nid list)     // Listenknotens (parents) de_list: old_first, ... , last
{                                                                                       //                                  new_first, old_first, ... , last
    struct syntree_node *parent = find_by_id(self, list);
    if(parent == NULL)
    {
		fprintf(stderr, "Error: list node not found, id: %d @ syntreNodePrepend.\n", list);
		return ERROR_ID;
    }
    struct syntree_node *inserted = find_by_id(self, elem);
    if(inserted == NULL)
    {
		fprintf(stderr, "Error: failed to prepend node (id: %d) to the list node (id: %d). Node %d not found @ syntreNodePrepend.\n", elem, list, elem);
		return ERROR_ID;
    }

    struct syntree_nlist_elem *old_first = parent->first;
    struct syntree_nlist_elem *new_first = malloc(sizeof(struct syntree_nlist_elem));
    if(new_first == NULL)
    {
		fprintf(stderr, "Error: making place for a new node (id: %d) in the list node's (id: %d) children list failed @ syntreNodePrepend. Not enough memory?\n", elem, list);
		return ERROR_ID;
    }
    new_first->node = inserted;
    inserted->parent = parent;
    new_first->next = old_first;
    parent->first = new_first;

    return list;
}

syntree_nid syntreeNodeAppend(syntree_t* self, syntree_nid list, syntree_nid elem)      // Listenknotens (parents) de_list: first, ..., old_last
{                                                                                       //                                  first, ..., old_last(aka inserted), new_last
    struct syntree_node *parent = find_by_id(self, list);
    if(parent == NULL)
    {
		fprintf(stderr, "Error: list node not found, id: %d @ syntreNodeAppend.\n", list);
		return ERROR_ID;
    }
    struct syntree_node *inserted = find_by_id(self, elem);
	if(inserted == NULL)
    {
		fprintf(stderr, "Error: failed to append node (id: %d) to the list node (id: %d). Node %d not found @ syntreNodeAppend.\n", elem, list, elem);
		return ERROR_ID;
    }
    struct syntree_nlist_elem *old_last = parent->last;
    struct syntree_nlist_elem *new_last;

    old_last->node = inserted;
    inserted->parent = parent;
    old_last->next = malloc(sizeof(struct syntree_nlist_elem));
    new_last = old_last->next;
    if(new_last == NULL)
    {
		fprintf(stderr, "Error: making place for a new node (id: %d) in the list node's (id: %d) children list failed @ syntreNodeAppend. Not enough memory?\n", elem, list);
		return ERROR_ID;
    }
    new_last->node = NULL;
    new_last->next = NULL;
    parent->last = new_last;

    return list;
}

syntree_nid syntreeNodePair(syntree_t* self, syntree_nid id1, syntree_nid id2)
{
    syntree_nid parent = syntreeNodeNumber(self, -1);
	if(parent == ERROR_ID)
	{
		fprintf(stderr, "Error: creating parent failed @ syntreeNodePair.\n");
		return ERROR_ID;
	}
	if(syntreeNodeAppend(self, parent, id1) == ERROR_ID)
	{
		fprintf(stderr, "Error: pairing %d failed (pairing with: %d) @ syntreeNodePair.\n", id1, id2);
		return ERROR_ID;
	}
	if(syntreeNodeAppend(self, parent, id2) == ERROR_ID)
	{
		fprintf(stderr, "Error: pairing %d failed (pairing with: %d) @ syntreeNodePair.\n", id2, id1);
		return ERROR_ID;
	}
    return parent;
}

void syntreePrint(const syntree_t* self, syntree_nid root)
{
    struct syntree_node *cur = find_by_id(self, root);

    struct syntree_nlist_elem *it = cur->first;
    if(cur->first == cur->last)
		printf("(%d)", cur->value);
    else
        printf("{");
    while(it != cur->last)
    {
        syntreePrint(self, it->node->ID);
        it = it->next;
    }
    if(cur->last != cur->first)
        printf("}");
}

void syntreeRelease(syntree_t* self)
{
    struct syntree_nlist_elem *it = self->first;
    while(it != self->last)
    {
        struct syntree_nlist_elem *it1 = it->node->first;
        while(it1 != it->node->last)
        {
            struct syntree_nlist_elem *tmp = it1->next;
            free(it1);
            it1 = tmp;
        }
		free(it1);	// it->node->last
		free(it->node);
		struct syntree_nlist_elem *tmp = it->next;
        free(it);
        it = tmp;
    }
	free(it);	// free last
}
