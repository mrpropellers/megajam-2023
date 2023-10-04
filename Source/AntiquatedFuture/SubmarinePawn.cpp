#include "SubmarinePawn.h"
#include "SubmarinePlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "SubmarineWeapons.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ASubmarinePawn::ASubmarinePawn()
{
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	SetRootComponent(Sphere);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Sphere);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Floating Pawn Movement"));

	MoveSensitivity = 1.0f;
	RotateSensitivity = 50.0f;
	JuggernautDashCooldown = 4.f;
	JuggernautDashDuration = 1.5f;

	bReplicates = true;
	// We want to manually and explicitly update movement - so don't let any auto-replication happen
	AActor::SetReplicateMovement(false);

	RollCooldown = 0.2f;
	DashCooldown = 2.f;
	ServerMovement = FRepFloatingMovement();

	bWeaponsAreInitialized = false;
	//bHasReceivedMovement = false;
}

FString ASubmarinePawn::ToString(ENetMode NetMode) const
{
	switch (NetMode)
	{
		case NM_Standalone:
			return FString("Standalone");
		case NM_DedicatedServer:
			return FString("DedicatedServer");
		case NM_ListenServer:
			return FString("ListenServer");
		case NM_Client:
			return FString("Client");
		case NM_MAX:
			return FString("Max");
		default:
			return FString("UNKNOWN");
	}
}
void ASubmarinePawn::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ASubmarinePawn, bHasReceivedMovement);
	DOREPLIFETIME_CONDITION(ASubmarinePawn, ServerMovement, COND_SkipOwner);
	// We'll manually invoke the local state changes - we trust our clients (: )
	DOREPLIFETIME(ASubmarinePawn, bIsJuggernaut);
}

// Called when the game starts
void ASubmarinePawn::BeginPlay()
{
	Super::BeginPlay();

	Weapons = TArray<USubmarineWeapon*>();
	if (!bWeaponsAreInitialized)
	{
		InitializeWeapons();
	}
	GameState = GetWorld()->GetGameState();
	
	const auto LocalRole = GetLocalRole();
	const auto LocalRoleString = UEnum::GetValueAsString(GetLocalRole());
	const auto RemoteRoleString = UEnum::GetValueAsString(GetRemoteRole());
	const auto NetMode = GetWorld()->GetNetMode();
	const auto NetModeString = ToString(NetMode);

	ServerMovement = FRepFloatingMovement();
	ServerMovement.Timestamp = -ExtrapolationLimit;
	
	NetDebugName = FString::Printf(TEXT("[%s | %s | %s]"),
		*GetName(), *LocalRoleString, *NetModeString);
	
	UE_LOG(LogTemp, Warning, TEXT("Spawned: %s || Is Authority: %s || Is Locally Controlled: %s"),
		*NetDebugName,
		IsAuthority() ? *FString("Yes") : *FString("No"),
		IsLocallyControlled() ? *FString("Yes") : *FString("No"));

	CurrentDashCooldown = DashCooldown;
	TimeLastDashFinished = UGameplayStatics::GetTimeSeconds(GetWorld());
	LastTimestampApplied = -1.f;
	
}

bool ASubmarinePawn::IsLocalControl() const
{
	return IsLocallyControlled();
}

bool ASubmarinePawn::IsAuthority() const
{
	// ... is it this simple?
	return GetLocalRole() == ROLE_Authority;
}


void ASubmarinePawn::CalculateAndSendUpdates(float DeltaTime)
{
	if (!IsRolling())
	{
		const FRotator CurrentRot = GetActorRotation();
		float TargetRoll = FMath::RoundToInt(CurrentRot.Roll / 90.0f) * 90.0f;
		const float NewRoll = FMath::Lerp(CurrentRot.Roll, TargetRoll, DeltaTime * CorrectiveSpeed);
		SetActorRotation(FRotator(CurrentRot.Pitch, CurrentRot.Yaw, NewRoll));
	}
	//UE_LOG(LogTemp, Log, TEXT("%s sending Movement updates"), *NetDebugName);
	// TODO: Send delta frames when we haven't moved too much since last update
	const auto Transform = RootComponent->GetComponentTransform();
	ServerSetTransform(
		GameState->GetServerWorldTimeSeconds(),
		Transform.GetLocation(),
		Transform.GetRotation(),
		GetVelocity()
		);
}

