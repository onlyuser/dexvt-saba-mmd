#ifndef H_FILE_MMD
#define H_FILE_MMD

#include <Saba/Model/MMD/MMDModel.h>
#include <Saba/Model/MMD/VMDAnimation.h>
#include <map>
#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace vt {

class Mesh;
class MeshBase;
class Texture;

struct MeshAttributes
{
    std::string m_texture_filename;
    glm::vec3   m_ambient_color;
    glm::vec3   m_diffuse_color;
    glm::vec3   m_specular_color;
    float       m_alpha;
};

struct MeshAnimContext
{
    std::shared_ptr<saba::MMDModel>     m_mmdModel;
    std::unique_ptr<saba::VMDAnimation> m_vmdAnim;
    std::vector<size_t>                 m_indices;
    std::vector<int>                    m_subMesh_min_vert_idx;
};

class FileMMD
{
public:
    static bool load_mmd(const std::string&               modelPath,
                         const std::vector<std::string>&  vmdPaths,
                         int                              frame,
                         double                           animTime,
                         std::vector<Mesh*>*              meshes,
                         std::map<Mesh*, MeshAttributes>* mesh2attr_map,
                         MeshAnimContext*                 meshAnimContext);
    static bool set_anim_time(std::vector<Mesh*>* meshes,
                              MeshAnimContext*    meshAnimContext,
                              int                 frame,
                              double              animTime,
                              glm::vec3*          global_min = NULL,
                              glm::vec3*          global_max = NULL);
    static bool load_mmd_impl(const std::string&                   modelPath,
                              const std::vector<std::string>&      vmdPaths,
                              int                                  frame,
                              double                               animTime,
                              std::vector<MeshBase*>*              meshes,
                              std::map<MeshBase*, MeshAttributes>* mesh2attr_map,
                              MeshAnimContext*                     meshAnimContext);
    static bool set_anim_time_impl(std::vector<MeshBase*>* meshes,
                                   MeshAnimContext*        meshAnimContext,
                                   int                     frame,
                                   double                  animTime,
                                   glm::vec3*              global_min = NULL,
                                   glm::vec3*              global_max = NULL);
};

}

#endif
