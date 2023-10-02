// Fill out your copyright notice in the Description page of Project Settings.


#include "SubmarineProjectile.h"

#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
ASubmarineProjectile::ASubmarineProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	AActor::SetReplicateMovement(true);
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Projectile Mesh"));
	Mesh->SetupAttachment(RootComponent);
	SetRootComponent(Mesh);

	Collider = CreateDefaultSubobject<USphereComponent>(TEXT("Collider"));
	Collider->SetSphereRadius(0.05);
	Collider->OnComponentHit.AddDynamic(this, &ASubmarineProjectile::OnCollision);
	Collider->SetupAttachment(RootComponent);
	
	InFlightParticles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("In Flight Particles"));
	InFlightParticles->SetupAttachment(RootComponent);

	HitParticles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("On Hit Particles"));
	HitParticles->SetupAttachment(RootComponent);
	
	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = 1000.f;
	Movement->MaxSpeed = Movement->InitialSpeed * 10.f;
	Movement->ProjectileGravityScale = 0.05f;
}

// Called when the game starts or when spawned
void ASubmarineProjectile::BeginPlay()
{
	Super::BeginPlay();
	//Movement->SetInterpolatedComponent(Mesh);
}

// Called every frame
void ASubmarineProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASubmarineProjectile::OnCollision(UPrimitiveComponent* HitComponentSelf, AActor* OtherActor,
	UPrimitiveComponent* HitComponentOther, FVector NormalImpulse, const FHitResult& HIt)
{
	
}


