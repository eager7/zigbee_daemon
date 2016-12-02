/*
 * Doubly-linked list
 * Copyright (c) 2009, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */
#include "list.h"

void dl_list_init ( struct dl_list *list )
{
	list->next = list;
	list->prev = list;
}

void dl_list_add ( struct dl_list *list, struct dl_list *item )
{
	item->next = list->next;
	item->prev = list;
	list->next->prev = item;
	list->next = item;
}

void dl_list_add_tail ( struct dl_list *list, struct dl_list *item )
{
	dl_list_add ( list->prev, item );
}

void dl_list_del ( struct dl_list *item )
{
	item->next->prev = item->prev;
	item->prev->next = item->next;
	item->next = NULL;
	item->prev = NULL;
}

int dl_list_empty ( struct dl_list *list )
{
	return list->next == list;
}

unsigned int dl_list_len ( struct dl_list *list )
{
	struct dl_list *item;
	int count = 0;
	for ( item = list->next; item != list; item = item->next )
	{
		count++;
	}
	return count;
}


