#include "../cMesh.h"

#include "Includes.h"
#include "../sContext.h"
#include "../VertexFormats.h"

#include <Engine/Logging/Logging.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <Engine/Math/Functions.h>

eae6320::cResult eae6320::Graphics::cMesh::InitializeGeometry(eae6320::Graphics::VertexFormats::sVertex_mesh i_vertexData[], const unsigned int i_vertexCount, uint16_t i_indexData[])
{
	auto result = eae6320::Results::Success;
	auto* const direct3dDevice = eae6320::Graphics::sContext::g_context.direct3dDevice;
	EAE6320_ASSERT(direct3dDevice);

	// Vertex Format
	{
		if (!(result = eae6320::Graphics::cVertexFormat::Load(eae6320::Graphics::eVertexType::Mesh, m_vertexFormat,
			"data/Shaders/Vertex/vertexInputLayout_mesh.shader")))
		{
			EAE6320_ASSERTF(false, "Can't initialize geometry without vertex format");
			return result;
		}
	}

	//Vertex Buffer
	{
		const auto bufferSize = sizeof(i_vertexData[0]) * i_vertexCount;
		EAE6320_ASSERT(bufferSize <= std::numeric_limits<decltype(D3D11_BUFFER_DESC::ByteWidth)>::max());
		const auto bufferDescription = [bufferSize] 
		{
			D3D11_BUFFER_DESC bufferDescription{};

			bufferDescription.ByteWidth = static_cast<unsigned int>(bufferSize);
			bufferDescription.Usage = D3D11_USAGE_IMMUTABLE;	// In our class the buffer will never change after it's been created
			bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDescription.CPUAccessFlags = 0;	// No CPU access is necessary
			bufferDescription.MiscFlags = 0;
			bufferDescription.StructureByteStride = 0;	// Not used
			return bufferDescription;
		}();

		const auto indexBufferSize = sizeof(i_indexData[0]) * m_indexCount;
		EAE6320_ASSERT(indexBufferSize <= std::numeric_limits<decltype(D3D11_BUFFER_DESC::ByteWidth)>::max());
		const auto indexBufferDescription = [indexBufferSize]
			{
				D3D11_BUFFER_DESC indexBufferDescription{};

				indexBufferDescription.ByteWidth = static_cast<unsigned int>(indexBufferSize);
				indexBufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
				indexBufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
				indexBufferDescription.CPUAccessFlags = 0;
				indexBufferDescription.MiscFlags = 0;
				indexBufferDescription.StructureByteStride = 0;

				return indexBufferDescription;
			}();
		
		const auto vertexInitialData = [&i_vertexData]
			{
				D3D11_SUBRESOURCE_DATA vertexInitialData{};
				vertexInitialData.pSysMem = i_vertexData;
				// (The other data members are ignored for non-texture buffers)
				return vertexInitialData;
			}();
		
		const auto initialData = [&i_indexData]
		{
			D3D11_SUBRESOURCE_DATA initialData{};
			initialData.pSysMem = i_indexData;
			// (The other data members are ignored for non-texture buffers)
			return initialData;
		}();

		const auto result_create = direct3dDevice->CreateBuffer(&bufferDescription, &vertexInitialData, &m_vertexBuffer);
		if ( FAILED( result_create ) )
		{
			result = eae6320::Results::Failure;
			EAE6320_ASSERTF(false, "3D object vertex buffer creation failed (HRESULT %#010x)", result_create);
			eae6320::Logging::OutputError("Direct3D failed to create a 3D object vertex buffer (HRESULT %#010x)", result_create);
			return result;
		}
		const auto index_result_create = direct3dDevice->CreateBuffer(&indexBufferDescription, &initialData, &m_indexBuffer);
		if (FAILED( index_result_create ) )
		{
			result = eae6320::Results::Failure;
			EAE6320_ASSERTF(false, "3D object index buffer creation failed (HRESULT %#010x)", index_result_create);
			eae6320::Logging::OutputError("Direct3D failed to create a 3D object index buffer (HRESULT %#010x)", index_result_create);
			return result;
		}
	}

	return result;

}

void eae6320::Graphics::cMesh::Draw()
{
	auto* const direct3dImmediateContext = sContext::g_context.direct3dImmediateContext;
	EAE6320_ASSERT(direct3dImmediateContext);

	EAE6320_ASSERT(m_vertexBuffer != nullptr);
	constexpr unsigned int startingSlot = 0;
	constexpr unsigned int vertexBufferCount = 1;

	// The "stride" defines how large a single vertex is in the stream of data
	constexpr unsigned int bufferStride = sizeof(VertexFormats::sVertex_mesh);
	// It's possible to start streaming data in the middle of a vertex buffer
	constexpr unsigned int bufferOffset = 0;
	direct3dImmediateContext->IASetVertexBuffers(startingSlot, vertexBufferCount, &m_vertexBuffer, &bufferStride, &bufferOffset);

	//Bind the Index buffer
	{
		EAE6320_ASSERT( m_indexBuffer );
		constexpr DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;
		//The indices start at the beginning of the buffer
		constexpr unsigned int offset = 0;
		direct3dImmediateContext->IASetIndexBuffer(m_indexBuffer, indexFormat, offset);
	}
	
	// Specify what kind of data the vertex buffer holds
	{
		// Bind the vertex format (which defines how to interpret a single vertex)
		{
			EAE6320_ASSERT(m_vertexFormat != nullptr);
			m_vertexFormat->Bind();
		}
		// Set the topology (which defines how to interpret multiple vertices as a single "primitive";
		// the vertex buffer was defined as a triangle list
		// (meaning that every primitive is a triangle and will be defined by three vertices)
		direct3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// Render triangles from the currently-bound index buffer
	{
		// As of this comment only a single triangle is drawn
		// (you will have to update this code in future assignments!)
		// It's possible to start rendering primitives in the middle of the stream
		constexpr unsigned int indexOfFirstIndexToUse = 0;
		constexpr unsigned int offsetToAddEachIndex = 0;
		//constexpr unsigned int indexOfFirstVertexToRender = 0;
		//direct3dImmediateContext->Draw(vertexCountToRender, indexOfFirstVertexToRender);
		direct3dImmediateContext->DrawIndexed(static_cast<unsigned int>(m_indexCount), indexOfFirstIndexToUse, offsetToAddEachIndex);
	}
}

eae6320::cResult eae6320::Graphics::cMesh::CleanUp()
{
	auto result = Results::Success;
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}
	if (m_vertexFormat)
	{
		m_vertexFormat->DecrementReferenceCount();
		m_vertexFormat = nullptr;
	}
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = nullptr;
	}

	return result;
}