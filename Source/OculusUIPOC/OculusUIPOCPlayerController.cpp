// Fill out your copyright notice in the Description page of Project Settings.

#include "OculusUIPOC.h"
#include "OculusUIPOCPlayerController.h"
#include "Engine.h"
#include "IHeadMountedDisplay.h"

void AOculusUIPOCPlayerController::UpdateRotation(float DeltaTime)
{
	// Calculate Delta to be applied on ViewRotation
	FRotator DeltaRot(RotationInput);

	FRotator NewControlRotation = GetControlRotation();
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("===================================================Control Rotation: ") + NewControlRotation.ToCompactString());

	if (PlayerCameraManager)
	{
		PlayerCameraManager->ProcessViewRotation(DeltaTime, NewControlRotation, DeltaRot);
	}

	SetControlRotation(NewControlRotation);

	if (!PlayerCameraManager || !PlayerCameraManager->bFollowHmdOrientation)
	{
		if (GEngine->HMDDevice.IsValid() && GEngine->HMDDevice->IsHeadTrackingAllowed())
		{
			FQuat HMDOrientation;
			FVector HMDPosition;

			// Disable bUpdateOnRT if using this method.
			if (!HMDInitialized) { // as soon as HMD is available reset the orientation
				GEngine->HMDDevice->ResetOrientationAndPosition(0.0);
				HMDInitialized = true;
			}
			GEngine->HMDDevice->GetCurrentOrientationAndPosition(HMDOrientation, HMDPosition);

			FRotator NewViewRotation = HMDOrientation.Rotator();
			//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("===================================================HMD Rotator: ") + NewViewRotation.ToCompactString());
			//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("===================================================HMD yaw:") + FString::SanitizeFloat(NewViewRotation.Yaw));

			// Only keep the yaw component from the controller.
			NewViewRotation.Yaw += NewControlRotation.Yaw;
			//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("===================================================Control Yaw:") + FString::SanitizeFloat(NewControlRotation.Yaw));
			//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("===================================================Total view yaw:") + FString::SanitizeFloat(NewViewRotation.Yaw));

			SetViewRotation(NewViewRotation);
		}
	}

	APawn* const P = GetPawnOrSpectator();
	if (P)
	{
		P->FaceRotation(NewControlRotation, DeltaTime);
	}
}

void AOculusUIPOCPlayerController::SetControlRotation(const FRotator& NewRotation)
{
	ControlRotation = NewRotation;

	// Anything that is overriding view rotation will need to 
	// call SetViewRotation() after SetControlRotation().
	SetViewRotation(NewRotation);

	if (RootComponent && RootComponent->bAbsoluteRotation)
	{
		RootComponent->SetWorldRotation(GetControlRotation());
	}
}

void AOculusUIPOCPlayerController::SetViewRotation(const FRotator& NewRotation)
{
	ViewRotation = NewRotation;
}

FRotator AOculusUIPOCPlayerController::GetViewRotation() const
{
	return ViewRotation;
}



