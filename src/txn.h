/*
 * Copyright (C) 2005-2012 Christoph Rupp (chris@crupp.de).
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * See files COPYING.* for License information.
 */

/**
 * @brief transactions
 *
 */

#ifndef HAM_TXN_H__
#define HAM_TXN_H__

#include "internal_fwd_decl.h"
#include "rb.h"
#include "util.h"
#include "error.h"

/**
 * A helper structure; ham_txn_t is declared in ham/hamsterdb.h as an
 * opaque C structure, but internally we use a C++ class. The ham_txn_t
 * struct satisfies the C compiler, and internally we just cast the pointers.
 */
struct ham_txn_t
{
  int dummy;
};


namespace ham {

class Transaction;

/**
 * a single operation in a transaction
 */
typedef struct txn_op_t
{
  /** flags and type of this operation; defined in txn.h */
  ham_u32_t _flags;

  /** the original flags of this operation */
  ham_u32_t _orig_flags;

  /** the referenced duplicate id (if neccessary) - used if this is
   * i.e. a ham_cursor_erase, ham_cursor_overwrite or ham_cursor_insert
   * with a DUPLICATE_AFTER/BEFORE flag
   * this is 1-based (like dupecache-index, which is also 1-based) */
  ham_u32_t _referenced_dupe;

  /** the Transaction of this operation */
  Transaction *_txn;

  /** the parent node */
  struct txn_opnode_t *_node;

  /** next in linked list (managed in txn_opnode_t) */
  struct txn_op_t *_node_next;

  /** previous in linked list (managed in txn_opnode_t) */
  struct txn_op_t *_node_prev;

  /** next in linked list (managed in Transaction) */
  struct txn_op_t *_txn_next;

  /** previous in linked list (managed in Transaction) */
  struct txn_op_t *_txn_prev;

  /** the log serial number (lsn) of this operation */
  ham_u64_t _lsn;

  /** if this op overwrites or erases another operation in the same node,
   * then _other_lsn stores the lsn of the other txn_op (only used when
   * the other txn_op is a duplicate) */
  ham_u64_t _other_lsn;

  /** the record */
  ham_record_t _record;

  /** a linked list of cursors which are attached to this txn_op */
  struct txn_cursor_t *_cursors;

} txn_op_t;

/** a NOP operation (empty) */
#define TXN_OP_NOP      0x000000u

/** txn operation is an insert */
#define TXN_OP_INSERT     0x010000u

/** txn operation is an insert w/ overwrite */
#define TXN_OP_INSERT_OW  0x020000u

/** txn operation is an insert w/ duplicate */
#define TXN_OP_INSERT_DUP   0x040000u

/** txn operation erases the key */
#define TXN_OP_ERASE    0x080000u

/** txn operation was already flushed */
#define TXN_OP_FLUSHED    0x100000u

/** get flags */
#define txn_op_get_flags(t)        (t)->_flags

/** set flags */
#define txn_op_set_flags(t, f)       (t)->_flags=f

/** get flags original flags of ham_insert/ham_cursor_insert/ham_erase... */
#define txn_op_get_orig_flags(t)       (t)->_orig_flags

/** set flags original flags of ham_insert/ham_cursor_insert/ham_erase... */
#define txn_op_set_orig_flags(t, f)    (t)->_orig_flags=f

/** get the referenced duplicate id */
#define txn_op_get_referenced_dupe(t)    (t)->_referenced_dupe

/** set the referenced duplicate id */
#define txn_op_set_referenced_dupe(t, id)  (t)->_referenced_dupe=id

/** get the Transaction pointer */
#define txn_op_get_txn(t)          (t)->_txn

/** set the Transaction pointer */
#define txn_op_set_txn(t, txn)       (t)->_txn=txn

/** get the parent node pointer */
#define txn_op_get_node(t)         (t)->_node

/** set the parent node pointer */
#define txn_op_set_node(t, n)        (t)->_node=n

/** get next txn_op_t structure */
#define txn_op_get_next_in_node(t)     (t)->_node_next

/** set next txn_op_t structure */
#define txn_op_set_next_in_node(t, n)    (t)->_node_next=n

/** get previous txn_op_t structure */
#define txn_op_get_previous_in_node(t)   (t)->_node_prev

/** set previous txn_op_t structure */
#define txn_op_set_previous_in_node(t, p)  (t)->_node_prev=p

/** get next txn_op_t structure */
#define txn_op_get_next_in_txn(t)      (t)->_txn_next

/** set next txn_op_t structure */
#define txn_op_set_next_in_txn(t, n)     (t)->_txn_next=n

/** get previous txn_op_t structure */
#define txn_op_get_previous_in_txn(t)    (t)->_txn_prev

/** set next txn_op_t structure */
#define txn_op_set_previous_in_txn(t, p)   (t)->_txn_prev=p

/** get lsn */
#define txn_op_get_lsn(t)          (t)->_lsn

/** set lsn */
#define txn_op_set_lsn(t, l)         (t)->_lsn=l

/** get lsn of other op */
#define txn_op_get_other_lsn(t)      (t)->_other_lsn

/** set lsn of other op */
#define txn_op_set_other_lsn(t, l)     (t)->_other_lsn=l

/** get record */
#define txn_op_get_record(t)         (&(t)->_record)

/** get cursor list */
#define txn_op_get_cursors(t)        (t)->_cursors

/** set cursor list */
#define txn_op_set_cursors(t, c)       (t)->_cursors=c

/**
 * add a cursor to this txn_op structure
 */
extern void
txn_op_add_cursor(txn_op_t *op, struct txn_cursor_t *cursor);

/**
 * remove a cursor from this txn_op structure
 */
extern void
txn_op_remove_cursor(txn_op_t *op, struct txn_cursor_t *cursor);

/**
 * returns true if the op is in a txn which has a conflict
 */
extern ham_bool_t
txn_op_conflicts(txn_op_t *op, Transaction *current_txn);

class TransactionTree;

/*
 * a node in the red-black Transaction tree (implemented in rb.h);
 * a group of Transaction operations which modify the same key
 */
typedef struct txn_opnode_t
{
  /** red-black tree stub */
  rb_node(struct txn_opnode_t) node;

  /** the database - need this pointer for the compare function */
  Database *_db;

  /** this is the key which is modified */
  ham_key_t _key;

  /** the parent tree */
  TransactionTree *_tree;

  /** the linked list of operations - head is oldest operation */
  txn_op_t *_oldest_op;

  /** the linked list of operations - tail is newest operation */
  txn_op_t *_newest_op;

} txn_opnode_t;

/** get the database */
#define txn_opnode_get_db(t)          (t)->_db

/** set the database */
#define txn_opnode_set_db(t, db)        (t)->_db=db

/** get pointer to the modified key */
#define txn_opnode_get_key(t)           (&(t)->_key)

/** set pointer to the modified key */
#define txn_opnode_set_key(t, k)        (t)->_key=(*k)

/** get pointer to the parent tree */
#define txn_opnode_get_tree(t)          (t)->_tree

/** set pointer to the parent tree */
#define txn_opnode_set_tree(t, tree)      (t)->_tree=tree

/** get pointer to the first (oldest) node in list */
#define txn_opnode_get_oldest_op(t)       (t)->_oldest_op

/** set pointer to the first (oldest) node in list */
#define txn_opnode_set_oldest_op(t, o)      (t)->_oldest_op=o

/** get pointer to the last (newest) node in list */
#define txn_opnode_get_newest_op(t)       (t)->_newest_op

/** set pointer to the last (newest) node in list */
#define txn_opnode_set_newest_op(t, n)      (t)->_newest_op=n


/*
 * Each Database has a binary tree which stores the current Transaction
 * operations
 */
class TransactionTree
{
  public:
    /** constructor */
    TransactionTree(Database *db);

