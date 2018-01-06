/**
 * Based on Sylvain Beucler's tutorial from the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Author: onlyuser
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using the GLUT library for the base windowing setup */
#include <GL/glut.h>
/* GLM */
// #define GLM_MESSAGES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <shader_utils.h>

#include <Buffer.h>
#include <Camera.h>
#include <File3ds.h>
#include <FileMMD.h>
#include <FrameBuffer.h>
#include <Light.h>
#include <Material.h>
#include <Mesh.h>
#include <Modifiers.h>
#include <PrimitiveFactory.h>
#include <Program.h>
#include <Scene.h>
#include <Shader.h>
#include <ShaderContext.h>
#include <Texture.h>
#include <Util.h>
#include <VarAttribute.h>
#include <VarUniform.h>
#include <vector>
#include <iostream> // std::cout
#include <sstream> // std::stringstream
#include <iomanip> // std::setprecision
#include <unistd.h> // access
#include <getopt.h> // getopt_long

#define ACCEPT_AVG_ANGLE_DISTANCE    0.001
#define ACCEPT_END_EFFECTOR_DISTANCE 0.001
#define IK_ITERS                     1
#define IK_SEGMENT_COUNT             3

const char* DEFAULT_CAPTION = "";

int init_screen_width  = 800,
    init_screen_height = 600;
vt::Camera  *camera         = NULL;
vt::Mesh    *mesh_skybox    = NULL;
vt::Light   *light          = NULL,
            *light2         = NULL,
            *light3         = NULL;
vt::Texture *texture_skybox = NULL;

bool left_mouse_down  = false,
     right_mouse_down = false;
glm::vec2 prev_mouse_coord,
          mouse_drag;
glm::vec3 prev_euler,
          euler,
          orbit_speed = glm::vec3(0, -0.5, -0.5);
float prev_orbit_radius = 0,
      orbit_radius      = 8,
      dolly_speed       = 0.1,
      light_distance    = 4;
bool show_bbox        = false,
     show_fps         = false,
     show_help        = false,
     show_lights      = false,
     show_normals     = false,
     wireframe_mode   = false,
     show_guide_wires = false,
     show_paths       = true,
     show_axis        = false,
     show_axis_labels = false,
     do_animation     = true,
     left_key         = false,
     right_key        = false,
     up_key           = false,
     down_key         = false,
     page_up_key      = false,
     page_down_key    = false,
     user_input       = true;

float prev_zoom         = 0,
      zoom              = 1,
      ortho_dolly_speed = 0.1;

int angle_delta = 2;

std::vector<vt::Mesh*> meshes_imported;
vt::Mesh* dummy = NULL;
vt::MeshAnimContext mesh_anim_context;
int frame = 0;
double anim_time = 0;
double anim_step = 0.1;

void* animation_loop(void* args);
void next_frame();

#define NTHREADS 1
pthread_t threads[NTHREADS];
void* retvals[NTHREADS];
void init_threads();
void deinit_threads();

pthread_mutex_t update_animation_mutex;
void init_mutexes();
void deinit_mutexes();

void init_multithreading_resources();
void deinit_multithreading_resources();

bool need_update_buffers = false;
bool do_animation_loop = true;

void display_usage()
{
    std::cout << "mmd2obj [-p <pmd/pmx file>] [-vmd <vmd file>] [-f <frame>] [-t <animation time (sec)>]" << std::endl;
}

void show_turn_off_anim_msg()
{
    std::cout << "Info: Animation is currently ON. Please turn off animation first by pressing SPACE." << std::endl;
}

struct options_t
{
    std::string m_modelPath;
    std::string m_vmdPath;
    int         m_frame;
    double      m_animTime;
    bool        m_showHelp;

    options_t()
        : m_frame(-1),
          m_animTime(-1),
          m_showHelp(false)
    {}
};

bool extract_options_from_args(options_t* options, int argc, char** argv)
{
    if(!options) {
        return false;
    }
    int opt = 0;
    int longIndex = 0;
    static const char *optString = "p:v:f:t:h?";
    static const struct option longOpts[] = {{ "pmd-pmx", required_argument, NULL, 'p' },
                                             { "vmd",     required_argument, NULL, 'v' },
                                             { "frame",   required_argument, NULL, 'f' },
                                             { "time",    required_argument, NULL, 't' },
                                             { "help",    no_argument,       NULL, 'h' },
                                             { NULL,      no_argument,       NULL, 0 }};
    opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    while(opt != -1) {
        switch(opt) {
            case 'p': options->m_modelPath = optarg; break;
            case 'v': options->m_vmdPath   = optarg; break;
            case 'f': options->m_frame     = atoi(optarg); break;
            case 't': options->m_animTime  = atof(optarg); break;
            case 'h':
            case '?': options->m_showHelp = true; break;
            case 0: // reserved
            default:
                break;
        }
        opt = getopt_long(argc, argv, optString, longOpts, &longIndex);
    }
    return options->m_showHelp || (options->m_modelPath.length() && options->m_vmdPath.length() && options->m_frame != -1 && options->m_animTime != -1);
}

