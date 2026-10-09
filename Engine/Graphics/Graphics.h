/*
	This file declares the external interface for the graphics system
*/

#ifndef EAE6320_GRAPHICS_H
#define EAE6320_GRAPHICS_H

// Includes
//=========

#include "Configuration.h"

#include <cstdint>
#include <Engine/Results/Results.h>

#if defined( EAE6320_PLATFORM_WINDOWS )
	#include <Engine/Windows/Includes.h>
#endif

// Interface
//==========

namespace eae6320
{
	namespace Math
	{
		class cMatrix_transformation;
		class cQuaternion;
		struct sVector;
	}

	namespace GameObject
	{
		class cCamera;
	}

	namespace Texture
	{
		class cTexture;
	}
}

namespace eae6320
{
	namespace Graphics
	{
		// Submission
		//-----------
		class cMesh;
		class cEffect;
		// These functions should be called from the application (on the application loop thread)

		// As the class progresses you will add your own functions for submitting data,
		// but the following is an example (that gets called automatically)
		// of how the application submits the total elapsed times
		// for the frame currently being submitted
		void SubmitElapsedTime( const float i_elapsedSecondCount_systemTime, const float i_elapsedSecondCount_simulationTime );
		//Submit the Camera data
		// the values are the camera object, the vertical field of view, focal length, near plane and far plane
		//void SubmitCameraDataForANewFrame(cCamera* i_camera, const float i_verticalFieldOfView, const float i_focalLength, const float i_nearPlaneDistance, const float i_farPlaneDistance);
		//void SubmitCameraDataForANewFrame(GameObject::cCamera* i_camera, eae6320::Math::cMatrix_transformation& i_transform);
		void SubmitCameraDataForANewFrame(const Math::sVector& i_position, const Math::cQuaternion& i_orientation, float i_fieldOfView, float i_aspectRatio, float i_nearZPlane, float i_farZPlane);

		//Submit the background color data for the new frame
		//the values for red, green and blue should be between 0.0f to 1.0f
		void SubmitBackgroundColorForANewFrame(const float i_redColorValue, const float i_greenColorValue, const float i_blueColorValue);
		//Submit the coordinates of the triangle to be drawn and the order of it
        // and pass the effect to be attached to the same
		void SubmitCoordinateWithOrderAndEffectForANewFrame(cMesh*& o_mesh, cEffect*& o_effect, Math::cMatrix_transformation& i_transform, Texture::cTexture*& o_texture, unsigned int i_textureSlot);
		// When the application is ready to submit data for a new frame
		// it should call this before submitting anything
		// (or, said another way, it is not safe to submit data for a new frame
		// until this function returns successfully)
		cResult WaitUntilDataForANewFrameCanBeSubmitted( const unsigned int i_timeToWait_inMilliseconds );
		// When the application has finished submitting data for a frame
		// it must call this function
		cResult SignalThatAllDataForAFrameHasBeenSubmitted();

		// Render
		//-------

		// This is called (automatically) from the main/render thread.
		// It will render a submitted frame as soon as it is ready
		// (i.e. as soon as SignalThatAllDataForAFrameHasBeenSubmitted() has been called)
		void RenderFrame();

		// Initialize / Clean Up
		//----------------------

		struct sInitializationParameters
		{
			
#if defined( EAE6320_PLATFORM_WINDOWS )
			HWND mainWindow = NULL;
	#if defined( EAE6320_PLATFORM_D3D )
			uint16_t resolutionWidth = 0, resolutionHeight = 0;
	#elif defined( EAE6320_PLATFORM_GL )
			HINSTANCE thisInstanceOfTheApplication = NULL;
	#endif
#endif
		};

		cResult Initialize( const sInitializationParameters& i_initializationParameters );
		cResult CleanUp();
	}
}

#endif	// EAE6320_GRAPHICS_H
