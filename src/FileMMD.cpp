#include <FileMMD.h>
#include <MeshBase.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <Saba/Base/UnicodeUtil.h>
#include <Saba/Base/Path.h>
#include <Saba/Model/MMD/MMDModel.h>
#include <Saba/Model/MMD/PMDModel.h>
#include <Saba/Model/MMD/PMXModel.h>
#include <Saba/Model/MMD/VMDFile.h>
#include <Saba/Model/MMD/VMDAnimation.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>

namespace vt {

MeshBase* alloc_mesh_base(std::string name, size_t num_vertex, size_t num_tri);

class Mesh;

Mesh* cast_mesh(MeshBase* mesh);
MeshBase* cast_mesh_base(Mesh* mesh);

bool FileMMD::load_mmd(const std::string&               modelPath,
                       const std::vector<std::string>&  vmdPaths,
                       int                              frame,
                       double                           animTime,
                       std::vector<Mesh*>*              meshes,
                       std::map<Mesh*, MeshAttributes>* mesh2attr_map,
                       MeshAnimContext*                 meshAnimContext)
{
    if(!meshes || !mesh2attr_map || !meshAnimContext) {
        return false;
    }
    std::vector<MeshBase*> meshes_iface;
    std::map<MeshBase*, MeshAttributes> mesh_iface_2_attr_map;
    if(!load_mmd_impl(modelPath,
                      vmdPaths,
                      frame,
                      animTime,
                      &meshes_iface,
                      &mesh_iface_2_attr_map,
                      meshAnimContext))
    {
        return false;
    }
    for(std::vector<MeshBase*>::iterator p = meshes_iface.begin(); p != meshes_iface.end(); p++) {
        meshes->push_back(cast_mesh(*p));
    }
    for(std::map<MeshBase*, MeshAttributes>::iterator q = mesh_iface_2_attr_map.begin(); q != mesh_iface_2_attr_map.end(); q++) {
        (*mesh2attr_map)[cast_mesh((*q).first)] = (*q).second;
    }
    return true;
}

bool FileMMD::set_anim_time(std::vector<Mesh*>* meshes,
                            MeshAnimContext*    meshAnimContext,
                            int                 frame,
                            double              animTime,
                            glm::vec3*          global_min,
                            glm::vec3*          global_max)
{
    std::vector<MeshBase*> meshes_iface;
    for(std::vector<Mesh*>::iterator p = meshes->begin(); p != meshes->end(); p++) {
        meshes_iface.push_back(cast_mesh_base(*p));
    }
    set_anim_time_impl(&meshes_iface,
                       meshAnimContext,
                       frame,
                       animTime,
                       global_min,
                       global_max);
    return true;
}

// NOTE: based on MMD2Obj
bool FileMMD::load_mmd_impl(const std::string&                   modelPath,
                            const std::vector<std::string>&      vmdPaths,
                            int                                  frame,
                            double                               animTime,
                            std::vector<MeshBase*>*              meshes,
                            std::map<MeshBase*, MeshAttributes>* mesh2attr_map,
                            MeshAnimContext*                     meshAnimContext)
{
    if(!meshes || !mesh2attr_map || !meshAnimContext) {
        return false;
    }

    std::string name = saba::PathUtil::GetFilenameWithoutExt(modelPath);

    // Load model.
    std::shared_ptr<saba::MMDModel> mmdModel;
    std::string mmdDataPath = "";   // Set MMD data path(default toon texture path).
    std::string ext = saba::PathUtil::GetExt(modelPath);
    if (ext == "pmd")
    {
        auto pmdModel = std::make_unique<saba::PMDModel>();
        if (!pmdModel->Load(modelPath, mmdDataPath))
        {
            std::cout << "Load PMDModel Fail.\n";
            return false;
        }
        mmdModel = std::move(pmdModel);
    }
    else if (ext == "pmx")
    {
        auto pmxModel = std::make_unique<saba::PMXModel>();
        if (!pmxModel->Load(modelPath, mmdDataPath))
        {
            std::cout << "Load PMXModel Fail.\n";
            return false;
        }
        mmdModel = std::move(pmxModel);
    }
    else
    {
        std::cout << "Unsupported Model Ext : " << ext << "\n";
        return false;
    }

    // Load animation.
    auto vmdAnim = std::make_unique<saba::VMDAnimation>();
    if (!vmdAnim->Create(mmdModel))
    {
        std::cout << "Create VMDAnimation Fail.\n";
        return false;
    }
    for (const auto& vmdPath : vmdPaths)
    {
        saba::VMDFile vmdFile;
        if (!saba::ReadVMDFile(&vmdFile, vmdPath.c_str()))
        {
            std::cout << "Read VMD File Fail.\n";
            return false;
        }
        if (!vmdAnim->Add(vmdFile))
        {
            std::cout << "Add VMDAnimation Fail.\n";
            return false;
        }
    }

    // Update vertex.
    mmdModel->Update();

    // Write positions.
    const glm::vec2* uvs = mmdModel->GetUpdateUVs();

    // Copy vertex indices.
    std::vector<size_t> indices(mmdModel->GetIndexCount());
    if (mmdModel->GetIndexElementSize() == 1)
    {
        uint8_t* mmdIndices = (uint8_t*)mmdModel->GetIndices();
        for (size_t i = 0; i < indices.size(); i++)
        {
            indices[i] = mmdIndices[i];
        }
    }
    else if (mmdModel->GetIndexElementSize() == 2)
    {
        uint16_t* mmdIndices = (uint16_t*)mmdModel->GetIndices();
        for (size_t i = 0; i < indices.size(); i++)
        {
            indices[i] = mmdIndices[i];
        }
    }
    else if (mmdModel->GetIndexElementSize() == 4)
    {
        uint32_t* mmdIndices = (uint32_t*)mmdModel->GetIndices();
        for (size_t i = 0; i < indices.size(); i++)
        {
            indices[i] = mmdIndices[i];
        }
    }
    else
    {
        return false;
    }

    // Write faces.
    std::map<MeshBase*, int> mesh2texid_map;
    glm::vec3 global_min, global_max;
    size_t subMeshCount = mmdModel->GetSubMeshCount();
    const saba::MMDSubMesh* subMeshes = mmdModel->GetSubMeshes();
    for (size_t i = 0; i < subMeshCount; i++)
    {
        int subMesh_min_vert_idx = mmdModel->GetVertexCount() - 1;
        int subMesh_max_vert_idx = 0;
        for (int j = 0; j < subMeshes[i].m_vertexCount; j += 3)
        {
            auto vtxIdx = subMeshes[i].m_beginIndex + j;
            int idx1 = indices[vtxIdx + 0];
            int idx2 = indices[vtxIdx + 1];
            int idx3 = indices[vtxIdx + 2];
            subMesh_min_vert_idx = std::min(subMesh_min_vert_idx, idx1);
            subMesh_min_vert_idx = std::min(subMesh_min_vert_idx, idx2);
            subMesh_min_vert_idx = std::min(subMesh_min_vert_idx, idx3);
            subMesh_max_vert_idx = std::max(subMesh_max_vert_idx, idx1);
            subMesh_max_vert_idx = std::max(subMesh_max_vert_idx, idx2);
            subMesh_max_vert_idx = std::max(subMesh_max_vert_idx, idx3);
        }

        size_t subMesh_num_vertex = subMesh_max_vert_idx - subMesh_min_vert_idx + 1;
        size_t subMesh_num_tri    = subMeshes[i].m_vertexCount / 3;
        MeshBase* mesh            = alloc_mesh_base(name, subMesh_num_vertex, subMesh_num_tri);
        meshes->push_back(mesh);
        mesh2texid_map[mesh] = subMeshes[i].m_materialID;
        meshAnimContext->m_subMesh_min_vert_idx.push_back(subMesh_min_vert_idx);

        int tri_idx = 0;
        for (int k = 0; k < subMeshes[i].m_vertexCount; k += 3)
        {
            auto vtxIdx = subMeshes[i].m_beginIndex + k;
            int idx1 = indices[vtxIdx + 0];
            int idx2 = indices[vtxIdx + 1];
            int idx3 = indices[vtxIdx + 2];
            int subMesh_idx1 = indices[vtxIdx + 0] - subMesh_min_vert_idx;
            int subMesh_idx2 = indices[vtxIdx + 1] - subMesh_min_vert_idx;
            int subMesh_idx3 = indices[vtxIdx + 2] - subMesh_min_vert_idx;
            mesh->set_tex_coord(subMesh_idx1, uvs[idx1]);
            mesh->set_tex_coord(subMesh_idx2, uvs[idx2]);
            mesh->set_tex_coord(subMesh_idx3, uvs[idx3]);
            mesh->set_tri_indices(tri_idx++, glm::ivec3(subMesh_idx1, subMesh_idx2, subMesh_idx3));
        }
    }

    // Write materials.
    std::map<int, MeshAttributes> texid2attr_map;
    size_t materialCount = mmdModel->GetMaterialCount();
    const saba::MMDMaterial* _materials = mmdModel->GetMaterials();
    for (size_t i = 0; i < materialCount; i++)
    {
        const auto& m = _materials[i];
        texid2attr_map[i].m_texture_filename = m.m_texture;
        texid2attr_map[i].m_ambient_color    = m.m_ambient;
        texid2attr_map[i].m_diffuse_color    = m.m_diffuse;
        texid2attr_map[i].m_specular_color   = m.m_specular;
    }

    for(std::map<MeshBase*, int>::iterator p = mesh2texid_map.begin(); p != mesh2texid_map.end(); p++) {
        (*mesh2attr_map)[(*p).first] = texid2attr_map[(*p).second];
    }

    meshAnimContext->m_mmdModel = mmdModel;
    meshAnimContext->m_vmdAnim  = std::move(vmdAnim);
    meshAnimContext->m_indices  = std::move(indices);

    set_anim_time_impl(meshes, meshAnimContext, frame, animTime);

    return true;
}

// NOTE: based on MMD2Obj
bool FileMMD::set_anim_time_impl(std::vector<MeshBase*>* meshes,
                                 MeshAnimContext*        meshAnimContext,
                                 int                     frame,
                                 double                  animTime,
                                 glm::vec3*              global_min,
                                 glm::vec3*              global_max)
{
    std::shared_ptr<saba::MMDModel> mmdModel = meshAnimContext->m_mmdModel;
    auto vmdAnim                             = std::move(meshAnimContext->m_vmdAnim);
    std::vector<size_t> &indices             = meshAnimContext->m_indices;

    // Initialize pose.
    {
        // Sync physics animation.
        mmdModel->InitializeAnimation();
        vmdAnim->SyncPhysics((float)animTime * 30.0f);
    }

    // Update animation(animation loop).
    {
        // Update bone animation.
        mmdModel->BeginAnimation();
        vmdAnim->Evaluate((float)animTime * 30.0f);
#if 0
        // NOTE: deprecated
        //mmdModel->UpdateAnimation();
#endif
        mmdModel->UpdateAllAnimation(vmdAnim.get(), frame, (float)animTime * 30.0f);
        mmdModel->EndAnimation();

#if 0
        // NOTE: deprecated
        // Update physics animation.
        mmdModel->UpdatePhysics(1.0f / 60.0f);
#endif

        // Update vertex.
        mmdModel->Update();
    }

    // Write positions.
    const glm::vec3* positions = mmdModel->GetUpdatePositions();
    const glm::vec3* normals   = mmdModel->GetUpdateNormals();

    // Write faces.
    bool init_global_bbox = false;
    size_t subMeshCount = mmdModel->GetSubMeshCount();
    const saba::MMDSubMesh* subMeshes = mmdModel->GetSubMeshes();
    for (size_t i = 0; i < subMeshCount; i++)
    {
        MeshBase* mesh = (*meshes)[i];
        int subMesh_min_vert_idx = meshAnimContext->m_subMesh_min_vert_idx[i];

        for (int k = 0; k < subMeshes[i].m_vertexCount; k += 3)
        {
            auto vtxIdx = subMeshes[i].m_beginIndex + k;
            int idx1 = indices[vtxIdx + 0];
            int idx2 = indices[vtxIdx + 1];
            int idx3 = indices[vtxIdx + 2];
            int subMesh_idx1 = indices[vtxIdx + 0] - subMesh_min_vert_idx;
            int subMesh_idx2 = indices[vtxIdx + 1] - subMesh_min_vert_idx;
            int subMesh_idx3 = indices[vtxIdx + 2] - subMesh_min_vert_idx;
            mesh->set_vert_coord(subMesh_idx1, positions[idx1]);
            mesh->set_vert_coord(subMesh_idx2, positions[idx2]);
            mesh->set_vert_coord(subMesh_idx3, positions[idx3]);
            mesh->set_vert_normal(subMesh_idx1, normals[idx1]);
            mesh->set_vert_normal(subMesh_idx2, normals[idx2]);
            mesh->set_vert_normal(subMesh_idx3, normals[idx3]);
        }

        mesh->update_bbox();
        if(global_min && global_max) {
            glm::vec3 local_min, local_max;
            mesh->get_min_max(&local_min, &local_max);
            if(init_global_bbox) {
                *global_min = glm::min(*global_min, local_min);
                *global_max = glm::max(*global_max, local_max);
            } else {
                *global_min = local_min;
                *global_max = local_max;
                init_global_bbox = true;
            }
        }
    }

#if 0
    glm::vec3 global_center = (global_min + global_max) * 0.5f;
    for(std::vector<MeshBase*>::iterator p = meshes->begin(); p != meshes->end(); p++) {
        //cast_mesh(*p)->set_origin(glm::vec3(0, 0, 0));
        (*p)->set_axis(global_center);
        (*p)->update_normals_and_tangents();
        (*p)->update_bbox();
    }
#endif

    meshAnimContext->m_mmdModel = mmdModel;
    meshAnimContext->m_vmdAnim  = std::move(vmdAnim);

    return true;
}

}
