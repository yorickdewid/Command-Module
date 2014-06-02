/*
 * Command Module [BOT]
 *
 * VERSION 1.30 [STABLE]
 *
 * Copyright (c) 2011
 * All rights reserved.
 *
 */

template <class T>
tree<T>::tree()
{
	_pup=0;
	_pleft=0;
	_pright=0;
	_pdown=0;
}

template <class T>
tree<T>::tree(tree *pparent)
{
	if (pparent->_pdown) {
		_pleft=pparent->_pdown;
		while (_pleft->_pright) _pleft=_pleft->_pright;
		_pleft->_pright=this;
	} else {
		pparent->_pdown=this;
		_pleft=0;
	}
	_pup=pparent;
	_pright=0;
	_pdown=0;
}

template <class T>
tree<T>::~tree()
{
	while (_pdown) delete _pdown;
	if (_pleft) _pleft->_pright=_pright;
	if (_pright) _pright->_pleft=_pleft;
	if (_pup && _pup->_pdown==this) _pup->_pdown=_pright;
}
