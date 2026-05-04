#include "../cEffect.h"

#include "Includes.h"

eae6320::cResult eae6320::Graphics::cEffect::Initialize_platformSpecific()
{
	//We don't have any platform dependent effect initializations
	auto result = Results::Success;
	return result;
}

void eae6320::Graphics::cEffect::Bind()
{
	auto* const direct3dImmediateContext = sContext::g_context.direct3dImmediateContext;
	EAE6320_ASSERT(direct3dImmediateContext);

	constexpr ID3D11ClassInstance* const* noInterfaces = nullptr;
	constexpr unsigned int interfaceCount = 0;
	// Vertex shader
	{
		EAE6320_ASSERT((m_vertexShader != nullptr) && (m_vertexShader->m_shaderObject.vertex != nullptr));
		direct3dImmediateContext->VSSetShader(m_vertexShader->m_shaderObject.vertex, noInterfaces, interfaceCount);
	}
	// Fragment shader
	{
		EAE6320_ASSERT((m_fragmentShader != nullptr) && (m_fragmentShader->m_shaderObject.vertex != nullptr));
		direct3dImmediateContext->PSSetShader(m_fragmentShader->m_shaderObject.fragment, noInterfaces, interfaceCount);
	}
	// Render state
	{
		m_renderState.Bind();
	}
}

eae6320::cResult eae6320::Graphics::cEffect::CleanUp_platformSpecific()
{
	//We don't have any platform dependent effect cleanups
	auto result = Results::Success;
	return result;
}