/*********************************************************************************************************************/
/*                                                  /===-_---~~~~~~~~~------____                                     */
/*                                                 |===-~___                _,-'                                     */
/*                  -==\\                         `//~\\   ~~~~`---.___.-~~                                          */
/*              ______-==|                         | |  \\           _-~`                                            */
/*        __--~~~  ,-/-==\\                        | |   `\        ,'                                                */
/*     _-~       /'    |  \\                      / /      \      /                                                  */
/*   .'        /       |   \\                   /' /        \   /'                                                   */
/*  /  ____  /         |    \`\.__/-~~ ~ \ _ _/'  /          \/'                                                     */
/* /-'~    ~~~~~---__  |     ~-/~         ( )   /'        _--~`                                                      */
/*                   \_|      /        _)   ;  ),   __--~~                                                           */
/*                     '~~--_/      _-~/-  / \   '-~ \                                                               */
/*                    {\__--_/}    / \\_>- )<__\      \                                                              */
/*                    /'   (_/  _-~  | |__>--<__|      |                                                             */
/*                   |0  0 _/) )-~     | |__>--<__|     |                                                            */
/*                   / /~ ,_/       / /__>---<__/      |                                                             */
/*                  o o _//        /-~_>---<__-~      /                                                              */
/*                  (^(~          /~_>---<__-      _-~                                                               */
/*                 ,/|           /__>--<__/     _-~                                                                  */
/*              ,//('(          |__>--<__|     /                  .----_                                             */
/*             ( ( '))          |__>--<__|    |                 /' _---_~\                                           */
/*          `-)) )) (           |__>--<__|    |               /'  /     ~\`\                                         */
/*         ,/,'//( (             \__>--<__\    \            /'  //        ||                                         */
/*       ,( ( ((, ))              ~-__>--<_~-_  ~--____---~' _/'/        /'                                          */
/*     `~/  )` ) ,/|                 ~-_~>--<_/-__       __-~ _/                                                     */
/*   ._-~//( )/ )) `                    ~~-'_/_/ /~~~~~~~__--~                                                       */
/*    ;'( ')/ ,)(                              ~~~~~~~~~~                                                            */
/*   ' ') '( (/                                                                                                      */
/*     '   '  `                                                                                                      */
/*********************************************************************************************************************/
#include <carbon.h>

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    /* Initialize internal library structures */
    carbon_setup();

    /* Create engine instance with given params */
    engine engine = engine_create(&(engine_params){});

    /* Fetch the world instance */
    ecs_world_t* world = engine_world(engine);

    /* Fetch the resource manager instance */
    resmngr rmgr = engine_resmngr(engine);

    /* Declare transform type */
    ecs_entity_t ecs_entity(transform) = ecs_lookup(world, "transform");
    ecs_type_t ecs_type(transform) = ecs_type_from_entity(world, ecs_entity(transform));

    /* Declare model type */
    ecs_entity_t ecs_entity(model) = ecs_lookup(world, "model");
    ecs_type_t ecs_type(model) = ecs_type_from_entity(world, ecs_entity(model));

    /* Create sample model resource */
    rid sample_model = resmngr_model_sample(rmgr);

    /* Create parent entity */
    ECS_ENTITY(world, p, transform, model);
    ecs_set(world, p, transform, {
        .pose = {
            .translation = (vec3){{0.0, 0.0, 0.0}},
            .scale = (vec3){{1.0, 1.0, 1.0}},
            .rotation = quat_id()
        },
        .dirty = 1
    });
    ecs_set(world, p, model, {
        .resource = sample_model
    });

    /* Create first child entity */
    ECS_ENTITY(world, c1, transform, model);
    ecs_set(world, c1, transform, {
        .pose = {
            .translation = (vec3){{-3.0, 0.0, 0.0}},
            .scale = (vec3){{0.5, 0.5, 0.5}},
            .rotation = quat_id()
        },
        .dirty = 1
    });
    ecs_set(world, c1, model, {
        .resource = sample_model
    });
    ecs_adopt(world, c1, p);

    /* Create second child entity */
    ECS_ENTITY(world, c2, transform, model);
    ecs_set(world, c2, transform, {
        .pose = {
            .translation = (vec3){{3.0, 0.0, 0.0}},
            .scale = (vec3){{0.3, 0.3, 0.3}},
            .rotation = quat_id()
        },
        .dirty = 1
    });
    ecs_set(world, c2, model, {
        .resource = sample_model
    });
    ecs_adopt(world, c2, p);

    /* Run */
    engine_run(engine);

    /* Cleanup allocated resources */
    engine_destroy(engine);

    return 0;
}
