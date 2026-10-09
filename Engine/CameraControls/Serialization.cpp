#include "Serialization.h"
#include <Engine/Asserts/Asserts.h>
#include <Engine/ScopeGuard/cScopeGuard.h>
#include <Engine/Logging/Logging.h>
#include <Engine/Math/Functions.h>
#include <Engine/Platform/Platform.h>
#include <Engine/Results/Results.h>
#include "Position/cPositionComposer.h"
#include "Position/cPositionFollow.h"
#include "Position/cPositionLockToTarget.h"
#include "Position/cPositionUnmodified.h"
#include "Position/iPositionStrategy.h"
#include "Orientation/cOrientationComposer.h"
#include "Orientation/cOrientationHardLookAt.h"
#include "Orientation/cOrientationLockToTarget.h"
#include "Orientation/cOrientationUnmodified.h"
#include "Orientation/iOrientationStrategy.h"
#include "cTrackingCamera.h"

namespace eae6320::Serialization
{
	std::unique_ptr<Camera::iPositionStrategy> DeserializePositionStrategyByID(int16_t i_classID, const char* i_buffer)
	{
		using namespace Camera;

		switch (i_classID)
		{
		case GetClassID<cPositionComposer>():
			return ConstructAndDeserialize<cPositionComposer>(i_buffer);
		case GetClassID<cPositionFollow>():
			return ConstructAndDeserialize<cPositionFollow>(i_buffer);
		case GetClassID<cPositionLockToTarget>():
			return ConstructAndDeserialize<cPositionLockToTarget>(i_buffer);
		case GetClassID<cPositionUnmodified>():
			return ConstructAndDeserialize<cPositionUnmodified>(i_buffer);
		default:
			EAE6320_ASSERTF(false, "Class ID %d does not correspond to a position strategy", i_classID);
			return std::unique_ptr<iPositionStrategy>();
		}
	}

	std::unique_ptr<Camera::iOrientationStrategy> DeserializeOrientationStrategyByID(int16_t i_classID, const char* i_buffer)
	{
		using namespace Camera;

		switch (i_classID)
		{
		case GetClassID<cOrientationComposer>():
			return ConstructAndDeserialize<cOrientationComposer>(i_buffer);
		case GetClassID<cOrientationHardLookAt>():
			return ConstructAndDeserialize<cOrientationHardLookAt>(i_buffer);
		case GetClassID<cOrientationLockToTarget>():
			return ConstructAndDeserialize<cOrientationLockToTarget>(i_buffer);
		case GetClassID<cOrientationUnmodified>():
			return ConstructAndDeserialize<cOrientationUnmodified>(i_buffer);
		default:
			EAE6320_ASSERTF(false, "Class ID %d does not correspond to a orientation strategy", i_classID);
			return std::unique_ptr<iOrientationStrategy>();
		}
	}

