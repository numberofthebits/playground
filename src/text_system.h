#ifndef TEXT_SYSTEM_H
#define TEXT_SYSTEM_H

#include <core/renderer.h>
#include <core/systembase.h>

typedef struct TextSystem {
  SystemBase base;
  Renderer text_renderer;
  GLuint program_id;
} TextSystem;

TextSystem *text_system_create(Services *services);

void text_system_update(Registry *registry, struct SystemBase *sys,
                        size_t frame_nr);

#endif
