#include "resmngr.h"
#include <stdlib.h>
#include <string.h>
#include "renderer.h"

typedef struct resmngr {
    struct slot_map scene_map;
}* resmngr;

resmngr resmngr_create()
{
    resmngr rm = calloc(1, sizeof(*rm));
    slot_map_init(&rm->scene_map, sizeof(renderer_scene));
    return rm;
}

int resmngr_handle_valid(rid r)
{
    return slot_map_key_valid(r);
}

void resmngr_destroy(resmngr rm)
{
    for (size_t i = 0; i < rm->scene_map.size; ++i) {
        rid r = slot_map_data_to_key(&rm->scene_map, i);
        resmngr_model_delete(rm, r);
    }
    slot_map_destroy(&rm->scene_map);
    free(rm);
}

rid resmngr_model_sample(resmngr rm)
{
    rid r = slot_map_insert(&rm->scene_map, 0);
    renderer_scene* rs = slot_map_lookup(&rm->scene_map, r);
    memset(rs, 0, sizeof(*rs));

    float vertices[] = {
        /* pos                  color                      uvs */
        -1.0f, -1.0f, -1.0f,    1.0f, 0.5f, 0.5f, 1.0f,    0.0f, 0.0f,
         1.0f, -1.0f, -1.0f,    1.0f, 0.5f, 0.5f, 1.0f,    1.0f, 0.0f,
         1.0f,  1.0f, -1.0f,    1.0f, 0.5f, 0.5f, 1.0f,    1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f,    1.0f, 0.5f, 0.5f, 1.0f,    0.0f, 1.0f,

        -1.0f, -1.0f,  1.0f,    0.5f, 1.0f, 0.5f, 1.0f,    0.0f, 0.0f,
         1.0f, -1.0f,  1.0f,    0.5f, 1.0f, 0.5f, 1.0f,    1.0f, 0.0f,
         1.0f,  1.0f,  1.0f,    0.5f, 1.0f, 0.5f, 1.0f,    1.0f, 1.0f,
        -1.0f,  1.0f,  1.0f,    0.5f, 1.0f, 0.5f, 1.0f,    0.0f, 1.0f,

        -1.0f, -1.0f, -1.0f,    0.5f, 0.5f, 1.0f, 1.0f,    0.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,    0.5f, 0.5f, 1.0f, 1.0f,    1.0f, 0.0f,
        -1.0f,  1.0f,  1.0f,    0.5f, 0.5f, 1.0f, 1.0f,    1.0f, 1.0f,
        -1.0f, -1.0f,  1.0f,    0.5f, 0.5f, 1.0f, 1.0f,    0.0f, 1.0f,

         1.0f, -1.0f, -1.0f,    1.0f, 0.5f, 0.0f, 1.0f,    0.0f, 0.0f,
         1.0f,  1.0f, -1.0f,    1.0f, 0.5f, 0.0f, 1.0f,    1.0f, 0.0f,
         1.0f,  1.0f,  1.0f,    1.0f, 0.5f, 0.0f, 1.0f,    1.0f, 1.0f,
         1.0f, -1.0f,  1.0f,    1.0f, 0.5f, 0.0f, 1.0f,    0.0f, 1.0f,

        -1.0f, -1.0f, -1.0f,    0.0f, 0.5f, 1.0f, 1.0f,    0.0f, 0.0f,
        -1.0f, -1.0f,  1.0f,    0.0f, 0.5f, 1.0f, 1.0f,    1.0f, 0.0f,
         1.0f, -1.0f,  1.0f,    0.0f, 0.5f, 1.0f, 1.0f,    1.0f, 1.0f,
         1.0f, -1.0f, -1.0f,    0.0f, 0.5f, 1.0f, 1.0f,    0.0f, 1.0f,

        -1.0f,  1.0f, -1.0f,    1.0f, 0.0f, 0.5f, 1.0f,    0.0f, 0.0f,
        -1.0f,  1.0f,  1.0f,    1.0f, 0.0f, 0.5f, 1.0f,    1.0f, 0.0f,
         1.0f,  1.0f,  1.0f,    1.0f, 0.0f, 0.5f, 1.0f,    1.0f, 1.0f,
         1.0f,  1.0f, -1.0f,    1.0f, 0.0f, 0.5f, 1.0f,    0.0f, 1.0f
    };
    gfx_buffer vbuf = gfx_make_buffer(&(gfx_buffer_desc){
        .size = sizeof(vertices),
        .content = vertices,
    });

    uint16_t indices[] = {
         0,  1,  2,
         0,  2,  3,
         6,  5,  4,
         7,  6,  4,
         8,  9, 10,
         8, 10, 11,
        14, 13, 12,
        15, 14, 12,
        16, 17, 18,
        16, 18, 19,
        22, 21, 20,
        23, 22, 20
    };
    gfx_buffer ibuf = gfx_make_buffer(&(gfx_buffer_desc){
        .type = GFX_BUFFERTYPE_INDEXBUFFER,
        .size = sizeof(indices),
        .content = indices,
    });

    *rs = (renderer_scene){
        .buffers = {
            vbuf,
            ibuf
        },
        .primitives = {
            [0] = {
                .vertex_buffer = 0,
                .index_buffer = 1,
                .base_element = 0,
                .num_elements = sizeof(indices) / sizeof(indices[0])
            }
        },
        .meshes = {
            [0] = {
                .first_primitive = 0,
                .num_primitives = 1
            }
        },
        .nodes = {
            [0] = {
                .mesh = 0,
                .transform = mat4_id()
            }
        },
        .num_buffers    = 2,
        .num_primitives = 1,
        .num_meshes     = 1,
        .num_nodes      = 1,
    };

    return r;
}

void* resmngr_model_lookup(resmngr rm, rid r)
{
    return slot_map_lookup(&rm->scene_map, r);
}

void resmngr_model_delete(resmngr rm, rid r)
{
    struct slot_map* sm = &rm->scene_map;
    renderer_scene* rs = slot_map_lookup(sm, r);

    for (size_t i = 0; i < rs->num_buffers; ++i) {
        gfx_buffer buf = rs->buffers[i];
        gfx_destroy_buffer(buf);
    }

    for (size_t i = 0; i< rs->num_images; ++i) {
        gfx_image img = rs->images[i];
        gfx_destroy_image(img);
    }

    slot_map_remove(sm, r);
}