int init_resources(const options_t& options)
{
    vt::Scene* scene = vt::Scene::instance();

    mesh_skybox = vt::PrimitiveFactory::create_viewport_quad("grid");
    scene->set_skybox(mesh_skybox);

    vt::Material* ambient_material = new vt::Material("ambient",
                                                      "src/shaders/ambient.v.glsl",
                                                      "src/shaders/ambient.f.glsl");
    scene->add_material(ambient_material);
    scene->set_wireframe_material(ambient_material);

    vt::Material* skybox_material = new vt::Material("skybox",
                                                     "src/shaders/skybox.v.glsl",
                                                     "src/shaders/skybox.f.glsl",
                                                     true); // use_overlay
    scene->add_material(skybox_material);

    vt::Material* phong_material = new vt::Material("phong",
                                                    "src/shaders/phong.v.glsl",
                                                    "src/shaders/phong.f.glsl");
    scene->add_material(phong_material);

    vt::Material* texture_mapped_material = new vt::Material("texture_mapped",
                                                             "src/shaders/texture_mapped.v.glsl",
                                                             "src/shaders/texture_mapped.f.glsl");
    scene->add_material(texture_mapped_material);

    texture_skybox = new vt::Texture("skybox_texture",
                                     "data/SaintPetersSquare2/posx.png",
                                     "data/SaintPetersSquare2/negx.png",
                                     "data/SaintPetersSquare2/posy.png",
                                     "data/SaintPetersSquare2/negy.png",
                                     "data/SaintPetersSquare2/posz.png",
                                     "data/SaintPetersSquare2/negz.png");
    scene->add_texture(          texture_skybox);
    skybox_material->add_texture(texture_skybox);

    glm::vec3 origin = glm::vec3(0);
    camera = new vt::Camera("camera", origin + glm::vec3(0, 0, orbit_radius), origin);
    scene->set_camera(camera);

    scene->add_light(light  = new vt::Light("light1", origin + glm::vec3(light_distance, 0, 0), glm::vec3(1, 0, 0)));
    scene->add_light(light2 = new vt::Light("light2", origin + glm::vec3(0, light_distance, 0), glm::vec3(0, 1, 0)));
    scene->add_light(light3 = new vt::Light("light3", origin + glm::vec3(0, 0, light_distance), glm::vec3(0, 0, 1)));

    mesh_skybox->set_material(skybox_material);
    mesh_skybox->set_texture_index(mesh_skybox->get_material()->get_texture_index_by_name("skybox_texture"));

    dummy = vt::PrimitiveFactory::create_box("dummy");
    dummy->center_axis();
    dummy->set_origin(glm::vec3(0));
    scene->add_mesh(dummy);
#if 1
    std::map<vt::Mesh*, vt::MeshAttributes> mesh2attr_map;
    if(access(options.m_modelPath.c_str(), F_OK) != -1 &&
       access(options.m_vmdPath.c_str(),   F_OK) != -1)
    {
        std::vector<std::string> vmdPaths;
        vmdPaths.push_back(options.m_vmdPath);
        vt::FileMMD::load_mmd(options.m_modelPath,
                              vmdPaths,
                              options.m_frame,
                              options.m_animTime,
                              &meshes_imported,
                              &mesh2attr_map,
                              &mesh_anim_context);
    }
    for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
        //(*p)->center_axis();
        //(*p)->set_origin(glm::vec3(0, 0, 0));
        //(*p)->set_scale(glm::vec3(0.33, 0.33, 0.33));
        //(*p)->flatten();
        //(*p)->set_material(phong_material);
        //(*p)->set_ambient_color(glm::vec3(0));
        (*p)->link_parent(dummy);
        scene->add_mesh(*p);
    }
    glm::vec3 global_min, global_max;
    vt::FileMMD::set_anim_time(&meshes_imported,
                               &mesh_anim_context,
                               frame,
                               anim_time,
                               &global_min,
                               &global_max);
    dummy->set_origin(-(global_min + global_max) * 0.5f);
    for(std::map<vt::Mesh*, vt::MeshAttributes>::iterator r = mesh2attr_map.begin(); r != mesh2attr_map.end(); r++) {
        vt::Mesh* mesh          = (*r).first;
        vt::MeshAttributes attr = (*r).second;
        mesh->set_material(texture_mapped_material);
        //mesh->set_material(ambient_material);
        vt::Texture* texture = new vt::Texture(attr.m_texture_filename, attr.m_texture_filename, false);
        scene->add_texture(texture);
        texture_mapped_material->add_texture(texture);
        mesh->set_texture_index(mesh->get_material()->get_texture_index_by_name(attr.m_texture_filename));
        mesh->set_ambient_color(attr.m_ambient_color);
        //mesh->set_diffuse_color(attr.m_diffuse_color);
        //mesh->set_specular_color(attr.m_specular_color);
        //mesh->set_alpha(attr.m_alpha);
    }
