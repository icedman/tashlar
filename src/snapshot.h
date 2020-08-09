#ifndef SNAPSHOT_H
#define SNAPSHPT_H

#include "block.h"
#include "keybinding.h"
#include <vector>

struct snapshot_t {
    block_list snapshot;
    operation_list history;

    void save(block_list& blocks);
    void restore(block_list& blocks);
};

typedef std::vector<snapshot_t> snap_list;

#endif // SNAPSHOT_H
