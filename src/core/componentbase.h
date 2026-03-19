#ifndef COMPONENT_BASE_H
#define COMPONENT_BASE_H

#include "types.h"

#include <stdint.h>

// TODO: Capping components at 30 could be silly. It should be however many we
// need. Let's cross that bridge if we get there.
// This needs to be defined regardless of user provided component flags. It's
// part of the general API
#define INVALID_COMPONENT_BIT (1U << 30)

struct Component {
  int /*ComponentBit*/ flag;
  size_t size;
  size_t alignment;
  const char *name;
};

// Is this struct really necessary?
// Registry init doesn't take this thing
struct Components {
  uint32_t num_components;
  const struct Component *components;
};

// TODO: Just return a pointer to Component instead of all these
//       pointless functions
int component_index(struct Components *components, int component_bit);

int component_flag(struct Components *components, uint32_t index);

size_t component_size(struct Components *components, int component_bit);

uint32_t component_table_size(struct Components *components);

const char *component_name(struct Components *components, int index);

const char *component_name_bit(Components *components, int component_bit);

void component_log_bitmask(Components *components, ComponentBitmaskT bitmask);

#endif
