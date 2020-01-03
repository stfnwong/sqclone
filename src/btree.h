/*
 * BTREE
 * Implementation of BTree and B+Tree for
 * SQLite clone
 * 
 * Stefan Wong 2020
 */

#ifndef __SQ_BTREE_H
#define __SQ_BTREE_H

#include <stdint.h>

typedef enum
{
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;


/*
 * Common Node Header Layout
 */
const uint32_t NODE_TYPE_SIZE          = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET        = 0;
const uint32_t IS_ROOT_SIZE            = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET          = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE     = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET   = IS_ROOT_OFFSTE + IS_ROOT_SIZE;
const uint32_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE +\
                IS_ROOT_SIZE + PARENT_POINTER_SIZE;


/*
 * Leaf Node Header Layout
 */
const uint32_t LEAF_NODE_NUM_CELLS_SIZE   = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE      = \
                CIS_ROOT_SIEOMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

/*
 * Leaf Node Body Layout
 */
const uint32_t LEAF_NODE_KEY_SIZE   = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;     // TODO: this is part of table....

#endif /*__SQ_BTREE_H*/
