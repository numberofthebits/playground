#include "log.h"
#include "game.h"
#include "arena.h"

#include <stdio.h>

#define STATIC_ARENA_SIZE 1024*1024*64

extern struct ArenaAllocator allocator;

/* void vec_test() { */
/*     Vec v = vec_create(); */

/*     VEC_RESERVE_T(&v, int, 16); */

/*     int num_elements = 16; */
/*     LOG_INFO("push %d elements", num_elements); */
/*     for (int i = 0; i < num_elements; ++i) { */
/*         VEC_PUSH_T(&v, int, i*10); */
/*         LOG_INFO("vec size %d", v.size); */
/*     } */

/*     chunk_print_hex(&v.storage); */

/*     LOG_INFO("print..."); */
/*     for (int i = 0; i < v.size; ++i) { */
/*         LOG_INFO("index #%d %d", i, VEC_GET_T(&v, int, i)); */
/*     } */

/*     LOG_INFO("erase..."); */
/*     while (v.size > 0) { */
/*         LOG_INFO("%d vec size", v.size); */
/*         VEC_ERASE_T(&v, int, 0); */
/*     } */

/*     LOG_INFO("destroy vec"); */
/*     vec_destroy(&v); */
/* } */
/*
void registry_test() {
    LOG_INFO("Registry test");
    Registry* registry = registry_create();

    System* movement_system = system_create(&movement_update, TRANSFORM_COMPONENT_BIT | PHYSICS_COMPONENT_BIT);
    System* physics_system = system_create(&physics_update, PHYSICS_COMPONENT_BIT);
    System* render_system = system_create(&render_update, RENDER_COMPONENT_BIT);
    render_system->system_impl = render_system_create();
    
    registry_add_system(registry, movement_system);
    registry_add_system(registry, physics_system);
    registry_add_system(registry, render_system);

    Entity e1 = registry_create_entity(registry);
    Entity e2 = registry_create_entity(registry);

    TransformComponent tc;
    tc.pos.x = 0.0;
    tc.pos.y = 0.0;
    PhysicsComponent pc;
    pc.velocity.x = 1.0;
    pc.velocity.y = 1.0;
    
    RenderComponent rc;
    rc.size.x = 0.10f;
    rc.size.y = 0.10f;
    rc.color.r = 255;
    rc.color.g = 127;
    rc.color.b = 0;

    registry_add_component(registry, e1, TRANSFORM_COMPONENT_BIT, &tc);
    registry_add_component(registry, e1, PHYSICS_COMPONENT_BIT, &pc);    

    registry_add_component(registry, e2, TRANSFORM_COMPONENT_BIT, &tc);
    registry_add_component(registry, e2, RENDER_COMPONENT_BIT, &rc);

    registry_add_entity(registry, e1);
    registry_add_entity(registry, e2);

    registry_update(registry);
    Sleep(1000);
    registry_update(registry);
    Sleep(1000);
    registry_remove_entity(registry, e1);
    registry_remove_entity(registry, e2);
    registry_update(registry);
}
*/

/* struct TestStruct { */
/*     int arr[9]; */
/*     double lol; */
/* }; */

/* void test_arena() {    */
/*     struct ArenaAllocator allocator; */
/*     arena_init(&allocator, 4096); */
/*     char* cptr = ArenaAlloc(&allocator, 1, char); */
/*     int* iptr = ArenaAlloc(&allocator, 1, int); */
/*     struct TestStruct* tsptr = ArenaAlloc(&allocator, 2, struct TestStruct); */
/*     exit(1); */
/* } */

int main(void) {

    if (!log_init(0)) {
        printf("Failed to initialize logger");
        return -1;
    }

    arena_init(&allocator, STATIC_ARENA_SIZE);
    
    Game* game = game_create();
    if (!game) {
        LOG_ERROR("Failed to initialize game");
        return -1;
    }
    game_setup(game);
    game_run(game);
    game_destroy(game);
    
    log_destroy();
    return 0;
}

