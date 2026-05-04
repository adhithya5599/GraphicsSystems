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

		uint16_t indexData[] = {
			0, 1, 2,
			1, 0, 3
		};
		const auto indexBufferSize = sizeof(indexData[0]) * m_indexCount;
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

eae6320::cResult eae6320::Graphics::cMesh::InitializeViews(const sInitializationParameters& i_initializationParameters)
{
	auto result = eae6320::Results::Success;

	ID3D11Texture2D* backBuffer = nullptr;
	ID3D11Texture2D* depthBuffer = nullptr;
	eae6320::cScopeGuard scopeGuard([&backBuffer, &depthBuffer]
		{
			// Regardless of success or failure the two texture resources should be released
			// (if the function is successful the views will hold internal references to the resources)
			if ( backBuffer )
			{
				backBuffer->Release();
				backBuffer = nullptr;
			}
			if (depthBuffer)
			{
				depthBuffer->Release();
				depthBuffer = nullptr;
			}
		} );

	auto& g_context = eae6320::Graphics::sContext::g_context;
	auto* const direct3dDevice = g_context.direct3dDevice;
	EAE6320_ASSERT( direct3dDevice );
	auto* const direct3dImmediateContext = g_context.direct3dImmediateContext;
	EAE6320_ASSERT(direct3dImmediateContext);

	// Create a "render target view" of the back buffer
	// (the back buffer was already created by the call to D3D11CreateDeviceAndSwapChain(),
	// but a "view" of it is required to use as a "render target",
	// meaning a texture that the GPU can render to)
	{
		// Get the back buffer from the swap chain
		{
			constexpr unsigned int bufferIndex = 0;	// This must be 0 since the swap chain is discarded
			const auto d3dResult = g_context.swapChain->GetBuffer(bufferIndex, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
			if ( FAILED( d3dResult ) )
			{
				result = eae6320::Results::Failure;
				EAE6320_ASSERTF(false, "Couldn't get the back buffer from the swap chain (HRESULT %#010x)", d3dResult);
				eae6320::Logging::OutputError("Direct3D failed to get the back buffer from the swap chain (HRESULT %#010x)", d3dResult);
				return result;
			}
		}
		// Create the view
		{
			constexpr D3D11_RENDER_TARGET_VIEW_DESC* const accessAllSubResources = nullptr;
			const auto d3dResult = direct3dDevice->CreateRenderTargetView(backBuffer, accessAllSubResources, &sContext::g_context.renderTargetView);
			if ( FAILED ( d3dResult ) )
			{
				result = eae6320::Results::Failure;
				EAE6320_ASSERTF(false, "Couldn't create render target view (HRESULT %#010x)", d3dResult);
				eae6320::Logging::OutputError("Direct3D failed to create the render target view (HRESULT %#010x)", d3dResult);
				return result;
			}
		}
	}
	// Create a depth/stencil buffer and a view of it
	{
		// Unlike the back buffer no depth/stencil buffer exists until and unless it is explicitly created
		{
			const auto textureDescription = [i_initializationParameters]
			{
					auto textureDescription = []() constexpr
					{
							D3D11_TEXTURE2D_DESC textureDescription{};
							textureDescription.MipLevels = 1;	// A depth buffer has no MIP maps
							textureDescription.ArraySize = 1;
							textureDescription.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	// 24 bits for depth and 8 bits for stencil
							{
								DXGI_SAMPLE_DESC& sampleDescription = textureDescription.SampleDesc;
								sampleDescription.Count = 1;	// No multisampling
								sampleDescription.Quality = 0;	// Doesn't matter when Count is 1
							}
							textureDescription.Usage = D3D11_USAGE_DEFAULT;	// Allows the GPU to write to it
							textureDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;
							textureDescription.CPUAccessFlags = 0;	// The CPU doesn't need access
							textureDescription.MiscFlags = 0;
							return textureDescription;
					}();

					textureDescription.Width = i_initializationParameters.resolutionWidth;
					textureDescription.Height = i_initializationParameters.resolutionHeight;

					return textureDescription;
			}();
			// The GPU renders to the depth/stencil buffer and so there is no initial data
			// (like there would be with a traditional texture loaded from disk)
			constexpr D3D11_SUBRESOURCE_DATA* const noInitialData = nullptr;
			const auto d3dResult = direct3dDevice->CreateTexture2D(&textureDescription, noInitialData, &depthBuffer);
			if ( FAILED( d3dResult ) )
			{
				result = eae6320::Results::Failure;
				EAE6320_ASSERTF(false, "Couldn't create depth buffer (HRESULT %#010x)", d3dResult);
				eae6320::Logging::OutputError("Direct3D failed to create the depth buffer resource (HRESULT %#010x)", d3dResult);
				return result;
			}
		}
		// Create the view
		{
			constexpr D3D11_DEPTH_STENCIL_VIEW_DESC* const noSubResources = nullptr;
			const auto d3dResult = direct3dDevice->CreateDepthStencilView(depthBuffer, noSubResources, &sContext::g_context.depthStencilView);
			if (FAILED( d3dResult ) )
			{
				result = eae6320::Results::Failure;
				EAE6320_ASSERTF(false, "Couldn't create depth stencil view (HRESULT %#010x)", d3dResult);
				eae6320::Logging::OutputError("Direct3D failed to create the depth stencil view (HRESULT %#010x)", d3dResult);
				return result;
			}
		}
	}

	// Bind the views
	{
		constexpr unsigned int renderTargetCount = 1;
		direct3dImmediateContext->OMSetRenderTargets(renderTargetCount, &sContext::g_context.renderTargetView, sContext::g_context.depthStencilView);
	}
	// Specify that the entire render target should be visible
	{
		const auto viewPort = [i_initializationParameters]
			{
				auto viewPort = []() constexpr
					{
						D3D11_VIEWPORT viewPort{};
						viewPort.TopLeftX = viewPort.TopLeftY = 0.0f;
						viewPort.MinDepth = 0.0f;
						viewPort.MaxDepth = 1.0f;

						return viewPort;
					}();
				viewPort.Width = static_cast<float>(i_initializationParameters.resolutionWidth);
				viewPort.Height = static_cast<float>(i_initializationParameters.resolutionHeight);

				return viewPort;
			}();
		constexpr unsigned int viewPortCount = 1;
		direct3dImmediateContext->RSSetViewports(viewPortCount, &viewPort);
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