#else
    const char* model_filename = "data/star_wars/TI_Low0.3ds";
    if(access(model_filename, F_OK) != -1) {
        vt::File3ds::load3ds(model_filename, -1, &meshes_imported);
    }
    for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
        (*p)->set_origin(glm::vec3(0));
        (*p)->set_scale(glm::vec3(0.33, 0.33, 0.33));
        (*p)->flatten();
        (*p)->set_material(phong_material);
        (*p)->set_ambient_color(glm::vec3(0));
        (*p)->link_parent(dummy);
        scene->add_mesh(*p);
    }
#endif

    init_multithreading_resources();
    return 1;
}

int deinit_resources()
{
    deinit_multithreading_resources();
    return 1;
}

void onIdle()
{
    // NOTE: can't send memory to GPU in worker thread, as OpenGL contexts are different for each thread!
    // https://stackoverflow.com/questions/1611102/glutpostredisplay-in-a-different-thread
    if(need_update_buffers) {
        for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
            (*p)->update_buffers();
        }
        need_update_buffers = false;
    }
    glutPostRedisplay();
}

void* animation_loop(void* args)
{
    while(do_animation_loop) {
        if(!do_animation) {
            continue;
        }
        next_frame();
    }
    return NULL;
}

void update_animation()
{
    pthread_mutex_lock(&update_animation_mutex);
    vt::FileMMD::set_anim_time(&meshes_imported,
                               &mesh_anim_context,
                               frame,
                               anim_time);
    need_update_buffers = true;
    std::cout << "anim_time: " << anim_time << std::endl;
    pthread_mutex_unlock(&update_animation_mutex);
}

void previous_frame()
{
    frame--;
    anim_time -= anim_step;
    update_animation();
}

void next_frame()
{
    frame++;
    anim_time += anim_step;
    update_animation();
}

void init_threads()
{
    for(int i = 0; i < NTHREADS; ++i) {
        if(pthread_create(&threads[i], NULL, animation_loop, NULL) != 0) {
            fprintf(stderr, "Error: Failed to create thread: %d\n", i);
            break;
        }
    }
}

void deinit_threads()
{
    for(int i = 0; i < NTHREADS; ++i) {
        if(pthread_join(threads[i], &retvals[i]) != 0) {
            fprintf(stderr, "Error: Failed to join thread: %d\n", i);
        }
    }
}

void init_mutexes()
{
    pthread_mutex_init(&update_animation_mutex, NULL);
}

void deinit_mutexes()
{
    pthread_mutex_destroy(&update_animation_mutex);
}

void init_multithreading_resources()
{
    init_mutexes();
    init_threads();
}

void deinit_multithreading_resources()
{
    deinit_threads();
    deinit_mutexes();
}

