#include "Graphics.h"

#include "cConstantBuffer.h"
#include "ConstantBufferFormats.h"
#include "cMesh.h"
#include "cEffect.h"
#include "sContext.h"

#include <Engine/Concurrency/cEvent.h>
#include <Engine/Logging/Logging.h>
#include <Engine/UserOutput/UserOutput.h>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/GameObject/cCamera.h>
#include <Engine/Lighting/cSceneLighting.h>
#include <Engine/Math/Functions.h>
#include <Engine/Texture/cTexture.h>

namespace
{	
	// Constant buffer object Common
	eae6320::Graphics::cConstantBuffer s_constantBuffer_frame(eae6320::Graphics::ConstantBufferTypes::Frame);

	//Constant Buffer object Draw
	eae6320::Graphics::cConstantBuffer s_constantBuffer_draw(eae6320::Graphics::ConstantBufferTypes::DrawCall);

	// Lighting
	//---------

	// The Graphics system owns the scene's lighting:
	// it loads the lighting file itself, converts it into constant buffer data, and releases it on clean up.
	// This is why the game never has to know that the Lighting system exists.
	// (The path follows the same convention as the other engine-owned data paths, like the vertex input layout shader:
	// a game provides its lighting by listing "Lighting/scene.lighting" in its AssetsToBuild.lua.)
	constexpr auto* const s_path_sceneLighting = "data/Lighting/scene.lighting";
	eae6320::Graphics::cConstantBuffer s_constantBuffer_lighting(eae6320::Graphics::ConstantBufferTypes::Lighting);
	// This holds a reference to the loaded lighting asset
	eae6320::Lighting::cSceneLighting* s_sceneLighting = nullptr;

	struct DrawAndColorParameters
	{
		eae6320::Graphics::cEffect* effectData = nullptr;
		eae6320::Graphics::cMesh* drawData = nullptr;
		eae6320::Texture::cTexture* textureData = nullptr;
		eae6320::Graphics::ConstantBufferFormats::sDrawCall drawData_frame;
		unsigned int textureSlot = 0;
	};

	// This struct's data is populated at submission time;
	// it must cache whatever is necessary in order to render a frame
	constexpr unsigned int countDrawAndEffect = 200;
	struct sDataRequiredToRenderAFrame
	{
		eae6320::Graphics::ConstantBufferFormats::sFrame constantData_frame;
		float backGroundColor[3];
		DrawAndColorParameters drawColorParameters[countDrawAndEffect];
		unsigned int countRenderFrame = 0;
	};
	// In our class there will be two copies of the data required to render a frame:
	//	* One of them will be in the process of being populated by the data currently being submitted by the application loop thread
	//	* One of them will be fully populated and in the process of being rendered from in the render thread
	// (In other words, one is being produced while the other is being consumed)
	sDataRequiredToRenderAFrame s_dataRequiredToRenderAFrame[2];
	auto* s_dataBeingSubmittedByApplicationThread = &s_dataRequiredToRenderAFrame[0];
	auto* s_dataBeingRenderedByRenderThread = &s_dataRequiredToRenderAFrame[1];

	// The following two events work together to make sure that
	// the main/render thread and the application loop thread can work in parallel but stay in sync:
	// This event is signaled by the application loop thread when it has finished submitting render data for a frame
	// (the main/render thread waits for the signal)
	eae6320::Concurrency::cEvent s_whenAllDataHasBeenSubmittedFromApplicationThread;
	// This event is signaled by the main/render thread when it has swapped render data pointers.
	// This means that the renderer is now working with all the submitted data it needs to render the next frame,
	// and the application loop thread can start submitting data for the following frame
	// (the application loop thread waits for the signal)
	eae6320::Concurrency::cEvent s_whenDataForANewFrameCanBeSubmittedFromApplicationThread;

	//eae6320::GameObject::cCamera* camera = new eae6320::GameObject::cCamera();
}

// Helper Declarations
//====================

namespace
{
	eae6320::cResult InitializeLighting();
	void FillLightingConstantData(const eae6320::Lighting::sLightingData& i_lightingData,
		eae6320::Graphics::ConstantBufferFormats::sLighting& o_constantData_lighting);
}

//Submission
//==========

