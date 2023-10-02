// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SubmarineSession.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct ANTIQUATEDFUTURE_API FSubmarineSession
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int ListIndex = -1;
	UPROPERTY(BlueprintReadOnly)
	FString Name = "Unknown";
	UPROPERTY(BlueprintReadOnly)
	int NumPlayers = -1;
};
