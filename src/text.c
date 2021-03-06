#include "text.h"
#include <stdlib.h>
#include <string.h>
#include <linmath.h>
#include <math.h>
#include <gfx.h>
#include "texture_font.h"

#define GLSRC(src) "#version 330 core\n" #src

static const char* TEXT_VSH = GLSRC(
in vec2 vpos;
in vec2 vtco;

out vec2 tco;
uniform mat4 mvp;

void main()
{
    tco = vtco;
    gl_Position = mvp * vec4(vpos, 0.0, 1.0);
}
);

static const char* TEXT_FSH = GLSRC(
out vec4 fcolor;
in vec2 tco;

uniform vec4 col;
uniform float scl;
uniform bool ssp;
uniform bool dfd;
uniform sampler2D tex;

const float SQRT2_2 = 0.70710678118654757;

float contour(float d, float w)
{
    return smoothstep(0.5 - w, 0.5 + w, d);
}

void main()
{
    vec2 uv = tco;
    float dist = texture(tex, uv).r;

    // Keep outlines a constant width irrespective of scaling
    float fw = 0.0;
    if (dfd) {
        // GLSL's fwidth = abs(dFdx(dist)) + abs(dFdy(dist))
        fw = fwidth(dist);
        // Stefan Gustavson's fwidth
        //fw = SQRT2_2 * length(vec2(dFdx(dist), dFdy(dist)));
    } else {
        fw = (1.0 / scl) * SQRT2_2 / gl_FragCoord.w;
    }
    float alpha = contour(dist, fw);

    if (ssp) {
        // Supersample
        float dscale = 0.354; // half of 1/sqrt2
        vec2 duv = dscale * (dFdx(uv) + dFdy(uv));
        vec4 box = vec4(uv - duv, uv + duv);
        float asum = contour(texture(tex, box.xy).r, fw)
                   + contour(texture(tex, box.zw).r, fw)
                   + contour(texture(tex, box.xw).r, fw)
                   + contour(texture(tex, box.zy).r, fw);
        // Weighted average, with 4 extra points having 0.5 weight each,
        // so 1 + 0.5 * 4 = 3 is the divisor
        alpha = (alpha + 0.5 * asum) / 3.0;
    }

    fcolor = col * vec4(vec3(1.0), alpha);
});

typedef struct tvertex {
    vec2 pos;
    vec2 uv;
} tvertex;

typedef struct vs_params {
    mat4 mvp;
} vs_params;

typedef struct fs_params {
    vec4 col;
    float scl;
    int ssp;
    int dfd;
} fs_params;

typedef struct text_renderer {
    gfx_shader shd;
    gfx_pipeline pip;
    gfx_buffer vbuf;
    gfx_buffer ibuf;
}* text_renderer;

