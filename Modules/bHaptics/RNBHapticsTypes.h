//
//  RNBHapticsTypes.h
//  Rayne-BHaptics
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BHAPTICS_TYPES_H_
#define __RAYNE_BHAPTICS_TYPES_H_

#include "RNBHaptics.h"

namespace RN
{
	enum class BHapticsDevicePosition : uint8
	{
		Default = 0,
		Vest = 1, VestFront = 201, VestBack = 202,
		Head = 2,
		ForearmL = 3, ForearmR = 4,
		HandL = 5, HandR = 6,
		FootL = 7, FootR = 8
	};

	enum class BHapticsFeedbackMode : uint8
	{
		PATH_MODE,
		DOT_MODE
	};

	//Structure used to play individual motors on each device.
	struct BHapticsDotPoint
	{
		BHapticsDotPoint()
		{
			Index = 0;
			Intensity = 0;
		}

		BHapticsDotPoint(int32 _index, int32 _intensity)
		{
			Index = std::min(std::max(_index, 0), 19);
			Intensity = std::min(std::max(_intensity, 0), 100);
		}

		//Index of the motor to be activated.
		int32 Index;

		//Intensity of the vibration from 0 to 100
		int32 Intensity;
	};

	//Structure to allow for continuous haptic feedback anywhere on the device, interpolating which motors are played.
	struct BHapticsPathPoint
	{
		BHapticsPathPoint()
		{
			X = 0;
			Y = 0;
			Intensity = 0;
			MotorCount = 3;
		}

		//X-value position on the device from 0.0 to 1.0
		float X;

		//Y-value position on the device from 0.0 to 1.0
		float Y;

		//Intensity of the vibration from 0 to 100
		int32 Intensity;

		//Number of motors activated when interpolating the point from 1 to 3.
		int32 MotorCount;

		BHapticsPathPoint(float _x, float _y, int32 _intensity, int32 _motorCount = 3)
		{
			int XRound = _x * 1000;
			int YRound = _y * 1000;
			X = XRound / 1000;
			Y = YRound / 1000;
			X = std::min(std::max(X, 0.0f), 1.0f);
			Y = std::min(std::max(Y, 0.0f), 1.0f);
			Intensity = std::min(std::max(_intensity, 0), 100);
			MotorCount = std::min(std::max(_motorCount, 1), 3);
		}
	};

	struct BHapticsFeedback
	{
		BHapticsDevicePosition Position;
		BHapticsFeedbackMode Mode;
		uint8 Values[20];
	};

	//Stores values used in rotating and transforming a feedback file on the haptic vest.
	struct BHapticsRotationOption
	{
		BHapticsRotationOption()
		{
			OffsetAngleX = 0;
			OffsetY = 0;
		}

		//Rotate the feedback file horizontally clockwise along the vest by the given angle in degrees.
		//A value of 180 will flip the feedback to the other side of the device.
		float OffsetAngleX = 0;

		//Vertical offset of the transformed feedback file, with negative values moving the feedback upwards.
		float OffsetY = 0;

		BHapticsRotationOption(float AngleX, float Y)
		{
			OffsetAngleX = AngleX;
			OffsetY = std::min(std::max(Y, -1.0f), 1.0f);
		}

		String *ToString()
		{
			return RNSTR("{ \"offsetAngleX\" : " << OffsetAngleX << ", \"offsetY\" : " << OffsetY << "}");
		}
	};

	//Stores the scaled values when altering a feedback file's intensity and duration.
	struct BHapticsScaleOption
	{
		BHapticsScaleOption()
		{
			Duration = 1.0;
			Intensity = 1.0;
		}

		//Multiplier to scale the intensity of the feedback, where 0.5 halves the intensity
		//and a value of 2.0 doubles it. Cannot be negative.
		float Intensity = 1.0;

		//Multiplier to scale the duration of the feedback, where 0.5 halves the duration
		//and a value of 2.0 doubles it. Cannot be negative.
		float Duration = 1.0;

		BHapticsScaleOption(float intensity, float duration)
		{
			Intensity = std::min(std::max(intensity, 0.01f), 100.0f);
			Duration = std::min(std::max(duration, 0.01f), 100.0f);
		}

		String *ToString()
		{
			return RNSTR("{ \"intensity\" : " << Intensity << ", \"duration\" : " << Duration << "}");
		}
	};

	class BHapticsDevice : public Object
	{
	public:
		BHapticsDevice();
		~BHapticsDevice();
		
		const String *deviceName;
		const String *address;
		BHapticsDevicePosition position;
		
		bool isConnected;
		bool isPaired;
		
	private:
		RNDeclareMetaAPI(BHapticsDevice, BHAPI)
	};

/*
	struct FRegisterRequest {
		const String *Key;

		TSharedPtr<FJsonObject> Project;

		void ToJsonObject(FJsonObject& JsonObject)
		{
			JsonObject.SetStringField("Key", Key);
			JsonObject.SetObjectField("Project", Project);
		}
	};

	USTRUCT()
	struct FHapticFrame {

		GENERATED_BODY()

		UPROPERTY()
		int DurationMillis;

		UPROPERTY()
			FString Position;

		UPROPERTY()
			TArray<FPathPoint> PathPoints;

		UPROPERTY()
			TArray<FDotPoint> DotPoints;
	};


	USTRUCT()
	struct FSubmitRequest {

		GENERATED_BODY()

			FSubmitRequest()
		{
			Type = "turnoff";
			Key = "";
			Parameters = TMap<FString, FString>();
		}

		FSubmitRequest(FString SubmissionKey, FString SubmissionType, FHapticFrame SubmissionFrame)
		{
			Type = SubmissionType;
			Key = SubmissionKey;
			Frame = SubmissionFrame;
			Parameters = TMap<FString, FString>();
		}

		UPROPERTY()
			FString Type;

		UPROPERTY()
			FString Key;

		UPROPERTY()
			TMap<FString, FString> Parameters;

		UPROPERTY()
			FHapticFrame Frame;
	};

	USTRUCT()
	struct FPlayerResponse {

		GENERATED_BODY()

			TArray<FString> RegisteredKeys;

		TArray<FString> ActiveKeys;

		int ConnectedDeviceCount;

		TArray<FString> ConnectedPositions;

		TArray<FHapticFeedback> Status;
	};

	USTRUCT()
	struct FPlayerRequest {

		GENERATED_BODY()

			UPROPERTY()
			TArray<FRegisterRequest> Register;

		UPROPERTY()
			TArray<FSubmitRequest> Submit;
	};
*/
}

#endif //__RAYNE_BHAPTICS_TYPES_H_
