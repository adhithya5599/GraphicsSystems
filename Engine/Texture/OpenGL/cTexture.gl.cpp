#include "../cTexture.h"
#include "Includes.h"
#include <Engine/Logging/Logging.h>

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
		constexpr GLint format = GL_BGRA;
		constexpr GLint level = 0;
		constexpr GLint border = 0;
		glTexImage2D(GL_TEXTURE_2D, level, format, m_width, m_height, border, 
			format, GL_UNSIGNED_BYTE, m_pixels);
		const auto errorCode = glGetError();
		if (errorCode == GL_NO_ERROR)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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