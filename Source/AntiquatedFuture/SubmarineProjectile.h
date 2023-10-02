// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SubmarineProjectile.generated.h"

class UNiagaraComponent;

UCLASS(Abstract)
class ANTIQUATEDFUTURE_API ASubmarineProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASubmarineProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnCollision(
		UPrimitiveComponent* HitComponentSelf,
		AActor* OtherActor,
		UPrimitiveComponent* HitComponentOther,
		FVector NormalImpulse,
		const FHitResult& HIt);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class USphereComponent* Collider;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UNiagaraComponent* InFlightParticles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UNiagaraComponent* HitParticles;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BaseDamage;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UProjectileMovementComponent* Movement;
};