text_renderer text_renderer_create()
{
    gfx_shader shd = gfx_make_shader(&(gfx_shader_desc){
        .attrs = {
            [0].name = "vpos",
            [1].name = "vtco",
        },
        .vs.uniform_blocks[0] = {
            .size = sizeof(vs_params),
            .uniforms = {
                [0] = {
                    .name = "mvp",
                    .type = GFX_UNIFORMTYPE_MAT4
                }
            }
        },
        .fs.uniform_blocks[0] = {
            .size = sizeof(fs_params),
            .uniforms = {
                [0] = {
                    .name = "col",
                    .type = GFX_UNIFORMTYPE_FLOAT4
                },
                [1] = {
                    .name = "scl",
                    .type = GFX_UNIFORMTYPE_FLOAT
                },
                [2] = {
                    .name = "ssp",
                    .type = GFX_UNIFORMTYPE_FLOAT
                },
                [3] = {
                    .name = "dfd",
                    .type = GFX_UNIFORMTYPE_FLOAT
                },
            }
        },
        .fs.images = {
            [0] = {
                .name = "tex",
                .image_type = GFX_IMAGETYPE_2D
            }
        },
        .vs.source = TEXT_VSH,
        .fs.source = TEXT_FSH,
    });

    gfx_pipeline pip = gfx_make_pipeline(&(gfx_pipeline_desc){
        .layout = {
            .attrs = {
                [0] = { .format = GFX_VERTEXFORMAT_FLOAT2 }, /* position */
                [1] = { .format = GFX_VERTEXFORMAT_FLOAT2 }, /* normal   */
            }
        },
        .shader = shd,
        .index_type = GFX_INDEXTYPE_UINT16,
        .depth = {
            .compare = GFX_COMPAREFUNC_ALWAYS,
            .write_enabled = false,
        },
        .cull_mode = GFX_CULLMODE_BACK,
        .face_winding = GFX_FACEWINDING_CCW,
        .colors[0] = {
            .blend = {
                .enabled = true,
                .src_factor_rgb = GFX_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = GFX_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .src_factor_alpha = GFX_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_alpha = GFX_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
            }
        }
    });

    gfx_buffer vbuf = gfx_make_buffer(&(gfx_buffer_desc){
        .type = GFX_BUFFERTYPE_VERTEXBUFFER,
        .usage = GFX_USAGE_DYNAMIC,
        .size = 8192,
    });
    gfx_buffer ibuf = gfx_make_buffer(&(gfx_buffer_desc){
        .type = GFX_BUFFERTYPE_INDEXBUFFER,
        .usage = GFX_USAGE_DYNAMIC,
        .size = 8192,
    });

    text_renderer tr = calloc(1, sizeof(*tr));
    tr->shd = shd;
    tr->pip = pip;
    tr->vbuf = vbuf;
    tr->ibuf = ibuf;
    return tr;
}

void text_renderer_destroy(text_renderer tr)
{
    gfx_destroy_buffer(tr->ibuf);
    gfx_destroy_buffer(tr->vbuf);
    gfx_destroy_pipeline(tr->pip);
    gfx_destroy_shader(tr->shd);
    free(tr);
}

static vec4 add_text(tvertex* verts, uint16_t* indcs, texture_font* font, const char* text, vec2* pen)
{
    vec4 bbox = {{0, 0, 0, 0}};
    /* Iterate through each character */
    for (size_t i = 0; i < strlen(text); ++i) {
        /* Retrieve glyph from the given font corresponding to the current char */
        texture_glyph* glyph = texture_font_get_glyph(font, text + i);
        /* Skip non existing glyphs */
        if (!glyph)
            continue;
        /* Newlines are special case */
        if (text[i] == '\n') {
            pen->x = 0;
            pen->y -= font->height;
            continue;
        }
        /* Calculate glyph render triangles */
        float kerning = i > 0 ? texture_glyph_get_kerning(glyph, text + i - 1) : 0.0f;
        pen->x += kerning;
        int x0 = (int)(pen->x + glyph->offset_x);
        int y0 = (int)(pen->y + glyph->offset_y);
        int x1 = (int)(x0 + glyph->width);
        int y1 = (int)(y0 - glyph->height);
        float s0 = glyph->s0;
        float t0 = glyph->t0;
        float s1 = glyph->s1;
        float t1 = glyph->t1;
        tvertex vertices[4] = {{{{x0, y0}}, {{s0, t0}}},
                               {{{x0, y1}}, {{s0, t1}}},
                               {{{x1, y1}}, {{s1, t1}}},
                               {{{x1, y0}}, {{s1, t0}}}};
        /* Construct indice list */
        uint16_t indices[6] = {0, 1, 2, 0, 2, 3};
        for (int j = 0; j < 6; ++j)
            indices[j] += i * 4;
        /* Append vertices and indices to buffers */
        memcpy(verts + i * 4, vertices, sizeof(vertices));
        memcpy(indcs + i * 6, indices,  sizeof(indices));
        /* Advance cursor */
        pen->x += glyph->advance_x;
        /* Calculate bounding total bounding box (bbox.{z,w} == {width,height}) */
        if (x0 < bbox.x) bbox.x = x0;
        if (y0 > bbox.y) bbox.y = y0;
        if ((x1 - bbox.x) > bbox.z) bbox.z = x1 - bbox.x;
        if ((bbox.y - y1) > bbox.w) bbox.w = bbox.y - y1;
    }
    return bbox;
}

