#include "componentbase.h"

#include "log.h"

int component_index(struct Components *components, int component_bit) {
  for (uint32_t i = 0; i < components->num_components; ++i) {
    if (components->components[i].flag == component_bit) {
      return i;
    }
  }

  return -1;
}

int component_flag(struct Components *components, uint32_t index) {
  if (index >= components->num_components) {
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
const char *component_name_bit(Components *components, int component_bit) {
  int index = component_index(components, component_bit);
  if (index < 0) {
    return nullptr;
  }
  return component_name(components, index);
}
void component_log_bitmask(Components *components, ComponentBitmaskT bitmask) {
  for (size_t i = 0; i < components->num_components; i += 1) {
    ComponentBitmaskT bit = 0x1 << i;
    if (bitmask & bit) {
      int index = component_index(components, bit);
      LOG_INFO("Bitflag %d: '%s'", bit, component_name(components, index));
    }
  }
}