void eae6320::Graphics::SubmitElapsedTime(const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_simulationTime)
{
	EAE6320_ASSERT(s_dataBeingSubmittedByApplicationThread);
	auto& constantData_frame = s_dataBeingSubmittedByApplicationThread->constantData_frame;
	constantData_frame.g_elapsedSecondCount_systemTime = i_elapsedSecondCount_systemTime;
	constantData_frame.g_elapsedSecondCount_simulationTime = i_elapsedSecondCount_simulationTime;	
}

//void eae6320::Graphics::SubmitCameraDataForANewFrame(eae6320::GameObject::cCamera* i_camera, eae6320::Math::cMatrix_transformation& i_transform)
//{
//	EAE6320_ASSERT(s_dataBeingSubmittedByApplicationThread);
//	auto& constantData_frame = s_dataBeingSubmittedByApplicationThread->constantData_frame;
//	constantData_frame.g_transform_worldToCamera = i_transform;
//	constantData_frame.g_transform_cameraToProjected = i_camera->GetCameraConfigurations();
//
//}

void eae6320::Graphics::SubmitCameraDataForANewFrame(const eae6320::Math::sVector& i_position, const eae6320::Math::cQuaternion& i_orientation, float i_fieldOfView, float i_aspectRatio, float i_nearZPlane, float i_farZPlane)
{
	EAE6320_ASSERT(s_dataBeingSubmittedByApplicationThread);
	auto& constantData_frame = s_dataBeingSubmittedByApplicationThread->constantData_frame;
	constantData_frame.g_transform_worldToCamera = i_transform;
	constantData_frame.g_transform_cameraToProjected = i_camera->GetCameraConfigurations();
	// The camera's world position is the translation of the inverse of the world-to-camera transform
	// (and because a camera only has rotation and translation,
	// CreateWorldToCameraTransform() is exactly the inverse that is needed).
	// Calculating it from the transform that is actually used for rendering keeps the two consistent,
	// regardless of how the game stores its camera.
	{
		const auto transform_cameraToWorld = Math::cMatrix_transformation::CreateWorldToCameraTransform(i_transform);
		const auto& cameraPosition_world = transform_cameraToWorld.GetTranslation();
		constantData_frame.g_cameraPosition_world[0] = cameraPosition_world.x;
		constantData_frame.g_cameraPosition_world[1] = cameraPosition_world.y;
		constantData_frame.g_cameraPosition_world[2] = cameraPosition_world.z;
		constantData_frame.g_cameraPosition_world[3] = 1.0f;
	}

	constantData_frame.g_transform_cameraToProjected = eae6320::Math::cMatrix_transformation::CreateCameraToProjectedTransform_perspective(i_fieldOfView, i_aspectRatio, i_nearZPlane, i_farZPlane);
}

void eae6320::Graphics::SubmitBackgroundColorForANewFrame(const float i_redColorValue, const float i_greenColorValue, const float i_blueColorValue)
{
	EAE6320_ASSERT(s_dataBeingSubmittedByApplicationThread);
	auto& backgroundColor_frame = s_dataBeingSubmittedByApplicationThread->backGroundColor;
	backgroundColor_frame[0] = i_redColorValue;
	backgroundColor_frame[1] = i_greenColorValue;
	backgroundColor_frame[2] = i_blueColorValue;
}

