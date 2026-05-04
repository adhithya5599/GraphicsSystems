#include "cMesh.h"
#include <Engine/Asserts/Asserts.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <Engine/Logging/Logging.h>

eae6320::cResult eae6320::Graphics::cMesh::Load(cMesh*& o_mesh, const unsigned int i_indexCount, VertexFormats::sVertex_mesh i_vertexData[], const unsigned int i_vertexCount, uint16_t i_indexData[])
{
    auto result = Results::Success;
    cMesh* newMesh = nullptr;
    cScopeGuard scopeGuard([&o_mesh, &result, &newMesh]
        {
            if (result)
            {
                EAE6320_ASSERT(newMesh != nullptr);
                o_mesh = newMesh;
            }
            else
            {
                if (newMesh)
                {
                    newMesh->DecrementReferenceCount();
                    newMesh = nullptr;
                }
                o_mesh = nullptr;
            }
        });

    //Allocate a new Mesh
    {
        newMesh = new cMesh(i_indexCount);
        if (!newMesh)
        {
            result = Results::OutOfMemory;
            EAE6320_ASSERTF(false, "Couldn't allocate memory for the mesh");
            Logging::OutputError("Failed to allocate memory for the mesh");
            return result;
        }
    }

    //Initialize the geometry
    if (!(result = newMesh->InitializeGeometry(i_vertexData, i_vertexCount, i_indexData)))
    {
        EAE6320_ASSERTF(false, "Can't initialize Graphics without the geometry data");
        return result;
    }
    return result;
}

eae6320::Graphics::cMesh::~cMesh()
{
    EAE6320_ASSERT(m_referenceCount == 0);
    const auto result = CleanUp();
    EAE6320_ASSERT(result);
}