void text_draw(text_renderer tr, text_draw_desc* desc)
{
    /* Alias to font handle */
    texture_font* tf = desc->fnt->tfont;

    /* Normalization factor to 12pt/1em */
    const float emscale = 16.0 / tf->size;

    /* Allocate vertex and indice data buffers */
    size_t num_chars = strlen(desc->text);
    size_t num_verts = 4 * num_chars;
    size_t num_indcs = 6 * num_chars;
    tvertex* verts   = calloc(num_verts, sizeof(*verts));
    uint16_t* indcs  = calloc(num_indcs, sizeof(*indcs));

    /* Fill vertex and indice data buffers */
    vec2 pen = vec2_new(0.0, 0.0);
    vec4 bbox = add_text(verts, indcs, tf, desc->text, &pen);

    /* Normalize bbox */
    bbox.x *= emscale; bbox.y *= emscale;
    bbox.z *= emscale; bbox.w *= emscale;

    /* Transform vertices */
    for (size_t i = 0; i < num_verts; ++i) {
        tvertex* vertex = &verts[i];

        /* Normalize to 12pt */
        vertex->pos.x *= emscale;
        vertex->pos.y *= emscale;

        /* Horizontal alignment */
        vertex->pos.x -= bbox.x;
        switch (desc->halign) {
            case TEXT_HALIGN_LEFT:
                break;
            case TEXT_HALIGN_CENTER:
                vertex->pos.x -= bbox.z / 2.0f;
                break;
            case TEXT_HALIGN_RIGHT:
                vertex->pos.x -= bbox.z;
                break;
        }

        /* Vertical alignment */
        vertex->pos.y -= bbox.y;
        switch (desc->valign) {
            case TEXT_VALIGN_TOP:
                break;
            case TEXT_VALIGN_CENTER:
                vertex->pos.y += bbox.w / 2.0f;
                break;
            case TEXT_VALIGN_BOTTOM:
                vertex->pos.y += bbox.w;
                break;
        }

        /* Position to screen coordinates */
        vertex->pos.x += desc->scr_width / 2.0;
        vertex->pos.y += desc->scr_height / 2.0;
    }

    /* Upload to GPU */
    gfx_update_buffer(tr->vbuf, &(gfx_range){verts, num_verts * sizeof(*verts)});
    gfx_update_buffer(tr->ibuf, &(gfx_range){indcs, num_indcs * sizeof(*indcs)});
    free(verts); free(indcs);

    /* Prepare parameters */
    mat4 mdl = mat4_world(desc->pos, desc->scl, desc->rot);
    mat4 prj = mat4_orthographic(0, desc->scr_width, 0, desc->scr_height, -1, 1);
    mat4 mvp = mat4_mul_mat4(mdl, prj);
    float scl_factor = vec3_length(desc->scl);

    vs_params vparams = (vs_params){
        .mvp = mvp
    };
    fs_params fparams = (fs_params){
        .col = vec4_one(),
        .scl = scl_factor,
        .ssp = 0,
        .dfd = 0,
    };

    /* Render text */
    gfx_begin_default_pass(&(gfx_pass_action){
        .colors[0] = {
            .action = GFX_ACTION_DONTCARE
        }
    }, desc->scr_width, desc->scr_height);
    gfx_apply_pipeline(tr->pip);
    gfx_apply_bindings(&(gfx_bindings){
        .vertex_buffers[0] = tr->vbuf,
        .index_buffer      = tr->ibuf,
        .fs_images[0]      = desc->fnt->atlas_img,
    });
    gfx_apply_uniforms(GFX_SHADERSTAGE_VS, 0, &(gfx_range){&vparams, sizeof(vparams)});
    gfx_apply_uniforms(GFX_SHADERSTAGE_FS, 0, &(gfx_range){&fparams, sizeof(fparams)});
    gfx_draw(0, num_indcs, 1);
    gfx_end_pass();
    gfx_commit();
}
