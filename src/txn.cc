/*
 * Copyright (C) 2005-2012 Christoph Rupp (chris@crupp.de).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * See files COPYING.* for License information.
 *
 */

#include "config.h"

#include <string.h>

#include "db.h"
#include "env.h"
#include "error.h"
#include "freelist.h"
#include "log.h"
#include "mem.h"
#include "page.h"
#include "btree_stats.h"
#include "btree_key.h"
#include "txn.h"
#include "txn_cursor.h"
#include "cursor.h"

namespace ham {

/* stuff for rb.h */
#ifndef __ssize_t_defined
typedef signed ssize_t;
#endif
#ifndef __cplusplus
typedef int bool;
#define true 1
#define false (!true)
#endif /* __cpluscplus */

static int
__cmpfoo(void *vlhs, void *vrhs)
{
    ham_compare_func_t foo;
    txn_opnode_t *lhs=(txn_opnode_t *)vlhs;
    txn_opnode_t *rhs=(txn_opnode_t *)vrhs;
    Database *db=txn_opnode_get_db(lhs);

    ham_assert(txn_opnode_get_db(lhs)==txn_opnode_get_db(rhs));
    if (lhs==rhs)
        return (0);

    foo=db->get_compare_func();

    return (foo((ham_db_t *)db,
                (ham_u8_t *)txn_opnode_get_key(lhs)->data,
                txn_opnode_get_key(lhs)->size,
                (ham_u8_t *)txn_opnode_get_key(rhs)->data,
                txn_opnode_get_key(rhs)->size));
}

rb_wrap(static, rbt_, TransactionTree, txn_opnode_t, node, __cmpfoo)

void
txn_op_add_cursor(txn_op_t *op, struct txn_cursor_t *cursor)
{
    ham_assert(!txn_cursor_is_nil(cursor));

    txn_cursor_set_coupled_next(cursor, txn_op_get_cursors(op));
    txn_cursor_set_coupled_previous(cursor, 0);

    if (txn_op_get_cursors(op)) {
        txn_cursor_t *old=txn_op_get_cursors(op);
        txn_cursor_set_coupled_previous(old, cursor);
    }

    txn_op_set_cursors(op, cursor);
}

void
txn_op_remove_cursor(txn_op_t *op, struct txn_cursor_t *cursor)
{
    ham_assert(!txn_cursor_is_nil(cursor));

    if (txn_op_get_cursors(op)==cursor) {
        txn_op_set_cursors(op, txn_cursor_get_coupled_next(cursor));
        if (txn_cursor_get_coupled_next(cursor))
            txn_cursor_set_coupled_previous(txn_cursor_get_coupled_next(cursor),
                            0);
    }
    else {
        if (txn_cursor_get_coupled_next(cursor))
            txn_cursor_set_coupled_previous(txn_cursor_get_coupled_next(cursor),
                            txn_cursor_get_coupled_previous(cursor));
        if (txn_cursor_get_coupled_previous(cursor))
            txn_cursor_set_coupled_next(txn_cursor_get_coupled_previous(cursor),
                            txn_cursor_get_coupled_next(cursor));
    }
    txn_cursor_set_coupled_next(cursor, 0);
    txn_cursor_set_coupled_previous(cursor, 0);
}

ham_bool_t
txn_op_conflicts(txn_op_t *op, Transaction *current_txn)
{
    Transaction *optxn=txn_op_get_txn(op);
    if (optxn->is_aborted())
        return (HAM_FALSE);
    else if (optxn->is_committed() || current_txn==optxn)
        return (HAM_FALSE);
    else /* txn is still active */
        return (HAM_TRUE);
}

txn_opnode_t *
txn_tree_get_first(TransactionTree *tree)
{
    if (tree)
        return (rbt_first(tree));
    else
        return (0);
}

txn_opnode_t *
txn_tree_get_last(TransactionTree *tree)
{
    if (tree)
        return (rbt_last(tree));
    else
        return (0);
}

txn_opnode_t *
txn_opnode_get_next_sibling(txn_opnode_t *node)
{
    return (rbt_next(txn_opnode_get_tree(node), node));
}

txn_opnode_t *
txn_opnode_get_previous_sibling(txn_opnode_t *node)
{
    return (rbt_prev(txn_opnode_get_tree(node), node));
}

void
txn_tree_enumerate(TransactionTree *tree, txn_tree_enumerate_cb cb, void *data)
{
    txn_opnode_t *node=rbt_first(tree);

    while (node) {
        cb(node, data);
        node=rbt_next(tree, node);
    }
}

static void *
__copy_key_data(Allocator *alloc, ham_key_t *key)
{
    void *data=0;

    if (key->data && key->size) {
        data=(void *)alloc->alloc(key->size);
        if (!data)
            return (0);
        memcpy(data, key->data, key->size);
    }

    return (data);
}

txn_opnode_t *
txn_opnode_get(Database *db, ham_key_t *key, ham_u32_t flags)
{
    txn_opnode_t *node=0, tmp;
    TransactionTree *tree=db->get_optree();
    int match=0;

    if (!tree)
        return (0);

    /* create a temporary node that we can search for */
    memset(&tmp, 0, sizeof(tmp));
    txn_opnode_set_key(&tmp, key);
    txn_opnode_set_db(&tmp, db);

    /* search if node already exists - if yes, return it */
    if ((flags&HAM_FIND_GEQ_MATCH)==HAM_FIND_GEQ_MATCH) {
        node=rbt_nsearch(tree, &tmp);
        if (node)
            match=__cmpfoo(&tmp, node);
    }
    else if ((flags&HAM_FIND_LEQ_MATCH)==HAM_FIND_LEQ_MATCH) {
        node=rbt_psearch(tree, &tmp);
        if (node)
            match=__cmpfoo(&tmp, node);
    }
    else if (flags&HAM_FIND_GT_MATCH) {
        node=rbt_search(tree, &tmp);
        if (node)
            node=txn_opnode_get_next_sibling(node);
        else
            node=rbt_nsearch(tree, &tmp);
        match=1;
    }
    else if (flags&HAM_FIND_LT_MATCH) {
        node=rbt_search(tree, &tmp);
        if (node)
            node=txn_opnode_get_previous_sibling(node);
        else
            node=rbt_psearch(tree, &tmp);
        match=-1;
    }
    else
        return (rbt_search(tree, &tmp));

    /* tree is empty? */
    if (!node)
        return (0);

    /* approx. matching: set the key flag */
    if (match<0)
        ham_key_set_intflags(key, (ham_key_get_intflags(key)
                        & ~BtreeKey::KEY_IS_APPROXIMATE) | BtreeKey::KEY_IS_LT);
    else if (match>0)
        ham_key_set_intflags(key, (ham_key_get_intflags(key)
                        & ~BtreeKey::KEY_IS_APPROXIMATE) | BtreeKey::KEY_IS_GT);

    return (node);
}

txn_opnode_t *
txn_opnode_create(Database *db, ham_key_t *key)
{
    txn_opnode_t *node=0;
    TransactionTree *tree=db->get_optree();
    Allocator *alloc=db->get_env()->get_allocator();

    /* make sure that a node with this key does not yet exist */
    ham_assert(txn_opnode_get(db, key, 0)==0);

    /* create the new node (with a copy for the key) */
    node=(txn_opnode_t *)alloc->alloc(sizeof(*node));
    if (!node)
        return (0);
    memset(node, 0, sizeof(*node));
    txn_opnode_set_key(node, key);
    txn_opnode_get_key(node)->data=__copy_key_data(alloc, key);
    txn_opnode_set_db(node, db);
    txn_opnode_set_tree(node, tree);

    /* store the node in the tree */
    rbt_insert(tree, node);

    return (node);
}

txn_op_t *
txn_opnode_append(Transaction *txn, txn_opnode_t *node, ham_u32_t orig_flags,
                    ham_u32_t flags, ham_u64_t lsn, ham_record_t *record)
{
    Allocator *alloc=txn->get_env()->get_allocator();
    txn_op_t *op;

    /* create and initialize a new txn_op_t structure */
    op=(txn_op_t *)alloc->alloc(sizeof(*op));
    if (!op)
        return (0);
    memset(op, 0, sizeof(*op));
    txn_op_set_flags(op, flags);
    txn_op_set_orig_flags(op, orig_flags);
    txn_op_set_lsn(op, lsn);
    txn_op_set_txn(op, txn);
    txn_op_set_node(op, node);

    /* create a copy of the record structure */
    if (record) {
        ham_record_t *oprec=txn_op_get_record(op);
        *oprec=*record;
        if (record->size && record->data) {
            oprec->data=alloc->alloc(record->size);
            if (!oprec->data) {
                alloc->free(op);
                return (0);
            }
            memcpy(oprec->data, record->data, record->size);
        }
        else {
            oprec->size=0;
            oprec->data=0;
        }
    }

    /* store it in the chronological list which is managed by the node */
    if (!txn_opnode_get_newest_op(node)) {
        ham_assert(txn_opnode_get_oldest_op(node)==0);
        txn_opnode_set_newest_op(node, op);
        txn_opnode_set_oldest_op(node, op);
    }
    else {
        txn_op_t *newest=txn_opnode_get_newest_op(node);
        txn_op_set_next_in_node(newest, op);
        txn_op_set_previous_in_node(op, newest);
        txn_opnode_set_newest_op(node, op);
    }

    /* store it in the chronological list which is managed by the transaction */
    if (!txn->get_newest_op()) {
        ham_assert(txn->get_oldest_op()==0);
        txn->set_newest_op(op);
        txn->set_oldest_op(op);
    }
    else {
        txn_op_t *newest=txn->get_newest_op();
        txn_op_set_next_in_txn(newest, op);
        txn_op_set_previous_in_txn(op, newest);
        txn->set_newest_op(op);
    }

    return (op);
}

void
txn_free_optree(TransactionTree *tree)
{
    Environment *env=txn_optree_get_db(tree)->get_env();
    txn_opnode_t *node;

    while ((node=rbt_last(tree))) {
        txn_opnode_free(env, node);
    }

    rbt_new(tree);
}

void
txn_opnode_free(Environment *env, txn_opnode_t *node)
{
    ham_key_t *key;

    TransactionTree *tree=txn_opnode_get_tree(node);
    rbt_remove(tree, node);

    key=txn_opnode_get_key(node);
    if (key->data)
        env->get_allocator()->free(key->data);

    env->get_allocator()->free(node);
}

static void
txn_op_free(Environment *env, Transaction *txn, txn_op_t *op)
{
    ham_record_t *rec;
    txn_op_t *next, *prev;
    txn_opnode_t *node;

    rec=txn_op_get_record(op);
    if (rec->data) {
        env->get_allocator()->free(rec->data);
        rec->data=0;
    }

    /* remove 'op' from the two linked lists */
    next=txn_op_get_next_in_node(op);
    prev=txn_op_get_previous_in_node(op);
    if (next)
        txn_op_set_previous_in_node(next, prev);
    if (prev)
        txn_op_set_next_in_node(prev, next);

    next=txn_op_get_next_in_txn(op);
    prev=txn_op_get_previous_in_txn(op);
    if (next)
        txn_op_set_previous_in_txn(next, prev);
    if (prev)
        txn_op_set_next_in_txn(prev, next);

    /* remove this op from the node */
    node=txn_op_get_node(op);
    if (txn_opnode_get_oldest_op(node)==op)
        txn_opnode_set_oldest_op(node, txn_op_get_next_in_node(op));

    /* if the node is empty: remove the node from the tree */
    if (txn_opnode_get_oldest_op(node)==0)
        txn_opnode_free(env, node);

    env->get_allocator()->free(op);
}

Transaction::Transaction(Environment *env, const char *name, ham_u32_t flags)
  : m_id(0), m_env(env), m_flags(flags), m_cursor_refcount(0), m_log_desc(0),
    m_remote_handle(0), m_newer(0), m_older(0), m_oldest_op(0), m_newest_op(0) {
  m_id = env->get_txn_id() + 1;
  env->set_txn_id(m_id);
  if (name)
    m_name = name;
  if (!(flags & HAM_TXN_TEMPORARY)) {
    get_key_arena().set_allocator(env->get_allocator());
    get_record_arena().set_allocator(env->get_allocator());
  }

  /* link this txn with the Environment */
  env_append_txn(env, this);
}

Transaction::~Transaction()
{
    free_ops();

    /* fix double linked transaction list */
    if (get_older())
        get_older()->set_newer(get_newer());
    if (get_newer())
        get_newer()->set_older(get_older());
}

ham_status_t
Transaction::commit(ham_u32_t flags)
{
    /* are cursors attached to this txn? if yes, fail */
    ham_assert(get_cursor_refcount()==0);

    /* this transaction is now committed!  */
    set_flags(get_flags()|TXN_STATE_COMMITTED);

    /* now flush all committed Transactions to disk */
    return (get_env()->flush_committed_txns());
}

ham_status_t
Transaction::abort(ham_u32_t flags)
{
    /* are cursors attached to this txn? if yes, fail */
    if (get_cursor_refcount()) {
        ham_trace(("Transaction cannot be aborted till all attached "
                    "Cursors are closed"));
        return (HAM_CURSOR_STILL_OPEN);
    }

    /* this transaction is now aborted!  */
    set_flags(get_flags()|TXN_STATE_ABORTED);

    /* immediately release memory of the cached operations */
    free_ops();

    /* clean up the changeset */
    get_env()->get_changeset().clear();

    return (0);
}

void
Transaction::free_ops()
{
    Environment *env=get_env();
    txn_op_t *n, *op=get_oldest_op();

    while (op) {
        n=txn_op_get_next_in_txn(op);
        txn_op_free(env, this, op);
        op=n;
    }

    set_oldest_op(0);
    set_newest_op(0);
}

TransactionTree::TransactionTree(Database *db)
  : m_db(db)
{
  rbt_new(this);
}

} // namespace ham
