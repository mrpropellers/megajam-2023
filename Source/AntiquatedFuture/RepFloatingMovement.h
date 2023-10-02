#pragma once

#include "RepFloatingMovement.generated.h"

USTRUCT()
struct FRepFloatingMovement
{
	FRepFloatingMovement(float Timestamp, const FVector_NetQuantize& Position, const FQuat& Orientation,
		const FVector_NetQuantize& Velocity)
		: Timestamp(Timestamp),
		Position(Position),
		Orientation(Orientation),
		Velocity(Velocity)
	{
	}

	GENERATED_USTRUCT_BODY()

	FRepFloatingMovement()
	{
		Timestamp = 0;
		Position = FVector::ZeroVector;
		Orientation = FQuat::Identity;
		Velocity = FVector::ZeroVector;
	}

	UPROPERTY()
	float Timestamp;

	UPROPERTY()
	FVector_NetQuantize Position;

	UPROPERTY()
	FQuat Orientation;

	UPROPERTY()
	FVector_NetQuantize Velocity;
};