void onTick()
{
    static unsigned int prev_tick = 0;
    static unsigned int frames = 0;
    unsigned int tick = glutGet(GLUT_ELAPSED_TIME);
    unsigned int delta_time = tick - prev_tick;
    static float fps = 0;
    if(delta_time > 1000) {
        fps = 1000.0 * frames / delta_time;
        frames = 0;
        prev_tick = tick;
    }
    if(show_fps && delta_time > 100) {
        std::stringstream ss;
        ss << std::setprecision(2) << std::fixed << fps << " FPS, "
            << "Mouse: {" << mouse_drag.x << ", " << mouse_drag.y << "}, "
            << "Yaw=" << EULER_YAW(euler) << ", Pitch=" << EULER_PITCH(euler) << ", Radius=" << orbit_radius << ", "
            << "Zoom=" << zoom;
        //ss << "Width=" << camera->get_width() << ", Width=" << camera->get_height();
        glutSetWindowTitle(ss.str().c_str());
    }
    frames++;
    //if(!do_animation) {
    //    return;
    //}
    if(left_key) {
        dummy->rotate(angle_delta, dummy->get_abs_heading());
        user_input = true;
    }
    if(right_key) {
        dummy->rotate(-angle_delta, dummy->get_abs_heading());
        user_input = true;
    }
    if(up_key) {
        dummy->rotate(-angle_delta, dummy->get_abs_left_direction());
        user_input = true;
    }
    if(down_key) {
        dummy->rotate(angle_delta, dummy->get_abs_left_direction());
        user_input = true;
    }
    if(page_up_key) {
        dummy->rotate(angle_delta, dummy->get_abs_up_direction());
        user_input = true;
    }
    if(page_down_key) {
        dummy->rotate(-angle_delta, dummy->get_abs_up_direction());
        user_input = true;
    }
    if(user_input) {
        std::stringstream ss;
        ss << "Roll=" << EULER_ROLL(dummy->get_euler()) << ", Pitch=" << EULER_PITCH(dummy->get_euler()) << ", Yaw=" << EULER_YAW(dummy->get_euler());
        std::cout << "\r" << std::setw(80) << std::left << ss.str() << std::flush;
        user_input = false;
    }
    static int angle = 0;
    angle = (angle + angle_delta) % 360;
}

char* get_help_string()
{
    return const_cast<char*>("HUD text");
}

void onDisplay()
{
    if(do_animation) {
        onTick();
    }
    vt::Scene* scene = vt::Scene::instance();
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(wireframe_mode) {
        scene->render(true, false, false, vt::Scene::use_material_type_t::USE_WIREFRAME_MATERIAL);
    } else {
        scene->render();
    }
    if(show_guide_wires || show_paths || show_axis || show_axis_labels || show_bbox || show_normals || show_help) {
        scene->render_lines_and_text(show_guide_wires, show_paths, show_axis, show_axis_labels, show_bbox, show_normals, show_help, get_help_string());
    }
    if(show_lights) {
        scene->render_lights();
    }
    glutSwapBuffers();
}

void onKeyboard(unsigned char key, int x, int y)
{
    switch(key) {
        case 'b': // bbox
            show_bbox = !show_bbox;
            break;
        case 'f': // frame rate
            show_fps = !show_fps;
            if(!show_fps) {
                glutSetWindowTitle(DEFAULT_CAPTION);
            }
            break;
        case 'g': // guide wires
            show_guide_wires = !show_guide_wires;
            break;
        case 'h': // help
            show_help = !show_help;
            break;
        case 'l': // lights
            show_lights = !show_lights;
            break;
        case 'n': // normals
            show_normals = !show_normals;
            break;
        case 'p': // projection
            if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_PERSPECTIVE) {
                camera->set_projection_mode(vt::Camera::PROJECTION_MODE_ORTHO);
            } else if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_ORTHO) {
                camera->set_projection_mode(vt::Camera::PROJECTION_MODE_PERSPECTIVE);
            }
            break;
        case 's': // paths
            show_paths = !show_paths;
            break;
        case '[': // previous frame
            if(do_animation) {
                show_turn_off_anim_msg();
            } else {
                previous_frame();
            }
            break;
        case ']': // next frame
            if(do_animation) {
                show_turn_off_anim_msg();
            } else {
                next_frame();
            }
            break;
        case 'w': // wireframe
            wireframe_mode = !wireframe_mode;
            if(wireframe_mode) {
                glPolygonMode(GL_FRONT, GL_LINE);
                dummy->set_ambient_color(glm::vec3(1, 0, 0));
                for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
                    (*p)->set_ambient_color(glm::vec3(1));
                }
            } else {
                dummy->set_ambient_color(glm::vec3(0));
                glPolygonMode(GL_FRONT, GL_FILL);
                for(std::vector<vt::Mesh*>::iterator p = meshes_imported.begin(); p != meshes_imported.end(); p++) {
                    (*p)->set_ambient_color(glm::vec3(0));
                }
            }
            break;
        case 'x': // axis
            show_axis = !show_axis;
            break;
        case 'z': // axis labels
            show_axis_labels = !show_axis_labels;
            break;
        case 32: // space
            do_animation = !do_animation;
            break;
        case 27: // escape
            exit(0);
            break;
    }
}

