#include "../cTexture.h"
#include "Includes.h"
#include <Engine/Logging/Logging.h>

// Anisotropic filtering is an extension (it only became core in OpenGL 4.6),
// so its constant might not be defined by the headers
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
	#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

eae6320::cResult eae6320::Texture::cTexture::InitializeTexture()
{
	auto result = eae6320::Results::Success;

	{
		constexpr GLsizei arrayCount = 1;
		glGenTextures(arrayCount, &m_textureId);
		const auto errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			glBindTexture(GL_TEXTURE_2D, m_textureId);
			const auto errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				result = eae6320::Results::Failure;
				EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
				eae6320::Logging::OutputError("OpenGL failed to bind a new texture: %s",
					reinterpret_cast<const char*>(gluErrorString(errorCode)));
				return result;
			}
		}
		else
		{
			result = eae6320::Results::Failure;
			EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
			eae6320::Logging::OutputError("OpenGL failed to get an unused texture ID: %s",
				reinterpret_cast<const char*>(gluErrorString(errorCode)));
			return result;
		}
	}

	{
		GLint format = (m_componentsPerPixel == 4) ? GL_BGRA : GL_BGR;
		GLint internalFormat = (m_componentsPerPixel == 4) ? GL_RGBA8 : GL_RGB8;
		constexpr GLint border = 0;
		// Every mip level is uploaded (level 0 is the full-size image)
		for (size_t level = 0; level < m_mipLevels.size(); ++level)
		{
			glTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), internalFormat,
				static_cast<GLsizei>(m_mipLevels[level].width), static_cast<GLsizei>(m_mipLevels[level].height), border,
				format, GL_UNSIGNED_BYTE, m_pixels + m_mipLevels[level].offset);
		}
		const auto errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			// Blend between the two closest mip levels ("trilinear" filtering)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// Texture coordinates outside of [0,1] repeat the texture
			// (OpenGL already defaults to this, but Direct3D doesn't, so it is set explicitly to keep the platforms the same)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			const auto errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				result = eae6320::Results::Failure;
				EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
				eae6320::Logging::OutputError("OpenGL failed to set texture parameters: %s",
					reinterpret_cast<const char*>(gluErrorString(errorCode)));
				return result;
			}
		}
		else
		{
			result = eae6320::Results::Failure;
			EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
			eae6320::Logging::OutputError("OpenGL failed to get texture parameters: %s",
				reinterpret_cast<const char*>(gluErrorString(errorCode)));
			return result;
		}
	}

	// Anisotropic filtering (see cTexture.d3d.cpp) is optional:
	// if the driver doesn't support it the texture still works with trilinear filtering
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
		// (glGetError() clears the error so that it isn't mistaken for a failure later)
		const auto errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			eae6320::Logging::OutputMessage("Anisotropic texture filtering isn't available, so textures will use trilinear filtering");
		}
	}

	{
		glBindTexture(GL_TEXTURE_2D, 0);
		const auto errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			result = eae6320::Results::Failure;
			EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
			eae6320::Logging::OutputError("OpenGL failed to unbind the texture: %s",
				reinterpret_cast<const char*>(gluErrorString(errorCode)));
			return result;
		}
	}

	if (m_pixels)
	{
		delete[] m_pixels;
		m_pixels = nullptr;
	}

	return result;
}

eae6320::cResult eae6320::Texture::cTexture::CleanUpTexture()
{
	auto result = Results::Success;
	// The OpenGL texture used to never be deleted, which leaked its GPU memory
	if (m_textureId != 0)
	{
		constexpr GLsizei textureCount = 1;
		glDeleteTextures(textureCount, &m_textureId);
		const auto errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			result = eae6320::Results::Failure;
			EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
			eae6320::Logging::OutputError("OpenGL failed to delete a texture: %s",
				reinterpret_cast<const char*>(gluErrorString(errorCode)));
		}
		m_textureId = 0;
	}
	return result;
}

void eae6320::Texture::cTexture::Bind(unsigned int i_textureUnit) const
{
	{
		glActiveTexture(GL_TEXTURE0 + i_textureUnit);
		const auto errorCode = glGetError();
		if(errorCode == GL_NO_ERROR)
		{
			glBindTexture(GL_TEXTURE_2D, m_textureId);
			const auto errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
				eae6320::Logging::OutputError("OpenGL failed to bind the texture to unit %u: %s",
					i_textureUnit, reinterpret_cast<const char*>(gluErrorString(errorCode)));
			}
		}
		else
		{
			EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
			eae6320::Logging::OutputError("OpenGL failed to activate texture unit %u: %s",
				i_textureUnit, reinterpret_cast<const char*>(gluErrorString(errorCode)));
		}
	}
}

void eae6320::Texture::cTexture::Unbind(unsigned int i_textureUnit)
{
	{
		glActiveTexture(GL_TEXTURE0 + i_textureUnit);
		const auto errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			const auto errorCode = glGetError();
			if (errorCode != GL_NO_ERROR)
			{
				EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
				eae6320::Logging::OutputError("OpenGL failed to unbind the texture to unit %u: %s",
					i_textureUnit, reinterpret_cast<const char*>(gluErrorString(errorCode)));
			}
		}
		else
		{
			EAE6320_ASSERTF(false, reinterpret_cast<const char*>(gluErrorString(errorCode)));
			eae6320::Logging::OutputError("OpenGL failed to activate texture unit %u: %s",
				i_textureUnit, reinterpret_cast<const char*>(gluErrorString(errorCode)));
		}
	}
}