 // private:
    /* the Database for all operations in this tree */
    Database *m_db;

    /* stuff for rb.h */
    txn_opnode_t *rbt_root;
    txn_opnode_t rbt_nil;
};

/** get the database handle of this txn tree */
#define txn_optree_get_db(t)    (t)->m_db

/**
 * traverses a tree; for each node, a callback function is executed
 */
typedef void(*txn_tree_enumerate_cb)(txn_opnode_t *node, void *data);

/**
 * retrieves the first (=smallest) node of the tree, or NULL if the
 * tree is empty
 */
extern txn_opnode_t *
txn_tree_get_first(TransactionTree *tree);

/**
 * retrieves the last (=largest) node of the tree, or NULL if the
 * tree is empty
 */
extern txn_opnode_t *
txn_tree_get_last(TransactionTree *tree);

extern void
txn_tree_enumerate(TransactionTree *tree, txn_tree_enumerate_cb cb, void *data);


/**
 * frees all nodes in the tree
 */
extern void
txn_free_optree(TransactionTree *tree);


/**
 * a Transaction structure
 */
class Transaction
{
  private:
    enum {
      /** Transaction was aborted */
      TXN_STATE_ABORTED   = 0x10000,
      /** Transaction was committed */
      TXN_STATE_COMMITTED = 0x20000
    };

  public:
    /** constructor; "begins" the Transaction
     * supported flags: HAM_TXN_READ_ONLY, HAM_TXN_TEMPORARY, ...
     */
    Transaction(Environment *env, const char *name, ham_u32_t flags);

    /** destructor; frees all txn_op_t structures associated with this
     * Transaction */
    ~Transaction();

    /** commits the Transaction */
    ham_status_t commit(ham_u32_t flags = 0);

    /** aborts the Transaction */
    ham_status_t abort(ham_u32_t flags = 0);

    /** returns true if the Transaction was aborted */
    bool is_aborted() const {
      return (m_flags & TXN_STATE_ABORTED) != 0;
    }

    /** returns true if the Transaction was committed */
    bool is_committed() const {
      return (m_flags & TXN_STATE_COMMITTED) != 0;
    }

    /** get the id */
    ham_u64_t get_id() const {
      return (m_id);
    }

