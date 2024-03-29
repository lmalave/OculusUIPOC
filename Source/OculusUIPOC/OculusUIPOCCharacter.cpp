// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "OculusUIPOC.h"
#include "Animation/AnimInstance.h"
#include "CoherentUIComponent.h"
#include "Engine.h"
#include "IHeadMountedDisplay.h"
#include "Leap.h"
#include <math.h> 
#include "OculusUIPOCCharacter.h"
#include "OculusUIPOCPlayerController.h"
#include "OculusUIPOCProjectile.h"
#include "Runtime/Core/Public/Misc/DateTime.h"
#include <string>
#include "UISurfaceActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AOculusUIPOCCharacter

AOculusUIPOCCharacter::AOculusUIPOCCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->AttachParent = GetCapsuleComponent();
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 30.0f, 10.0f);

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	Mesh1P->AttachParent = FirstPersonCameraComponent;
	Mesh1P->RelativeLocation = FVector(0.f, 0.f, -150.f);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P are set in the
	// derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	// enable tick
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	RaytraceInputEnable = true;
	LeapEnable = true;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AOculusUIPOCCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	
	//InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AOculusUIPOCCharacter::TouchStarted);
	if( EnableTouchscreenMovement(InputComponent) == false )
	{
		InputComponent->BindAction("Fire", IE_Pressed, this, &AOculusUIPOCCharacter::OnFire);
	}
	
	InputComponent->BindAxis("MoveForward", this, &AOculusUIPOCCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AOculusUIPOCCharacter::MoveRight);
	
	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AOculusUIPOCCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AOculusUIPOCCharacter::LookUpAtRate);
}

void AOculusUIPOCCharacter::OnFire()
{ 
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		const FRotator SpawnRotation = GetControlRotation();
		// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
		const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(GunOffset);

		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			// spawn the projectile at the muzzle
			World->SpawnActor<AOculusUIPOCProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if(FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if(AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}

}

void AOculusUIPOCCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if( TouchItem.bIsPressed == true )
	{
		return;
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AOculusUIPOCCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	if( ( FingerIndex == TouchItem.FingerIndex ) && (TouchItem.bMoved == false) )
	{
		OnFire();
	}
	TouchItem.bIsPressed = false;
}

void AOculusUIPOCCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if ((TouchItem.bIsPressed == true) && ( TouchItem.FingerIndex==FingerIndex))
	{
		if (TouchItem.bIsPressed)
		{
			if (GetWorld() != nullptr)
			{
				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
				if (ViewportClient != nullptr)
				{
					FVector MoveDelta = Location - TouchItem.Location;
					FVector2D ScreenSize;
					ViewportClient->GetViewportSize(ScreenSize);
					FVector2D ScaledDelta = FVector2D( MoveDelta.X, MoveDelta.Y) / ScreenSize;									
					if (ScaledDelta.X != 0.0f)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.X * BaseTurnRate;
						AddControllerYawInput(Value);
					}
					if (ScaledDelta.Y != 0.0f)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.Y* BaseTurnRate;
						AddControllerPitchInput(Value);
					}
					TouchItem.Location = Location;
				}
				TouchItem.Location = Location;
			}
		}
	}
}

void AOculusUIPOCCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AOculusUIPOCCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AOculusUIPOCCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AOculusUIPOCCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AOculusUIPOCCharacter::EnableTouchscreenMovement(class UInputComponent* InputComponent)
{
	bool bResult = false;
	if(FPlatformMisc::GetUseVirtualJoysticks() || GetDefault<UInputSettings>()->bUseMouseForTouch )
	{
		bResult = true;
		InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AOculusUIPOCCharacter::BeginTouch);
		InputComponent->BindTouch(EInputEvent::IE_Released, this, &AOculusUIPOCCharacter::EndTouch);
		InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AOculusUIPOCCharacter::TouchUpdate);
	}
	return bResult;
}

void AOculusUIPOCCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UISurfaceRaytraceHandler->HandleRaytrace();
	HandleLeap();

}

void AOculusUIPOCCharacter::HandleLeap() {

	if (LeapEnable == true) {
		LeapInput->UpdateHandLocations();
		// first handle movement
		FVector MovementHandPalmLocation_CharacterSpace = LeapInput->GetLeftPalmLocation_CharacterSpace();
		FVector MovementHandFingerLocation_CharacterSpace = LeapInput->GetLeftFingerLocation_CharacterSpace();
		VirtualJoystick->CalculateMovementFromHandLocation(MovementHandPalmLocation_CharacterSpace, MovementHandFingerLocation_CharacterSpace);
		MoveForward(VirtualJoystick->GetForwardMovement());
		MoveRight(VirtualJoystick->GetRightMovement());
		TurnAtRate(VirtualJoystick->GetTurnRate());

		// next handle UI input
		ActionHandPalmLocation = LeapInput->GetRightPalmLocation_WorldSpace();
		ActionHandFingerLocation = LeapInput->GetRightFingerLocation_WorldSpace();
		AUISurfaceActor* SelectedUISurface = UISurfaceRaytraceHandler->GetSelectedUISurfaceActor();
		if (SelectedUISurface && SelectedUISurface->CoherentUIComponent && SelectedUISurface->CoherentUIComponent->GetView()) {
			SelectedUISurface->HandleVirtualTouchInput(ActionHandPalmLocation, ActionHandFingerLocation);
		}
	}
}

FRotator AOculusUIPOCCharacter::GetViewRotation() const
{
	if (AOculusUIPOCPlayerController* MYPC = Cast<AOculusUIPOCPlayerController>(Controller))
	{
		return MYPC->GetViewRotation();
	}
	else if (Role < ROLE_Authority)
	{
		// check if being spectated
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PlayerController = *Iterator;
			if (PlayerController && PlayerController->PlayerCameraManager->GetViewTargetPawn() == this)
			{
				return PlayerController->BlendedTargetViewRotation;
			}
		}
	}

	return GetActorRotation();
}

void AOculusUIPOCCharacter::ResetHMD()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("================================================Resetting HMD"));
	GEngine->HMDDevice->ResetOrientationAndPosition(0.0);
}

void AOculusUIPOCCharacter::BeginPlay() {
	LeapController = new Leap::Controller();
	LeapController->setPolicy(Leap::Controller::POLICY_OPTIMIZE_HMD);
	LeapInput = new LeapInputReader(LeapController, this);
	VirtualJoystick = new VirtualJoystick3D(this);
	UISurfaceRaytraceHandler = new UISurfaceRaytraceInputHandler(this, FirstPersonCameraComponent);
}
