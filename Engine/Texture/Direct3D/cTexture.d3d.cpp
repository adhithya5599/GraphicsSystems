#include "../cTexture.h"
#include "Includes.h"
#include <Engine/Logging/Logging.h>
#include <Engine/Graphics/sContext.h>

#include <vector>

eae6320::cResult eae6320::Texture::cTexture::InitializeTexture()
{
	auto result = Results::Success;
	auto* const direct3dDevice = eae6320::Graphics::sContext::g_context.direct3dDevice;
	EAE6320_ASSERT(direct3dDevice);

	{
		D3D11_TEXTURE2D_DESC textureDescription{};
		textureDescription.Width = m_width;
		textureDescription.Height = m_height;
		textureDescription.MipLevels = static_cast<UINT>(m_mipLevels.size());
		textureDescription.ArraySize = 1;
		textureDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDescription.SampleDesc.Count = 1;
		//textureDescription.Usage = D3D11_USAGE_DEFAULT;
		textureDescription.Usage = D3D11_USAGE_IMMUTABLE;
		textureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		// An immutable texture must be given all of its mip levels when it is created
		std::vector<D3D11_SUBRESOURCE_DATA> initialData(m_mipLevels.size());
		for (size_t i = 0; i < m_mipLevels.size(); ++i)
		{
			initialData[i].pSysMem = m_pixels + m_mipLevels[i].offset;
			initialData[i].SysMemPitch = m_mipLevels[i].width * m_componentsPerPixel;
			initialData[i].SysMemSlicePitch = 0;
		}

		ID3D11Texture2D* texture = nullptr;
		
		{
			const auto result_create = direct3dDevice->CreateTexture2D(&textureDescription, initialData.data(), &texture);
			if (FAILED(result_create))
			{
				result = eae6320::Results::Failure;
				EAE6320_ASSERTF(false, "Texture creation failed (HRESULT %#010x)", result_create);
				eae6320::Logging::OutputError("Direct3D failed to create a texture (HRESULT %#010x)", result_create);

				if (texture)
				{
					texture->Release();
					texture = nullptr;
				}
				if (m_pixels)
				{
					delete[] m_pixels;
					m_pixels = nullptr;
				}
				return result;
			}
		}

		{
			D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDescription{};
			shaderResourceViewDescription.Format = textureDescription.Format;
			shaderResourceViewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			shaderResourceViewDescription.Texture2D.MipLevels = textureDescription.MipLevels;
			shaderResourceViewDescription.Texture2D.MostDetailedMip = 0;

			const auto result_create = direct3dDevice->CreateShaderResourceView(texture, &shaderResourceViewDescription, &m_textureView);
			// The view keeps its own reference to the texture,
			// so this function's reference must be released whether or not the view was created
			// (it used to never be released, which leaked every texture's GPU memory)
			texture->Release();
			texture = nullptr;
			if (FAILED(result_create))
			{
				result = eae6320::Results::Failure;
				EAE6320_ASSERTF(false, "Texture shader resource view creation failed (HRESULT %#010x)", result_create);
				eae6320::Logging::OutputError("Direct3D failed to create a texture shader resource view (HRESULT %#010x)", result_create);
				return result;
			}
		}
	}

	// The sampler state
	{
		D3D11_SAMPLER_DESC samplerDescription{};
		// Anisotropic filtering samples along the direction that the texture is stretched on screen,
		// which keeps surfaces seen at a grazing angle (like the ground stretching away from the camera) sharp
		// instead of blurring them like ordinary mip mapping would
		samplerDescription.Filter = D3D11_FILTER_ANISOTROPIC;
		samplerDescription.MaxAnisotropy = 8;
		// Texture coordinates outside of [0,1] repeat the texture
		// (so a big surface can show a small texture many times instead of one stretched copy)
		samplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDescription.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDescription.MinLOD = 0.0f;
		samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;
		const auto result_create = direct3dDevice->CreateSamplerState(&samplerDescription, &m_samplerState);
		if (FAILED(result_create))
		{
			result = eae6320::Results::Failure;
			EAE6320_ASSERTF(false, "Texture sampler state creation failed (HRESULT %#010x)", result_create);
			eae6320::Logging::OutputError("Direct3D failed to create a texture sampler state (HRESULT %#010x)", result_create);
			return result;
		}
	}

	// The pixels have been copied to the GPU and aren't needed anymore
	if (m_pixels)
	{
		delete[] m_pixels;
		m_pixels = nullptr;
	}

	return result;
}

eae6320::cResult eae6320::Texture::cTexture::CleanUpTexture()
{
	if (m_samplerState)
	{
		m_samplerState->Release();
		m_samplerState = nullptr;
	}
	if (m_textureView)
	{
		m_textureView->Release();
		m_textureView = nullptr;
	}
	return Results::Success;
}

void eae6320::Texture::cTexture::Bind(unsigned int i_textureUnit) const
{
	auto* const direct3dImmediateContext = eae6320::Graphics::sContext::g_context.direct3dImmediateContext;
	EAE6320_ASSERT(direct3dImmediateContext);

	{
		EAE6320_ASSERT(m_textureView && m_samplerState);
		direct3dImmediateContext->PSSetShaderResources(i_textureUnit, 1, &m_textureView);
		// The shaders declare a sampler in the same slot as each texture (see DeclareTexture() in shaders.inc)
		direct3dImmediateContext->PSSetSamplers(i_textureUnit, 1, &m_samplerState);
	}
}

void eae6320::Texture::cTexture::Unbind(unsigned int i_textureUnit)
{
	auto* const direct3dImmediateContext = eae6320::Graphics::sContext::g_context.direct3dImmediateContext;
	EAE6320_ASSERT(direct3dImmediateContext);

	{
		ID3D11ShaderResourceView* nullView = nullptr;
		direct3dImmediateContext->PSSetShaderResources(i_textureUnit, 1, &nullView);
	}
}