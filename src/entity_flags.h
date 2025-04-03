#ifndef ENTITY_FLAGS_H
#define ENTITY_FLAGS_H

#define ENTITY_HOSTILE 0x0
#define ENTITY_FRIENDLY 0x1
#define ENTITY_PROJECTILE 0x2
#define ENTITY_PROJECTILE_FRIENDLY (ENTITY_FRIENDLY | ENTITY_PROJECTILE)
#define ENTITY_PROJECTILE_HOSTILE ENTITY_PROJECTILE

static inline EntityFlags is_friendly(EntityFlags flags) {
  return flags & ENTITY_FRIENDLY;
}

static inline EntityFlags is_hostile(EntityFlags flags) {
  return !(flags & ENTITY_FRIENDLY);
}

static inline EntityFlags is_projectile(EntityFlags flags) {
  return flags & ENTITY_PROJECTILE;
}

static inline EntityFlags is_same_faction(EntityFlags a, EntityFlags b) {
  return ((a & ENTITY_FRIENDLY) ^ (b & ENTITY_FRIENDLY)) == 0;
}

static inline EntityFlags is_friendly_projectile(EntityFlags flags) {
  return (flags & ENTITY_PROJECTILE_FRIENDLY) == ENTITY_PROJECTILE_FRIENDLY;
}

static inline EntityFlags is_hostile_projectile(EntityFlags flags) {
  return (flags & ENTITY_PROJECTILE_HOSTILE) == ENTITY_PROJECTILE_HOSTILE;
}

/* Godbolt test
#define MOCK_FLAG 0x4000

int main(int argc, char* argv[]) {
    printf("%d\n", is_friendly(0| MOCK_FLAG));
    printf("%d\n", is_friendly(ENTITY_FRIENDLY| MOCK_FLAG));

    printf("%d\n", is_projectile(0| MOCK_FLAG));
    printf("%d\n", is_projectile(ENTITY_PROJECTILE| MOCK_FLAG));


    printf("%d\n", is_hostile(0 | MOCK_FLAG));
    printf("%d\n", is_hostile(ENTITY_FRIENDLY));

    return 0;
}
*/

#endif
