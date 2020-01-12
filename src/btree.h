/*
 * BTREE
 * Tree for SQLite clone
 *
 * Stefan Wong 2020
 */

#ifndef __SQ_BTREE_H
#define __SQ_BTREE_H

#include <stdint.h>

/*
 * Common Node Header Layout
 */
typedef enum
{
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;

// ================ TREE NODES

typedef uint32_t NodeCommonHeader;

typedef struct
{
    NodeCommonHeader header;
    uint32_t key;
    void*    value
} NodeCommon;

uint8_t node_common_get_type(NodeCommon* node);
uint8_t node_common_is_root(NodeCommon* node);
void    node_common_set_type(NodeCommon* node, NodeType type);
void    node_common_set_root(NodeCommon* node);
void    node_common_clear_root(NodeCommon* node);


// Leaf node header
typedef struct
{
    uint32_t parent_ptr;
    uint32_t num_cells;
} LeafNodeHeader;


typedef struct
{
    LeafNodeHeader header;
    uint32_t key;
    void*    value;     // max size is 
} LeafNode;

LeafNode* new_leaf_node(unsigned int page_size);
uint32_t  leaf_node_get_key(LeafNode* node);


// Body node header
typedef struct
{

} BodyNodeHeader;


#endif /*__SQ_BTREE_H*/
