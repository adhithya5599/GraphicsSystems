#include "cEffect.h"

#include <Engine/ScopeGuard/cScopeGuard.h>
#include <Engine/Asserts/Asserts.h>
#include <Engine/Logging/Logging.h>

eae6320::cResult eae6320::Graphics::cEffect::Load(cEffect*& o_effect, const std::string& i_vertexShaderPath, const std::string& i_fragmentShaderPath)
{
	auto result = Results::Success;
	cEffect* newEffect = nullptr;
	cScopeGuard scopeGuard([&o_effect, &result, &newEffect]
		{
			if (result)
			{
				EAE6320_ASSERT(newEffect != nullptr);
				o_effect = newEffect;
			}
			else
			{
				if (newEffect)
				{
					newEffect->DecrementReferenceCount();
					newEffect = nullptr;
				}
				o_effect = nullptr;
			}
		});

	//Allocate a new Effect
	{
		newEffect = new cEffect();
		if (!newEffect)
		{
			result = Results::OutOfMemory;
			EAE6320_ASSERTF(false, "Couldn't allocate memory for the effect");
			Logging::OutputError("Couldn't allocate memory for the effect");
			return result;
		}
	}

	//Initialize the shading data
	{
		if (!(result = newEffect->InitializeShadingData(i_vertexShaderPath, i_fragmentShaderPath)))
		{
			EAE6320_ASSERTF(false, "Can't initialize Graphics without the shading data");
			return result;
		}
	}
	
	return result;
}

eae6320::cResult eae6320::Graphics::cEffect::InitializeShadingData(const std::string& i_vertexShaderPath, const std::string& i_fragmentShaderPath)
{
	auto result = eae6320::Results::Success;	
	if (!(result = eae6320::Graphics::cShader::Load(i_vertexShaderPath,
		m_vertexShader, eae6320::Graphics::eShaderType::Vertex)))
	{
		EAE6320_ASSERTF(false, "Can't initialize shading data without vertex shader");
		return result;
	}
	if (!(result = eae6320::Graphics::cShader::Load(i_fragmentShaderPath,
		m_fragmentShader, eae6320::Graphics::eShaderType::Fragment)))
	{
		EAE6320_ASSERTF(false, "Can't initialize shading data without fragment shader");
		return result;
	}

	{
		constexpr auto renderStateBits = []
		{
			uint8_t renderStateBits = 0;
			eae6320::Graphics::RenderStates::DisableAlphaTransparency(renderStateBits);
			//eae6320::Graphics::RenderStates::EnableAlphaTransparency(renderStateBits);
			eae6320::Graphics::RenderStates::EnableDepthTesting(renderStateBits);
			eae6320::Graphics::RenderStates::EnableDepthWriting(renderStateBits);
			//eae6320::Graphics::RenderStates::DisableDepthTesting(renderStateBits);
			//eae6320::Graphics::RenderStates::DisableDepthWriting(renderStateBits);
			//eae6320::Graphics::RenderStates::DisableDrawingBothTriangleSides(renderStateBits);
			eae6320::Graphics::RenderStates::EnableDrawingBothTriangleSides(renderStateBits);

			return renderStateBits;
		}();
		if (!(result = m_renderState.Initialize(renderStateBits)))
		{
			EAE6320_ASSERTF(false, "Can't initialize shading data without render state"); 
			return result;
		}
	}
	
	if (!(result = Initialize_platformSpecific()))
	{
		EAE6320_ASSERTF(false, "Can't initialize platform specific fragment or shading data");
		return result;
	}

	return result;
}

eae6320::cResult eae6320::Graphics::cEffect::CleanUp()
{
	auto result = Results::Success;
	if ( m_vertexShader )
	{
		m_vertexShader->DecrementReferenceCount();
		m_vertexShader = nullptr;
	}
	if ( m_fragmentShader )
	{
		m_fragmentShader->DecrementReferenceCount();
		m_fragmentShader = nullptr;
	}

	if (!(result = CleanUp_platformSpecific() ) )
	{
		EAE6320_ASSERTF(false, "Couldn't clean up platform specific effect data");
		return result;
	}

	return result;
}

eae6320::Graphics::cEffect::~cEffect()
{
	EAE6320_ASSERT(m_referenceCount == 0);
	const auto result = CleanUp();
	EAE6320_ASSERT(result);
}