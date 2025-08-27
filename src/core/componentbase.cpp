#include "componentbase.h"

#include "log.h"

// TODO: This is dumb. Just read `Components::num_components` instead of jumping
// around the code. But one thing at a time.
int component_table_size(struct Components *components) {
  return components->num_components;
}

int component_index(struct Components *components, int component_bit) {
  for (int i = 0; i < component_table_size(components); ++i) {
    if (components->components[i].flag == component_bit) {
      return i;
    }
  }

  return -1;
}

int component_flag(struct Components *components, int index) {
  if (index >= component_table_size(components)) {
    return INVALID_COMPONENT_BIT;
  }

  return components->components[index].flag;
}

size_t component_size(struct Components *components, int flag) {
  int index = component_index(components, flag);
  if (index < 0) {
    LOG_WARN("Failed to find component index for bit %d\n", flag);
    return -1;
  }

  return components->components[index].size;
}

const char *component_name(struct Components *components, int index) {
  return components->components[index].name;
}
