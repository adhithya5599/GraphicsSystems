#include "cPositionLockToTarget.h"
#include <cmath>
#include <limits>
#include <Engine/Math/cMatrix_transformation.h>
#include <Engine/Math/sVector.h>
#include <Engine/Math/cQuaternion.h>
#include <Engine/Math/Functions.h>

namespace eae6320::Camera
{
	struct sSerializedPositionLockToTarget
	{
		Math::sVector damping = Math::sVector();
		uint32_t dampingDirectionsRelativeToTarget;
	};

	const Math::sVector& cPositionLockToTarget::GetDamping() const
	{
		return m_damping;
	}

	void cPositionLockToTarget::SetDamping(const Math::sVector& i_damping)
	{
		m_damping = i_damping;
	}

	bool cPositionLockToTarget::IsDampingDirectionsRelativeToTarget() const
	{
		return m_dampingDirectionsRelativeToTarget;
	}

	void cPositionLockToTarget::SetDampingDirectionsRelativeToTarget(bool i_dampingDirectionsRelativeToTarget)
	{
		m_dampingDirectionsRelativeToTarget = i_dampingDirectionsRelativeToTarget;
	}

	Math::sVector cPositionLockToTarget::CalculatePosition(
		float i_deltaSeconds,
		const cTrackingCamera& i_trackingCamera,
		const Math::sVector& i_cameraPosition,
		const Math::cQuaternion& i_cameraOrientation,
		const Math::sVector& i_targetPosition,
		const Math::cQuaternion& i_targetOrientation)
	{
		Math::cMatrix_transformation transformation(i_targetOrientation, Math::sVector());
		Math::sVector up = transformation.GetUpDirection();
		Math::sVector forward = -transformation.GetBackDirection();


		if (IsDampingDirectionsRelativeToTarget())
		{
			Math::sVector delta = i_targetPosition - i_cameraPosition;
			Math::sVector deltaZ = Math::sVector::Project(delta, forward);
			Math::sVector deltaY = Math::sVector::Project(delta, up);
			Math::sVector deltaX = delta - deltaZ - deltaY;

			Math::sVector deltaZDamped = Math::sVector::Damp(Math::sVector(), deltaZ, GetDamping().z, i_deltaSeconds);
			Math::sVector deltaYDamped = Math::sVector::Damp(Math::sVector(), deltaY, GetDamping().y, i_deltaSeconds);
			Math::sVector deltaXDamped = Math::sVector::Damp(Math::sVector(), deltaX, GetDamping().x, i_deltaSeconds);

			return i_cameraPosition + deltaXDamped + deltaYDamped + deltaZDamped;
		}
		else
		{
			return Math::sVector::Damp(i_cameraPosition, i_targetPosition, m_damping, i_deltaSeconds);
		}
	}
}

namespace eae6320::Serialization
{
	template <>
	void Serialize<Camera::cPositionLockToTarget>(const Camera::cPositionLockToTarget& i_toSerialize, char* io_buffer)
	{
		sSerializedPositionLockToTarget* serialized = reinterpret_cast<sSerializedPositionLockToTarget*>(io_buffer);

		serialized->damping = i_toSerialize.GetDamping();
		serialized->dampingDirectionsRelativeToTarget = i_toSerialize.IsDampingDirectionsRelativeToTarget() ? 1 : 0;
	}

	template <>
	void Deserialize<Camera::cPositionLockToTarget>(const char* i_buffer, Camera::cPositionLockToTarget& io_toDeserialize)
	{
		const sSerializedPositionLockToTarget* serialized = reinterpret_cast<const sSerializedPositionLockToTarget*>(i_buffer);

		io_toDeserialize.SetDamping(serialized->damping);
		io_toDeserialize.SetDampingDirectionsRelativeToTarget(serialized->dampingDirectionsRelativeToTarget != 0);
	}
}