#ifndef SNAPSHOT_H
#define SNAPSHPT_H

#include "block.h"

struct snapshot_t
{
    block_list snapshot;

    void save(block_list &blocks);
    void restore(block_list &blocks);
};

#endif // SNAPSHOT_H
