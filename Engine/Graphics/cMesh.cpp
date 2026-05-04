#include "cMesh.h"
#include <Engine/Asserts/Asserts.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Platform/Platform.h>

eae6320::cResult eae6320::Graphics::cMesh::Load(cMesh*& o_mesh, const std::string& i_vertexMeshPath)
{
    uint16_t* indexData = nullptr;
    eae6320::Graphics::VertexFormats::sVertex_mesh* vertexData = nullptr;
    uint16_t vertexCount = 0;
    uint16_t indexCount = 0;

    Platform::sDataFromFile dataFromFile;
    auto result = Results::Success;

    {
        std::string errorMessage;
        if (!(result = Platform::LoadBinaryFile(i_vertexMeshPath.c_str(), dataFromFile, &errorMessage)))
        {
            EAE6320_ASSERTF(false, "Couldn't load the mesh data from the binary file");
            Logging::OutputError("Failed to read from binary file with error %s", errorMessage);
            return result;
        }
    }

    auto currentOffset = reinterpret_cast<uintptr_t>(dataFromFile.data);
    const auto finalOffset = currentOffset + dataFromFile.size;

    {
        memcpy(&vertexCount, reinterpret_cast<void*>(currentOffset), sizeof(vertexCount));
    }

    {
        currentOffset += sizeof(vertexCount);
        memcpy(&indexCount, reinterpret_cast<void*>(currentOffset), sizeof(indexCount));
    }

    {
        currentOffset += sizeof(indexCount);
        vertexData = reinterpret_cast<eae6320::Graphics::VertexFormats::sVertex_mesh*>(currentOffset);
    }

    {
        currentOffset += sizeof(eae6320::Graphics::VertexFormats::sVertex_mesh) * vertexCount;
        indexData = reinterpret_cast<uint16_t*>(currentOffset);
    }
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
        newMesh = new cMesh(static_cast<unsigned int>(indexCount));
        if (!newMesh)
        {
            result = Results::OutOfMemory;
            EAE6320_ASSERTF(false, "Couldn't allocate memory for the mesh");
            Logging::OutputError("Failed to allocate memory for the mesh");
            return result;
        }
    }

    //Initialize the geometry
    if (!(result = newMesh->InitializeGeometry(vertexData, static_cast<unsigned int>(vertexCount), indexData)))
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