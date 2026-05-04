/*
	This struct represents a three-dimensional position or direction
*/

#ifndef EAE6320_MATH_SVECTOR_H
#define EAE6320_MATH_SVECTOR_H

#include <limits>
#include "sVector2D.h"

// Struct Declaration
//===================

namespace eae6320
{
	namespace Math
	{
		class cQuaternion;

		struct sVector
		{
			// Data
			//=====

			float x = 0.0f, y = 0.0f, z = 0.0f;

			// Interface
			//==========

			// Addition
			//---------

			constexpr sVector operator +( const sVector& i_rhs ) const;
			constexpr sVector& operator +=( const sVector& i_rhs );

			constexpr sVector operator +( const float i_rhs ) const;
			constexpr sVector& operator +=( const float i_rhs );
			friend constexpr sVector operator +( const float i_lhs, const sVector& i_rhs );


			// Subtraction / Negation
			//-----------------------

			constexpr sVector operator -( const sVector& i_rhs ) const;
			constexpr sVector& operator -=( const sVector& i_rhs );
			constexpr sVector operator -() const;

			constexpr sVector operator -( const float i_rhs ) const;
			constexpr sVector& operator -=( const float i_rhs );
			friend constexpr sVector operator -( const float i_lhs, const sVector& i_rhs );

			// Products
			//---------

			constexpr sVector operator *( const float i_rhs ) const;
			constexpr sVector& operator *=( const float i_rhs );
			friend constexpr sVector operator *( const float i_lhs, const sVector& i_rhs );

			friend constexpr float Dot( const sVector& i_lhs, const sVector& i_rhs );
			friend constexpr sVector Cross( const sVector& i_lhs, const sVector& i_rhs );

			// Division
			//---------

			sVector operator /( const float i_rhs ) const;
			sVector& operator /=( const float i_rhs );

			// Length / Normalization
			//-----------------------

			float GetLength() const;
			float Normalize();
			sVector GetNormalized() const;

			// Comparison
			//-----------

			constexpr bool operator ==( const sVector& i_rhs ) const;
			constexpr bool operator !=( const sVector& i_rhs ) const;

			// (CameraControls new operations)
			//================================

			// Comparison
			//-----------

			static constexpr bool Approximately(const sVector& i_lhs, const sVector& i_rhs, float i_epsilon = std::numeric_limits<float>::epsilon());

			// Lerp
			//-----

			static constexpr sVector Lerp(const sVector& i_from, const sVector& i_to, float i_time);
			static constexpr sVector LerpUnclamped(const sVector& i_from, const sVector& i_to, float i_time);

			// Clamp
			//------

			static constexpr sVector Clamp(const sVector& i_value, const sVector& i_min, const sVector& i_max);

			// Damping
			//--------

			static constexpr sVector Damp(const sVector& i_from, const sVector& i_to, float i_halflife, float i_deltaSeconds);
			static constexpr sVector Damp(const sVector& i_from, const sVector& i_to, const sVector& i_halflife, float i_deltaSeconds);
			static constexpr sVector SmoothDamp(const sVector& i_from, const sVector& i_to, float i_smoothTime, float i_deltaSeconds, sVector& io_velocity);
			static constexpr sVector SmoothDamp(const sVector& i_from, const sVector& i_to, const sVector& i_smoothTime, float i_deltaSeconds, sVector& io_velocity);

			// Projection
			//-----------

			static sVector Project(const sVector& i_toProject, const sVector& i_projectedOnto);
			static sVector ProjectOntoPlane(const sVector& i_toProject, const sVector& i_planeNormal);

			// (end CameraControls new operations)

			// Initialization / Clean Up
			//--------------------------

			constexpr sVector() = default;
			constexpr sVector( const float i_x, const float i_y, const float i_z );
		};

		// Friends
		//========

		constexpr sVector operator +( const float i_lhs, const sVector& i_rhs );
		constexpr sVector operator -( const float i_lhs, const sVector& i_rhs );
		constexpr sVector operator *( const float i_lhs, const sVector& i_rhs );
		constexpr float Dot( const sVector& i_lhs, const sVector& i_rhs );
		constexpr sVector Cross( const sVector& i_lhs, const sVector& i_rhs );

		// (CameraControls new operations)
		//================================

		// Screen space
		//-------------

		sVector2D WorldSpaceToScreenSpace(
			const sVector& i_worldPosition,
			const sVector& i_cameraPosition,
			const cQuaternion& i_cameraOrientation,
			const sVector2D& i_fieldOfViewRadians);

		sVector ScreenSpaceToWorldSpace(
			const sVector2D& i_screenPosition,
			const sVector& i_cameraPosition,
			const cQuaternion& i_cameraOrientation,
			const sVector2D& i_fieldOfViewRadians);

		// (end CameraControls new operations)
	}
}

#include "sVector.inl"

#endif	// EAE6320_MATH_SVECTOR_H