void eae6320::Graphics::SubmitCoordinateWithOrderAndEffectForANewFrame(eae6320::Graphics::cMesh*& o_mesh, eae6320::Graphics::cEffect*& o_effect, eae6320::Math::cMatrix_transformation& i_transform, eae6320::Texture::cTexture*& o_texture, unsigned int i_textureSlot)
{
	EAE6320_ASSERT(s_dataBeingSubmittedByApplicationThread);
	
	auto& countRenderFrame_frame = s_dataBeingSubmittedByApplicationThread->countRenderFrame;
	
	if (countRenderFrame_frame >= countDrawAndEffect)
	{
		EAE6320_ASSERTF(false, "Couldn't get the new graphics data");
		Logging::OutputError("Exceeded mesh and effect memory allocated");
		UserOutput::Print("The renderer failed to signal to the application that new graphics data can be submitted."
			" The application is probably in a bad state and should be exited");
		return;
	}
	
	auto& mesh_data_frame = s_dataBeingSubmittedByApplicationThread->drawColorParameters[countRenderFrame_frame].drawData;
	mesh_data_frame = o_mesh;
	mesh_data_frame->IncrementReferenceCount();

	auto& effect_data_frame = s_dataBeingSubmittedByApplicationThread->drawColorParameters[countRenderFrame_frame].effectData;
	effect_data_frame = o_effect;
	effect_data_frame->IncrementReferenceCount();

	auto& drawData_frame = s_dataBeingSubmittedByApplicationThread->drawColorParameters[countRenderFrame_frame].drawData_frame;
	drawData_frame.g_transform_localToWorld = i_transform;

	auto& texture_data_frame = s_dataBeingSubmittedByApplicationThread->drawColorParameters[countRenderFrame_frame].textureData;
	texture_data_frame = o_texture;
	if (texture_data_frame)
	{
		texture_data_frame->IncrementReferenceCount();
	}

	auto& textureSlot_data_frame = s_dataBeingSubmittedByApplicationThread->drawColorParameters[countRenderFrame_frame].textureSlot;
	textureSlot_data_frame = i_textureSlot;

	++countRenderFrame_frame;
}

eae6320::cResult eae6320::Graphics::WaitUntilDataForANewFrameCanBeSubmitted(const unsigned int i_timeToWait_inMilliseconds)
{
	return Concurrency::WaitForEvent(s_whenDataForANewFrameCanBeSubmittedFromApplicationThread, i_timeToWait_inMilliseconds);
}

eae6320::cResult eae6320::Graphics::SignalThatAllDataForAFrameHasBeenSubmitted()
{
	return s_whenAllDataHasBeenSubmittedFromApplicationThread.Signal();
}

// Render
//-------

void eae6320::Graphics::RenderFrame()
{
	// Wait for the application loop to submit data to be rendered
	{
		if ( Concurrency::WaitForEvent( s_whenAllDataHasBeenSubmittedFromApplicationThread ) )
		{
			// Switch the render data pointers so that
			// the data that the application just submitted becomes the data that will now be rendered
			std::swap(s_dataBeingSubmittedByApplicationThread, s_dataBeingRenderedByRenderThread);
			// Once the pointers have been swapped the application loop can submit new data
			if ( !s_whenDataForANewFrameCanBeSubmittedFromApplicationThread.Signal() )
			{
				EAE6320_ASSERTF(false, "Couldn't signal that new graphics data can be submitted");
				Logging::OutputError("Failed to signal that new render data can be submitted");
				UserOutput::Print("The renderer failed to signal to the application that new graphics data can be submitted."
					" The application is probably in a bad state and should be exited");
				return;
			}
		}
		else
		{
			EAE6320_ASSERTF(false, "Waiting for the graphics data to be submitted failed");
			Logging::OutputError("Waiting for the application loop to submit data to be rendered failed");
			UserOutput::Print("The renderer failed to wait for the application to submit data to be rendered."
				" The application is probably in a bad state and should be exited");
			return;
		}
	}

	// Every frame an entirely new image will be created.
	// Before drawing anything, then, the previous image will be erased
	// by "clearing" the image buffer (filling it with a solid color)
	{
		// Index 0 -> Red, 1 -> Blue, 2 -> Green
		//constexpr float backGroundColor[] = {
		//	1.0f, 0.0f, 1.0f
		//};
		EAE6320_ASSERT(s_dataBeingRenderedByRenderThread);
		auto& backgroundColor_frame = s_dataBeingRenderedByRenderThread->backGroundColor;
		sContext::g_context.ClearImageBuffer(backgroundColor_frame);
	}

	// In addition to the color buffer there is also a hidden image called the "depth buffer"
	// which is used to make it less important which order draw calls are made.
	// It must also be "cleared" every frame just like the visible color buffer.
	{
		sContext::g_context.ClearDepthBuffer();
	}

	EAE6320_ASSERT(s_dataBeingRenderedByRenderThread);

	//Platform d3d
	//-----------
	//auto* const dataRequiredToRenderFrame = s_dataBeingRenderedByRenderThread;
	//------------

	// Update the frame constant buffer
	{
		// Copy the data from the system memory that the application owns to GPU memory
		auto& constantData_frame = s_dataBeingRenderedByRenderThread->constantData_frame;
		s_constantBuffer_frame.Update(&constantData_frame);
	}

	auto& countRenderFrame_frame = s_dataBeingRenderedByRenderThread->countRenderFrame;
	for (unsigned int i = 0; i < countRenderFrame_frame; i++)
	{
		auto& mesh_data_frame = s_dataBeingRenderedByRenderThread->drawColorParameters[i].drawData;
		auto& effect_data_frame = s_dataBeingRenderedByRenderThread->drawColorParameters[i].effectData;
		auto& texture_data_frame = s_dataBeingRenderedByRenderThread->drawColorParameters[i].textureData;
		auto& textureSlot_data_frame = s_dataBeingRenderedByRenderThread->drawColorParameters[i].textureSlot;

		//Update the draw constant buffer
		{
			auto& drawData_frame = s_dataBeingRenderedByRenderThread->drawColorParameters[i].drawData_frame;
			s_constantBuffer_draw.Update(&drawData_frame);
		}

		// Bind the shading data
		{
			effect_data_frame->Bind();
		}
		//Bind the texture data
		{
			if (texture_data_frame)
			{
				texture_data_frame->Bind(textureSlot_data_frame);
			}
		}
		// Draw the geometry
		{
			mesh_data_frame->Draw();
		}

		if (mesh_data_frame)
		{
			mesh_data_frame->DecrementReferenceCount();
			mesh_data_frame = nullptr;
		}
		if (effect_data_frame)
		{
			effect_data_frame->DecrementReferenceCount();
			effect_data_frame = nullptr;
		}
		if (texture_data_frame)
		{
			texture_data_frame->DecrementReferenceCount();
			texture_data_frame = nullptr;
		}
	}
	countRenderFrame_frame = 0;

	// Everything has been drawn to the "back buffer", which is just an image in memory.
	// In order to display it the contents of the back buffer must be "presented"
	// (or "swapped" with the "front buffer", which is the image that is actually being displayed)
	{
		s_constantBuffer_frame.SwapBuffer();
	}

	// After all of the data that was submitted for this frame has been used
	// you must make sure that it is all cleaned up and cleared out
	// so that the struct can be re-used (i.e. so that data for a new frame can be submitted to it)
	{
		// (At this point in the class there isn't anything that needs to be cleaned up)
		//dataRequiredToRenderFrame	// TODO
	}
}

