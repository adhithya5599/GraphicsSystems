// Includes
//=========

#include "../cVertexFormat.h"

#include "Includes.h"
#include "../sContext.h"
#include "../VertexFormats.h"

#include <Engine/Asserts/Asserts.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Platform/Platform.h>

// Interface
//==========

// Render
//-------

void eae6320::Graphics::cVertexFormat::Bind()
{
	auto* const direct3dImmediateContext = sContext::g_context.direct3dImmediateContext;
	EAE6320_ASSERT( direct3dImmediateContext );

	// Set the layout (which defines how to interpret a single vertex)
	EAE6320_ASSERT( m_vertexInputLayout );
	direct3dImmediateContext->IASetInputLayout( m_vertexInputLayout );
}

// Implementation
//===============

// Initialize / Clean Up
//----------------------

eae6320::cResult eae6320::Graphics::cVertexFormat::Initialize( const eVertexType i_type, const char* const i_vertexShaderPath )
{
	auto result = Results::Success;

	auto* const direct3dDevice = sContext::g_context.direct3dDevice;
	EAE6320_ASSERT( direct3dDevice );

	// Load the compiled binary vertex shader for the input layout
	Platform::sDataFromFile vertexShaderDataFromFile;
	std::string errorMessage;
	if ( result = Platform::LoadBinaryFile( i_vertexShaderPath, vertexShaderDataFromFile, &errorMessage ) )
	{
		// Create the vertex layout

		switch ( i_type )
		{
		case eVertexType::Mesh:
			{
				constexpr unsigned int vertexElementCount = 4;
				D3D11_INPUT_ELEMENT_DESC layoutDescription[vertexElementCount] = {};
				{
					// Slot 0

					// POSITION
					// 3 floats == 12 bytes
					// Offset = 0
					{
						auto& positionElement = layoutDescription[0];

						positionElement.SemanticName = "POSITION";
						positionElement.SemanticIndex = 0;	// (Semantics without modifying indices at the end can always use zero)
						positionElement.Format = DXGI_FORMAT_R32G32B32_FLOAT;
						positionElement.InputSlot = 0;
						positionElement.AlignedByteOffset = offsetof( VertexFormats::sVertex_mesh, x );
						positionElement.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
						positionElement.InstanceDataStepRate = 0;	// (Must be zero for per-vertex data)
					}
					
					// Slot 1
					//COLOR
					// Offset = 12
					{
						auto& colorElement = layoutDescription[1];
						colorElement.SemanticName = "COLOR";
						colorElement.SemanticIndex = 0;
						colorElement.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
						colorElement.InputSlot = 0;
						colorElement.AlignedByteOffset = offsetof( VertexFormats::sVertex_mesh, r );
						colorElement.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
						colorElement.InstanceDataStepRate = 0;
					}

					//Slot 2
					//Texture
					// Offset = 16
					{
						auto& texCoordElement = layoutDescription[2];
						texCoordElement.SemanticName = "TEXCOORD";
						texCoordElement.SemanticIndex = 0;
						texCoordElement.Format = DXGI_FORMAT_R32G32_FLOAT;
						texCoordElement.InputSlot = 0;
						texCoordElement.AlignedByteOffset = offsetof( VertexFormats::sVertex_mesh, u );
						texCoordElement.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
						texCoordElement.InstanceDataStepRate = 0;
					}

					// NORMAL
					// 3 floats == 12 bytes
					// Offset = 24
					{
						auto& normalElement = layoutDescription[3];
						normalElement.SemanticName = "NORMAL";
						normalElement.SemanticIndex = 0;
						normalElement.Format = DXGI_FORMAT_R32G32B32_FLOAT;
						normalElement.InputSlot = 0;
						normalElement.AlignedByteOffset = offsetof( VertexFormats::sVertex_mesh, nx );
						normalElement.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
						normalElement.InstanceDataStepRate = 0;
					}

				}

				const auto d3dResult = direct3dDevice->CreateInputLayout( layoutDescription, vertexElementCount,
					vertexShaderDataFromFile.data, vertexShaderDataFromFile.size, &m_vertexInputLayout );
				if ( FAILED( d3dResult ) )
				{
					result = Results::Failure;
					EAE6320_ASSERTF( false, "Mesh vertex input layout creation failed (HRESULT %#010x)", d3dResult );
					Logging::OutputError( "Direct3D failed to create the mesh vertex input layout (HRESULT %#010x)", d3dResult );
				}
			}
			break;
		default:
			result = Results::Failure;
			EAE6320_ASSERTF( false, "Invalid vertex type %u", i_type );
			Logging::OutputError( "Invalid vertex type %u", i_type );
		}
	}
	else
	{
		EAE6320_ASSERTF( false, errorMessage.c_str() );
		Logging::OutputError( "A vertex input layout shader couldn't be loaded: %s", errorMessage.c_str() );
		return result;
	}

	return result;
}

eae6320::cResult eae6320::Graphics::cVertexFormat::CleanUp()
{
	auto result = Results::Success;

	if ( m_vertexInputLayout )
	{
		m_vertexInputLayout->Release();
		m_vertexInputLayout = nullptr;
	}

	return result;
}