    /** sets the id */
    void set_id(ham_u64_t id) {
      m_id = id;
    }

    /** get the environment pointer */
    Environment *get_env() const {
      return (m_env);
    }

    /** get the txn name */
    const std::string &get_name() const {
      return (m_name);
    }

    /** set the txn name */
    void set_name(const char *name) {
      m_name = name;
    }

    /** get the flags */
    ham_u32_t get_flags() const {
      return (m_flags);
    }

    /** set the flags */
    void set_flags(ham_u32_t flags) {
      m_flags = flags;
    }

    /** get the cursor refcount */
    ham_size_t get_cursor_refcount() const {
      return (m_cursor_refcount);
    }

    /** set the cursor refcount */
    void set_cursor_refcount(ham_size_t count) {
      m_cursor_refcount = count;
    }

    /** get the index of the log file descriptor */
    // TODO make this private
    int get_log_desc() const {
      return (m_log_desc);
    }

    /** set the index of the log file descriptor */
    // TODO make this private
    void set_log_desc(int desc) {
      m_log_desc = desc;
    }

    /** get the remote database handle */
    ham_u64_t get_remote_handle() const {
      return (m_remote_handle);
    }

    /** set the remote database handle */
    void set_remote_handle(ham_u64_t h) {
      m_remote_handle = h;
    }

    /** get the 'newer' pointer of the linked list */
    Transaction *get_newer() const {
      return (m_newer);
    }

    /** set the 'newer' pointer of the linked list */
    void set_newer(Transaction *n) {
      m_newer = n;
    }

    /** get the 'older' pointer of the linked list */
    Transaction *get_older() const {
      return (m_older);
    }

    /** set the 'older' pointer of the linked list */
    void set_older(Transaction *o) {
      m_older = o;
    }

    /** get the oldest transaction operation */
    // TODO make this private
    txn_op_t *get_oldest_op() const {
      return (m_oldest_op);
    }

    /** set the oldest transaction operation */
    // TODO make this private
    void set_oldest_op(txn_op_t *op) {
      m_oldest_op = op;
    }

    /** get the newest transaction operation */
    // TODO make this private
    txn_op_t *get_newest_op() const {
      return (m_newest_op);
    }

    /** set the newest transaction operation */
    // TODO make this private
    void set_newest_op(txn_op_t *op) {
      m_newest_op = op;
    }

    /** Get the memory buffer for the key data */
    ByteArray &get_key_arena() {
      return (m_key_arena);
    }

    /** Get the memory buffer for the record data */
    ByteArray &get_record_arena() {
      return (m_record_arena);
    }

    /** frees the internal txn trees, nodes and ops */
    // TODO make this private
    void free_ops();

  private:
    /** the id of this txn */
    ham_u64_t m_id;

    /** owner of this transaction */
    Environment *m_env;

    /** flags for this transaction */
    ham_u32_t m_flags;

    /** the Transaction name */
    std::string m_name;

    /** reference counter for cursors (number of cursors attached to this txn)
     */
    ham_size_t m_cursor_refcount;

    /** index of the log file descriptor for this transaction [0..1] */
    int m_log_desc;

    /** the remote database handle */
    ham_u64_t m_remote_handle;

    /** linked list of all transactions */
    Transaction *m_newer, *m_older;

    /** the linked list of operations - head is oldest operation */
    txn_op_t *m_oldest_op;

    /** the linked list of operations - tail is newest operation */
    txn_op_t *m_newest_op;

    /** this is where key->data points to when returning a
     * key to the user */
    ByteArray m_key_arena;

    /** this is where record->data points to when returning a
     * record to the user */
    ByteArray m_record_arena;
};

/**
 * get an opnode for an optree; if a node with this
 * key already exists then the existing node is returned, otherwise NULL
 *
 * flags can be HAM_FIND_GEQ_MATCH, HAM_FIND_LEQ_MATCH
 */
extern txn_opnode_t *
txn_opnode_get(Database *db, ham_key_t *key, ham_u32_t flags);

/**
 * creates an opnode for an optree; asserts that a node with this
 * key does not yet exist
 *
 * returns NULL if out of memory
 */
extern txn_opnode_t *
txn_opnode_create(Database *db, ham_key_t *key);

/**
 * insert an actual operation into the txn_tree
 */
extern txn_op_t *
txn_opnode_append(Transaction *txn, txn_opnode_t *node, ham_u32_t orig_flags,
          ham_u32_t flags, ham_u64_t lsn, ham_record_t *record);

/**
 * frees a txn_opnode_t structure, and removes it from its tree
 */
extern void
txn_opnode_free(Environment *env, txn_opnode_t *node);

/**
 * retrieves the next larger sibling of a given node, or NULL if there
 * is no sibling
 */
extern txn_opnode_t *
txn_opnode_get_next_sibling(txn_opnode_t *node);

/**
 * retrieves the previous larger sibling of a given node, or NULL if there
 * is no sibling
 */
extern txn_opnode_t *
txn_opnode_get_previous_sibling(txn_opnode_t *node);

} // namespace ham


#endif /* HAM_TXN_H__ */