// Initialize / Clean Up
//----------------------

eae6320::cResult eae6320::Graphics::Initialize(const sInitializationParameters& i_initializationParameters)
{
	auto result = Results::Success;
	// Initialize the platform-specific context
	if ( !( result = sContext::g_context.Initialize(i_initializationParameters) ) )
	{
		EAE6320_ASSERTF(false, "Can't initialize Graphics without context");
		return result;
	}
	// Initialize the platform-independent graphics objects
	{
		if ( result = s_constantBuffer_frame.Initialize() )
		{
			// There is only a single frame constant buffer that is reused
			// and so it can be bound at initialization time and never unbound
			s_constantBuffer_frame.Bind(
				// In our class both vertex and fragment shaders use per-frame constant data
				static_cast<uint_fast8_t>(eShaderType::Vertex) | static_cast<uint_fast8_t>(eShaderType::Fragment ) );
		}
		else
		{
			EAE6320_ASSERTF(false, "Can't initialize Graphics without frame constant buffer");
			return result;
		}

		if (result = s_constantBuffer_draw.Initialize())
		{
			s_constantBuffer_draw.Bind(
				static_cast<uint_fast8_t>(eShaderType::Vertex) | static_cast<uint_fast8_t>(eShaderType::Fragment));
		}
		else
		{
			EAE6320_ASSERTF(false, "Can't initialize Graphics without draw constant buffer");
			return result;
		}
	}
	// Initialize the lighting
	{
		if (!(result = InitializeLighting()))
		{
			EAE6320_ASSERTF(false, "Can't initialize Graphics without lighting");
			return result;
		}
	}
	// Initialize the events
	{
		if ( !( result = s_whenAllDataHasBeenSubmittedFromApplicationThread.Initialize(Concurrency::EventType::ResetAutomaticallyAfterBeingSignaled ) ) )
		{
			EAE6320_ASSERTF(false, "Can't initialize Graphics without event for when data has been submitted from the application thread");
			return result;
		}
		if ( !( result = s_whenDataForANewFrameCanBeSubmittedFromApplicationThread.Initialize(Concurrency::EventType::ResetAutomaticallyAfterBeingSignaled,
			Concurrency::EventState::Signaled ) ) )
		{
			EAE6320_ASSERTF(false, "Can't initialize Graphics without event for when data can be submitted from the application thread");
			return result;
		}
	}

	// Initialize the views
	{
		if ( !( result = sContext::g_context.InitializeViews(i_initializationParameters)))
		{
			EAE6320_ASSERTF(false, "Can't initialize Graphics without the views");
			return result;
		}
	}
	return result;
}

