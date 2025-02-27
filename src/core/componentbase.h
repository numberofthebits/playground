#ifndef COMPONENT_BASE_H
#define COMPONENT_BASE_H

// TODO: Capping components at 30 could be silly. It should be however many we
// need.
//       Let's cross that bridge if we get there.
// This needs to be defined regardless of user provided component flags. It's
// part of the general API
#define INVALID_COMPONENT_BIT (1U << 30)

struct Component {
  int /*ComponentBit*/ flag;
  size_t size;
  size_t alignment;
  const char *name;
};

struct Components {
  size_t num_components;
  const struct Component *components;
};

int component_index(struct Components *components, int component_bit);

int component_flag(struct Components *components, int index);

size_t component_size(struct Components *components, int component_bit);

int component_table_size(struct Components *components);

const char *component_name(struct Components *components, int index);

#endif
