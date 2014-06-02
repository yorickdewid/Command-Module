/*
 * Command Module [BOT]
 *
 * VERSION 1.30 [STABLE]
 *
 * Copyright (c) 2011
 * All rights reserved.
 *
 */

#ifndef _INCL_TREE_H
#define _INCL_TREE_H

#include <windows.h>

template <class T>
class tree
{
public:
	tree *_pup;
	tree *_pleft;
	tree *_pright;
	tree *_pdown;
	T _data;

	tree();
	tree(tree *pparent);
	~tree();
};

#endif
