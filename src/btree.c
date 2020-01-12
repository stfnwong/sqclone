/*
 * BTREE
 * Tree for SQLite clone
 *
 * Stefan Wong 2020
 */


#include "btree.h"


// ======== Node Common ======== //
uint8_t node_common_get_type(NodeCommon* node)
{
    return (uint8_t) (node->header & 0x02);
}

uint8_t node_common_is_root(NodeCommon* node)
{
    return (uint8_t) (node->header & 0x01);
}

void node_common_set_type(NodeCommon* node, NodeType type)
{
    node->header = (node->header & 0x00000002);
    node->header = (node->header & 0x00000001) | type;
}

void node_common_set_root(NodeCommon* node)
{
    node->header = (node->header | 0x01);
}

void node_common_clear_root(NodeCommon* node)
{
    node->header = (node->header & 0x0000002);
}