float ASubmarinePawn::Now() const
{
	return GameState->GetServerWorldTimeSeconds();
}

// void ASubmarinePawn::UpdateServerMovement(const float Timestamp)
// {
// 	bHasReceivedMovement = true;
// }

void ASubmarinePawn::OnRep_Move()
{
	if (IsLocallyControlled())
	{
		UE_LOG(LogTemp, Error, TEXT("We should not be simulating movement if locally controlled!"))
		return;
	}
	//UE_LOG(LogTemp, Log, TEXT("%s applying update from Server Movement"), *NetDebugName);
	// const auto ServerDeltaTime = Now() - ServerMovement.Timestamp;
	// const auto Displacement = ServerMovement.Velocity * ServerDeltaTime;
	LastTimestampApplied = ServerMovement.Timestamp;
	RootComponent->SetWorldLocation(ServerMovement.Position);
	RootComponent->SetWorldRotation(ServerMovement.Orientation);
	GetMovementComponent()->Velocity = ServerMovement.Velocity;
}

void ASubmarinePawn::ServerSetTransform_Implementation(
	const float Timestamp,
	const FVector_NetQuantize Position,
	const FQuat Rotation,
	const FVector_NetQuantize NewVelocity
	)
{
	//UE_LOG(LogTemp, Log, TEXT("%s executing RPC"), *NetDebugName);
	const auto CurrentTime = Now();
	float DeltaTime = CurrentTime - Timestamp;
	if (DeltaTime < -FLT_EPSILON)
	{
		// This seems to happen quite a lot when clients/servers are first connected
		// UE_LOG(LogTemp, Warning, TEXT("Received an update from %f in the future? Assuming 0 DeltaTime"),
		// 	-DeltaTime)
		DeltaTime = 0.f;
	}
	
	//const FVector_NetQuantize CurrentPosition = Position + (NewVelocity * DeltaTime);
	
	// Check if we are ALSO a local player in addition to a server, only move self if not locally controlled
	if (!IsLocallyControlled())
	{
		// TODO: We could extrapolate forward in ServerTime with a sweep to detect collision
		RootComponent->SetWorldLocation(Position);
		// TODO: Really would like an angular velocity here too...
		RootComponent->SetWorldRotation(Rotation);
		GetMovementComponent()->Velocity = NewVelocity;
	}

	// Not sure which of these two methods guarantees Replication... both seem to break pretty regularly
	//ServerMovement = FRepFloatingMovement(CurrentTime, CurrentPosition, Rotation, NewVelocity);
	ServerMovement.Timestamp = Timestamp;
	ServerMovement.Position = Position;
	ServerMovement.Orientation = Rotation;
	ServerMovement.Velocity = NewVelocity;
}


void ASubmarinePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bWeaponsAreInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("Weapons failed to initialize in SetupPlayerInputComponent or BeginPlay."
								" Doing it now."))
	}

	if (IsLocallyControlled())
	{
		CalculateAndSendUpdates(DeltaTime);
	}
	else if (Now() - ServerMovement.Timestamp < ExtrapolationLimit)
	{
		ApplyLastUpdate();
	}
	else
	{
		// Might want to re-enable this once we're positive that things are stable...
		//UE_LOG(LogTemp, Log, TEXT("%s doing nothing while we wait for a fresh movement update..."), *NetDebugName)
	}

}

void ASubmarinePawn::ApplyLastUpdate()
{
	if (IsLocallyControlled())
	{
		UE_LOG(LogTemp, Error, TEXT("We should not be simulating movement if locally controlled!"))
		return;
	}
	//UE_LOG(LogTemp, Log, TEXT("%s applying update from Server Movement"), *NetDebugName);
	float ServerDeltaTime = Now() - ServerMovement.Timestamp;
	if (ServerDeltaTime < 0)
	{
		ServerDeltaTime = 0;
	}
	const auto Displacement = ServerMovement.Velocity * ServerDeltaTime;
	LastTimestampApplied = ServerMovement.Timestamp;
	RootComponent->SetWorldLocation(ServerMovement.Position + Displacement);
	RootComponent->SetWorldRotation(ServerMovement.Orientation);
	GetMovementComponent()->Velocity = ServerMovement.Velocity;
}


void ASubmarinePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	const auto SubmarineController = Cast<ASubmarinePlayerController>(Controller);
	if (SubmarineController == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Player Controller not set to ASubmarinePlayerController"))
	}
	else if(APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(SubmarineController->PawnMappingContext, 0);
		}
	}

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	const ASubmarinePlayerController* SubmarinePlayerController = Cast<ASubmarinePlayerController>(Controller);
	check(EnhancedInput && SubmarinePlayerController);
	EnhancedInput->BindAction(SubmarinePlayerController->MoveAction, ETriggerEvent::Triggered, this, &ASubmarinePawn::Move);
	EnhancedInput->BindAction(SubmarinePlayerController->RotateAction, ETriggerEvent::Triggered, this, &ASubmarinePawn::Rotate);
	EnhancedInput->BindAction(SubmarinePlayerController->DashAction, ETriggerEvent::Triggered, this, &ASubmarinePawn::Dash);
	

	if (!bWeaponsAreInitialized)
	{
		InitializeWeapons();
	}
	for (const auto& Weapon: Weapons)
	{
		EnhancedInput->BindAction(Weapon->ShootAction, ETriggerEvent::Triggered, Weapon, &USubmarineWeapon::HandleShootAction);
	}
}

void ASubmarinePawn::InitializeWeapons()
{
	GetComponents<USubmarineWeapon>(Weapons);
	if (Weapons.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Found no Weapons! Can't initialize Weapon actions..."));
		return;
	}
	for (const auto& Weapon: Weapons)
	{
		Weapon->BindToPlayer(Camera);
		Weapon->SetInstigator(this);
	}
	bWeaponsAreInitialized = true;
}



void ASubmarinePawn::Move(const FInputActionValue& ActionValue)
{
	if (bIsChargingSuperDash || (bIsJuggernaut && bIsDashing))
	{
		return;
	}
	LastKnownStrafeInput = ActionValue.Get<FInputActionValue::Axis3D>();
	AddMovementInput(GetActorRotation().RotateVector(LastKnownStrafeInput), MoveSensitivity);
}


void ASubmarinePawn::Rotate(const FInputActionValue& ActionValue)
{
	// Ignore rotate if we're in the middle of a Juggernaut dash
	if (bIsJuggernaut && bIsDashing)
	{
		return;
	}
	FRotator Input(ActionValue[0], ActionValue[1], ActionValue[2]);
	// TODO: We should really just be clamping sensitivity to a predetermined max
	const float Sensitivity = bIsChargingSuperDash ? RotateSensitivity / 4.f : RotateSensitivity;
	Input *= GetWorld()->GetDeltaSeconds() * Sensitivity;
	AddActorLocalRotation(Input);

	if (!FMath::IsNearlyZero(Input.Roll))
	{
		TimeLastRolled = GetWorld()->GetTimeSeconds();
	}
	
}

void ASubmarinePawn::SetAsJuggernaut()
{
	bIsJuggernaut = true;
	if (IsLocallyControlled())
	{
		OnRep_Juggernaut();
	}
}

void ASubmarinePawn::OnRep_Juggernaut()
{
	if (bIsJuggernaut)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s is now the Juggernaut!"), *GetActorNameOrLabel())
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Called OnRep_Juggernaut even though we're not!"))
		return;
	}
	CurrentDashCooldown = JuggernautDashCooldown;
	for (const auto& Weapon: Weapons)
	{
		Weapon->OnBecomeJuggernaut();
	}
}

bool ASubmarinePawn::CanDash()
{
	return !(bIsDashing || bIsChargingSuperDash) &&
		UGameplayStatics::GetTimeSeconds(GetWorld()) - TimeLastDashFinished > CurrentDashCooldown;
}

bool ASubmarinePawn::IsRolling()
{
	return GetWorld()->GetTimeSeconds() - TimeLastRolled < RollCooldown;
}