eae6320::cResult eae6320::Graphics::CleanUp()
{
	auto result = Results::Success;
	{
		auto& countRenderFrame_frame = s_dataBeingRenderedByRenderThread->countRenderFrame;
		for (unsigned int i = 0; i < countRenderFrame_frame; i++)
		{
			auto& mesh_data_frame = s_dataBeingRenderedByRenderThread->drawColorParameters[i].drawData;
			auto& effect_data_frame = s_dataBeingRenderedByRenderThread->drawColorParameters[i].effectData;
			auto& texture_data_frame = s_dataBeingRenderedByRenderThread->drawColorParameters[i].textureData;
			if (mesh_data_frame)
			{
				mesh_data_frame->DecrementReferenceCount();
				mesh_data_frame = nullptr;
			}
			if (effect_data_frame)
			{
				effect_data_frame->DecrementReferenceCount();
				effect_data_frame = nullptr;
			}
			if (texture_data_frame)
			{
				texture_data_frame->DecrementReferenceCount();
				texture_data_frame = nullptr;
			}
		}
	}
	{
		auto& countRenderFrame_frame = s_dataBeingSubmittedByApplicationThread->countRenderFrame;
		for (unsigned int i = 0; i < countRenderFrame_frame; i++)
		{
			auto& mesh_data_frame = s_dataBeingSubmittedByApplicationThread->drawColorParameters[i].drawData;
			auto& effect_data_frame = s_dataBeingSubmittedByApplicationThread->drawColorParameters[i].effectData;
			auto& texture_data_frame = s_dataBeingSubmittedByApplicationThread->drawColorParameters[i].textureData;
			if (mesh_data_frame)
			{
				mesh_data_frame->DecrementReferenceCount();
				mesh_data_frame = nullptr;
			}
			if (effect_data_frame)
			{
				effect_data_frame->DecrementReferenceCount();
				effect_data_frame = nullptr;
			}
			if (texture_data_frame)
			{
				texture_data_frame->DecrementReferenceCount();
				texture_data_frame = nullptr;
			}
		}
	}

	{
		const auto result_constantBuffer_frame = s_constantBuffer_frame.CleanUp();
		if ( !result_constantBuffer_frame )
		{
			EAE6320_ASSERT(false);
			if ( result )
			{
				result = result_constantBuffer_frame;
			}
		}
	}

	// Release the lighting.
	// Graphics holds the only reference, so releasing it deletes the asset.
	{
		if (s_sceneLighting)
		{
			s_sceneLighting->DecrementReferenceCount();
			s_sceneLighting = nullptr;
		}
		const auto result_constantBuffer_lighting = s_constantBuffer_lighting.CleanUp();
		if (!result_constantBuffer_lighting)
		{
			EAE6320_ASSERT(false);
			if (result)
			{
				result = result_constantBuffer_lighting;
			}
		}
	}

	{
		const auto result_constantBuffer_draw = s_constantBuffer_draw.CleanUp();
		if (!result_constantBuffer_draw)
		{
			EAE6320_ASSERT(false);
			{
				if (result)
				{
					result = result_constantBuffer_draw;
				}
			}
		}
	}

	{
		const auto result_context = sContext::g_context.CleanUp();
		if ( !result_context )
		{
			EAE6320_ASSERT(false);
			if ( result )
			{
				result = result_context;
			}
		}
	}

	s_dataBeingSubmittedByApplicationThread = nullptr;
	s_dataBeingRenderedByRenderThread = nullptr;
	return result;
}

// Helper Definitions
//===================