void onSpecial(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_F1:
            light->set_enabled(!light->is_enabled());
            break;
        case GLUT_KEY_F2:
            light2->set_enabled(!light2->is_enabled());
            break;
        case GLUT_KEY_F3:
            light3->set_enabled(!light3->is_enabled());
            break;
        case GLUT_KEY_HOME:
            dummy->set_euler(glm::vec3(0));
            dummy->get_transform(true); // TODO: why is this necessary?
            user_input = true;
            break;
        case GLUT_KEY_LEFT:
            left_key = true;
            break;
        case GLUT_KEY_RIGHT:
            right_key = true;
            break;
        case GLUT_KEY_UP:
            up_key = true;
            break;
        case GLUT_KEY_DOWN:
            down_key = true;
            break;
        case GLUT_KEY_PAGE_UP:
            page_up_key = true;
            break;
        case GLUT_KEY_PAGE_DOWN:
            page_down_key = true;
            break;
    }
}

void onSpecialUp(int key, int x, int y)
{
    switch(key) {
        case GLUT_KEY_LEFT:
            left_key = false;
            break;
        case GLUT_KEY_RIGHT:
            right_key = false;
            break;
        case GLUT_KEY_UP:
            up_key = false;
            break;
        case GLUT_KEY_DOWN:
            down_key = false;
            break;
        case GLUT_KEY_PAGE_UP:
            page_up_key = false;
            break;
        case GLUT_KEY_PAGE_DOWN:
            page_down_key = false;
            break;
    }
}

void onMouse(int button, int state, int x, int y)
{
    if(state == GLUT_DOWN) {
        prev_mouse_coord.x = x;
        prev_mouse_coord.y = y;
        if(button == GLUT_LEFT_BUTTON) {
            left_mouse_down = true;
            prev_euler = euler;
        }
        if(button == GLUT_RIGHT_BUTTON) {
            right_mouse_down = true;
            if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_PERSPECTIVE) {
                prev_orbit_radius = orbit_radius;
            } else if (camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_ORTHO) {
                prev_zoom = zoom;
            }
        }
    }
    else {
        left_mouse_down = right_mouse_down = false;
    }
}

void onMotion(int x, int y)
{
    if(left_mouse_down || right_mouse_down) {
        mouse_drag = glm::vec2(x, y) - prev_mouse_coord;
    }
    if(left_mouse_down) {
        euler = prev_euler + glm::vec3(0, mouse_drag.y * EULER_PITCH(orbit_speed), mouse_drag.x * EULER_YAW(orbit_speed));
        camera->orbit(euler, orbit_radius);
    }
    if(right_mouse_down) {
        if(camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_PERSPECTIVE) {
            orbit_radius = prev_orbit_radius + mouse_drag.y * dolly_speed;
            camera->orbit(euler, orbit_radius);
        } else if (camera->get_projection_mode() == vt::Camera::PROJECTION_MODE_ORTHO) {
            zoom = prev_zoom + mouse_drag.y * ortho_dolly_speed;
            camera->set_zoom(&zoom);
        }
    }
}

void onReshape(int width, int height)
{
    camera->resize(0, 0, width, height);
    glViewport(0, 0, width, height);
}

int main(int argc, char* argv[])
{
    options_t options;
    if(!extract_options_from_args(&options, argc, argv))
    {
        display_usage();
        return EXIT_FAILURE;
    }

    DEFAULT_CAPTION = argv[0];

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_ALPHA | GLUT_DOUBLE | GLUT_DEPTH /*| GLUT_STENCIL*/);
    glutInitWindowSize(init_screen_width, init_screen_height);
    glutCreateWindow(DEFAULT_CAPTION);

    GLenum glew_status = glewInit();
    if(glew_status != GLEW_OK) {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
        return 1;
    }

    if(!GLEW_VERSION_2_0) {
        fprintf(stderr, "Error: your graphic card does not support OpenGL 2.0\n");
        return 1;
    }

    const char* s = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("GLSL version %s\n", s);

    if(init_resources(options)) {
        glutDisplayFunc(onDisplay);
        glutKeyboardFunc(onKeyboard);
        glutSpecialFunc(onSpecial);
        glutSpecialUpFunc(onSpecialUp);
        glutMouseFunc(onMouse);
        glutMotionFunc(onMotion);
        glutReshapeFunc(onReshape);
        glutIdleFunc(onIdle);
        //glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glutMainLoop();
        do_animation_loop = false;
        deinit_resources();
    }

    return 0;
}