void ASubmarinePawn::Dash(const FInputActionValue& ActionValue)
{
	if (bIsJuggernaut)
	{
		JuggernautDash(ActionValue);
		return;
	}
	// Check whether this action was Started or Ended (we trigger on Start for normal dash)
	if (!ActionValue.Get<bool>())
	{
		return;
	}
	if (!CanDash())
	{
		UE_LOG(LogTemp, Log, TEXT("In Dash cooldown; can't dash."))
		return;
	}
	
	bIsDashing = true;

	Movement->Velocity += GetActorRotation().RotateVector(LastKnownStrafeInput * DashSpeed);
	DefaultDeceleration = Movement->Deceleration;
	Movement->Deceleration = DashSlowdown;

	DefaultAcceleration = Movement->Acceleration;
	DefaultMaxSpeed = Movement->MaxSpeed;
	Movement->MaxSpeed = DashSpeed;
	Movement->Acceleration = DashSpeed * 10.f;

	GetWorld()->GetTimerManager().SetTimer(DashEndTimerHandle, this, &ASubmarinePawn::EndDash, DashDuration, false);
	// NOTE: This will only broadcast locally since the Dash logic only executes locally
	DashStarted.Broadcast();
}

void ASubmarinePawn::JuggernautDash(const FInputActionValue& ActionValue)
{
	const bool bIsButtonPressed = ActionValue.Get<bool>();
	if (bIsButtonPressed && CanDash())
	{
		UE_LOG(LogTemp, Log, TEXT("Charging up Juggernaut Dash"));
		bIsChargingSuperDash = true;
		TimeJuggernautDashChargingStarted = GetWorld()->GetTimeSeconds();
	}
	else if (!bIsButtonPressed) 
	{
		if (CanDash() || !bIsChargingSuperDash)
		{
			UE_LOG(LogTemp, Warning, TEXT("Got a Dash Completed event but Pawn can still dash???"))
			return;
		}
		const float ValidChargeThreshold = JuggernautDashChargeDuration * 0.25f;
		const float ChargeWaitThreshold = JuggernautDashChargeDuration * 0.5f;
		const float TimeSpentCharging = GetWorld()->GetTimeSeconds() - TimeJuggernautDashChargingStarted;
		if (TimeSpentCharging < ValidChargeThreshold)
		{
			UE_LOG(LogTemp, Log, TEXT("Juggernaut Dash cancelled"));
			bIsDashing = false;
			bIsChargingSuperDash = false;
		}
		else if (TimeSpentCharging < ChargeWaitThreshold)
		{
			const float RemainingWaitTime = ChargeWaitThreshold - TimeSpentCharging;
			UE_LOG(LogTemp, Log, TEXT("Juggernaut Dash locked in, will execute in %f seconds"), RemainingWaitTime);
			GetWorld()->GetTimerManager().SetTimer(
				DashEndTimerHandle, this, &ASubmarinePawn::DoJuggernautDash, RemainingWaitTime, false);
		}
		else
		{
			DoJuggernautDash();
		}
	}
}

void ASubmarinePawn::DoJuggernautDash()
{
	UE_LOG(LogTemp, Log, TEXT("Doing Juggernaut Dash"));
	bIsChargingSuperDash = false;
	bIsDashing = true;

	const float ChargeRatio = (GetWorld()->GetTimeSeconds() - TimeJuggernautDashChargingStarted) / JuggernautDashChargeDuration;

	DefaultAcceleration = Movement->Acceleration;
	DefaultMaxSpeed = Movement->MaxSpeed;
	Movement->MaxSpeed = JuggernautDashSpeed;
	Movement->Acceleration = JuggernautDashSpeed * 10.f;

	Movement->Velocity = GetActorForwardVector() * JuggernautDashSpeed;
	DefaultDeceleration = Movement->Deceleration;
	Movement->Deceleration = 0.f;


	GetWorld()->GetTimerManager().SetTimer(
		DashEndTimerHandle, this, &ASubmarinePawn::EndDash, ChargeRatio * JuggernautDashDuration, false);
	// NOTE: This will only broadcast locally since the Dash logic only executes locally
	DashStarted.Broadcast();
	
}


void ASubmarinePawn::EndDash()
{
	bIsDashing = false;
	Movement->Deceleration = DefaultDeceleration;
	Movement->Acceleration = DefaultAcceleration;
	Movement->MaxSpeed = DefaultMaxSpeed;
	TimeLastDashFinished = UGameplayStatics::GetTimeSeconds(GetWorld());
}