namespace
{
	eae6320::cResult InitializeLighting()
	{
		auto result = eae6320::Results::Success;

		// Load the scene's lighting.
		// A missing or invalid lighting file is reported but isn't fatal:
		// the default lighting is used instead so that the game can still be seen
		// (whereas a missing mesh or shader can't be substituted with anything sensible).
		if (!eae6320::Lighting::cSceneLighting::Load(s_path_sceneLighting, s_sceneLighting))
		{
			eae6320::Logging::OutputError("The scene lighting couldn't be loaded from \"%s\" so the default lighting will be used"
				" (is \"Lighting/scene.lighting\" listed in the game's AssetsToBuild.lua?)", s_path_sceneLighting);
			if (!(result = eae6320::Lighting::cSceneLighting::CreateDefault(s_sceneLighting)))
			{
				EAE6320_ASSERTF(false, "Couldn't create the default lighting");
				return result;
			}
		}
		EAE6320_ASSERT(s_sceneLighting);

		// Convert the lighting into the layout that the shaders expect
		// and create the constant buffer with that data already in it
		eae6320::Graphics::ConstantBufferFormats::sLighting constantData_lighting{};
		FillLightingConstantData(s_sceneLighting->GetData(), constantData_lighting);
		if (result = s_constantBuffer_lighting.Initialize(&constantData_lighting))
		{
			// The lights don't change, so like the frame constant buffer
			// this is bound once and never unbound.
			// Only fragment shaders use it because lighting is calculated per fragment.
			s_constantBuffer_lighting.Bind(static_cast<uint_fast8_t>(eae6320::Graphics::eShaderType::Fragment));
		}
		else
		{
			EAE6320_ASSERTF(false, "Can't initialize Graphics without the lighting constant buffer");
			return result;
		}

		return result;
	}

	void FillLightingConstantData(const eae6320::Lighting::sLightingData& i_lightingData,
		eae6320::Graphics::ConstantBufferFormats::sLighting& o_constantData_lighting)
	{
		const auto SetColor = [](float (&o_color)[4], const eae6320::Lighting::sColor& i_color, const float i_intensity)
		{
			o_color[0] = i_color.r * i_intensity;
			o_color[1] = i_color.g * i_intensity;
			o_color[2] = i_color.b * i_intensity;
			o_color[3] = 1.0f;
		};
		const auto SetVector = [](float (&o_vector)[4], const eae6320::Math::sVector& i_vector, const float i_w)
		{
			o_vector[0] = i_vector.x;
			o_vector[1] = i_vector.y;
			o_vector[2] = i_vector.z;
			o_vector[3] = i_w;
		};

		// Ambient
		SetColor(o_constantData_lighting.g_ambient_skyColor, i_lightingData.ambient.skyColor, i_lightingData.ambient.intensity);
		SetColor(o_constantData_lighting.g_ambient_groundColor, i_lightingData.ambient.groundColor, i_lightingData.ambient.intensity);
		// Directional
		{
			// The file stores the direction the light travels,
			// but the shading math needs the direction from the surface toward the light (the opposite)
			SetVector(o_constantData_lighting.g_directional_directionToLight, -i_lightingData.directional.direction, 0.0f);
			SetColor(o_constantData_lighting.g_directional_color, i_lightingData.directional.color, i_lightingData.directional.intensity);
		}
		// Specular
		o_constantData_lighting.g_specular_intensity = i_lightingData.specular.intensity;
		o_constantData_lighting.g_specular_shininess = i_lightingData.specular.shininess;
		// Point lights
		{
			const auto pointLightCount = (i_lightingData.pointLightCount < eae6320::Lighting::MaxPointLightCount)
				? i_lightingData.pointLightCount : eae6320::Lighting::MaxPointLightCount;
			o_constantData_lighting.g_pointLightCount = static_cast<int32_t>(pointLightCount);
			for (unsigned int i = 0; i < pointLightCount; ++i)
			{
				const auto& pointLight = i_lightingData.pointLights[i];
				SetVector(o_constantData_lighting.g_pointLight_positionAndRange[i], pointLight.position, pointLight.range);
				SetColor(o_constantData_lighting.g_pointLight_color[i], pointLight.color, pointLight.intensity);
			}
		}
	}
}