	cResult LoadCameraFromFile(const char* i_infile, Camera::cTrackingCamera& o_trackingCamera)
	{
		auto result = Results::Success;

		sCameraFileMetadata* metadata;

		Platform::sDataFromFile fileData;

		std::string loadFileErrorMessage;
		result = Platform::LoadBinaryFile(i_infile, fileData, &loadFileErrorMessage);
		if (!result)
		{
			//EAE6320_ASSERTF(false, "Binary file %s could not be loaded: %s", i_infile, loadFileErrorMessage.c_str());
			Logging::OutputError("Failed to load binary file %s: %s", i_infile, loadFileErrorMessage.c_str());
			return result;
		}
		// (fileData frees its memory in its destructor when this function returns,
		// which is fine because everything that is needed is copied into the camera and new strategy objects)

		// The metadata can only be read if the file is at least big enough to contain it
		if (fileData.size < sizeof(sCameraFileMetadata))
		{
			Logging::OutputError("File %s is too small (%u bytes) to be a camera file", i_infile, static_cast<unsigned int>(fileData.size));
			result = Results::InvalidFile;
			return result;
		}

		metadata = reinterpret_cast<sCameraFileMetadata*>(fileData.data);

		if (metadata->magicNumber != sCameraFileMetadata::DefaultMagicNumber)
		{
			EAE6320_ASSERTF(false, "File %s magic number %x does not match expected magic number %x",
				i_infile, metadata->magicNumber, sCameraFileMetadata::DefaultMagicNumber);
			Logging::OutputError("File %s is probably corrputed (magic number mismatch (%x, expected %x))",
				i_infile, metadata->magicNumber, sCameraFileMetadata::DefaultMagicNumber);
			result = Results::InvalidFile;
			return result;
		}

		size_t expectedFileSize = sizeof(sCameraFileMetadata) + metadata->cameraDataSize
			+ metadata->positionStrategySize + metadata->orientationStrategySize;
		if (fileData.size < expectedFileSize)
		{
			EAE6320_ASSERTF(false, "File %s size %d does not match expected file size %d",
				i_infile, fileData.size, expectedFileSize);
			Logging::OutputError("File %s size mismatches expected size from metadata (%d, expected %d))",
				i_infile, fileData.size, expectedFileSize);
			// (this used to return without setting a failure, so a truncated file was reported as loaded successfully)
			result = Results::InvalidFile;
			return result;
		}

		uintptr_t fileDataPtrNum = reinterpret_cast<uintptr_t>(fileData.data);
		uintptr_t cameraDataPtrNum = fileDataPtrNum + sizeof(sCameraFileMetadata);
		uintptr_t positionStrategyDataPtrNum = cameraDataPtrNum + metadata->cameraDataSize;
		uintptr_t orientationStrategyDataPtrNum = positionStrategyDataPtrNum + metadata->positionStrategySize;

		const char* cameraDataPtr = reinterpret_cast<const char*>(cameraDataPtrNum);
		const char* positionStrategyDataPtr = reinterpret_cast<const char*>(positionStrategyDataPtrNum);
		const char* orientationStrategyDataPtr = reinterpret_cast<const char*>(orientationStrategyDataPtrNum);

#ifdef _DEBUG
		uint32_t checksum = ComputeChecksum(*metadata, cameraDataPtr, positionStrategyDataPtr, orientationStrategyDataPtr);
		if (checksum != metadata->checksum)
		{
			//EAE6320_FAIL("File %s checksum %x does not match expected checksum %x",
				//i_infile, metadata->checksum, checksum);
			Logging::OutputError("File %s is probably corrputed (checksum mismatch (%x, expected %x))",
				i_infile, metadata->checksum, checksum);
			result = Results::InvalidFile;
			return result;
		}
#endif

		Deserialize(cameraDataPtr, o_trackingCamera);

		std::unique_ptr<Camera::iPositionStrategy> positionStrategy
			= DeserializePositionStrategyByID(metadata->positionStrategyClassId, positionStrategyDataPtr);
		std::unique_ptr<Camera::iOrientationStrategy> orientationStrategy
			= DeserializeOrientationStrategyByID(metadata->orientationStrategyClassId, orientationStrategyDataPtr);

		if (!positionStrategy || !orientationStrategy)
		{
			Logging::OutputError("File %s has an unknown position or orientation strategy", i_infile);
			result = Results::InvalidFile;
			return result;
		}

		o_trackingCamera.SetPositionStrategy(std::move(positionStrategy));
		o_trackingCamera.SetOrientationStrategy(std::move(orientationStrategy));

		return result;
	}

	uint32_t ComputeChecksum(const sCameraFileMetadata& i_metadata,
		const void* i_cameraData, const void* i_positionStrategyData, const void* i_orientationStrategyData)
	{
		uint32_t cameraDataHash = Math::HashCRC32(i_cameraData, i_metadata.cameraDataSize);
		uint32_t positionStrategyHash = Math::HashCRC32(i_positionStrategyData, i_metadata.positionStrategySize);
		uint32_t orientationStrategyHash = Math::HashCRC32(i_orientationStrategyData, i_metadata.orientationStrategySize);

		constexpr size_t checksumAndMagicNumberSize = sizeof(uint32_t) + sizeof(uint32_t);

		const void* metadataData = reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(&i_metadata) + checksumAndMagicNumberSize);

		uint32_t metadataHash = Math::HashCRC32(metadataData, sizeof(sCameraFileMetadata) - checksumAndMagicNumberSize);

		return metadataHash ^ cameraDataHash ^ positionStrategyHash ^ orientationStrategyHash;
	}
}