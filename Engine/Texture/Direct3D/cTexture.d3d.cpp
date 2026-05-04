#include "../cTexture.h"
#include "Includes.h"
#include <Engine/Logging/Logging.h>
#include <Engine/Graphics/sContext.h>

eae6320::cResult eae6320::Texture::cTexture::InitializeTexture()
{
	auto result = Results::Success;
	auto* const direct3dDevice = eae6320::Graphics::sContext::g_context.direct3dDevice;
	EAE6320_ASSERT(direct3dDevice);

	{
		D3D11_TEXTURE2D_DESC textureDescription{};
		textureDescription.Width = m_width;
		textureDescription.Height = m_height;
		textureDescription.MipLevels = 1;
		textureDescription.ArraySize = 1;
		textureDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDescription.SampleDesc.Count = 1;
		textureDescription.Usage = D3D11_USAGE_DEFAULT;
		textureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA initialData{};
		initialData.pSysMem = m_pixels;
		initialData.SysMemPitch = m_width * m_componentsPerPixel;

		ID3D11Texture2D* texture = nullptr;
		
		{
			const auto result_create = direct3dDevice->CreateTexture2D(&textureDescription, &initialData, &texture);
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
			const auto result_create = direct3dDevice->CreateShaderResourceView(texture, nullptr, &m_textureView);
			if (FAILED(result_create))
			{
				result = eae6320::Results::Failure;
				EAE6320_ASSERTF(false, "Texture shader resource view creation failed (HRESULT %#010x)", result_create);
				eae6320::Logging::OutputError("Direct3D failed to create a texture shader resource view (HRESULT %#010x)", result_create);
				return result;
			}
		}
	}

	return result;
}

void eae6320::Texture::cTexture::Bind(unsigned int i_textureUnit) const
{
	auto* const direct3dImmediateContext = eae6320::Graphics::sContext::g_context.direct3dImmediateContext;
	EAE6320_ASSERT(direct3dImmediateContext);

	{
		EAE6320_ASSERT(m_textureView);
		direct3dImmediateContext->PSSetShaderResources(i_textureUnit, 1, &m_textureView);
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