// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SungminWorldCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Classes/Components/SphereComponent.h"
#include "OtherNetworkCharacter.h"
#include "Engine/World.h"
#include "SungminWorldGameMode.h"


//////////////////////////////////////////////////////////////////////////
// ASungminWorldCharacter

ASungminWorldCharacter::ASungminWorldCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)		

	// 체력, 에너지, 기분 기정
	HealthValue = 0.5f;
	EnergyValue = 0.5f;
	MoodValue = 0.5f;	

	bIsAlive = true;
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASungminWorldCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	// PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASungminWorldCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Hit", IE_Pressed, this, &ASungminWorldCharacter::HitOtherCharacter);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASungminWorldCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASungminWorldCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ASungminWorldCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ASungminWorldCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ASungminWorldCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ASungminWorldCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ASungminWorldCharacter::OnResetVR);
}

void ASungminWorldCharacter::Jump()
{
	ACharacter::Jump();

	if (!GetCharacterMovement()->IsFalling())
	{
		if (EnergyValue > 0)
		{
			EnergyValue -= 0.05f;
		}
		else
		{
			EnergyValue = 0;
		}
		
	}	
}

void ASungminWorldCharacter::UpdateHealth(float HealthChange)
{
	HealthValue += HealthChange;
}

float ASungminWorldCharacter::GetHealth()
{
	return HealthValue;
}

bool ASungminWorldCharacter::IsAlive()
{
	return bIsAlive;
}

void ASungminWorldCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ASungminWorldCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void ASungminWorldCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void ASungminWorldCharacter::HitOtherCharacter()
{
	UE_LOG(LogClass, Log, TEXT("Hit called"));	
	TArray<UActorComponent*> Comps;
	TArray<AActor*> NearCharacters;
	this->GetComponents(Comps);
	for (auto Comp : Comps)
	{
		USphereComponent* HitSphere = Cast<USphereComponent>(Comp);
		if (HitSphere)
		{
			HitSphere->GetOverlappingActors(NearCharacters);
			break;
		}
	}

	for (auto Character : NearCharacters)
	{
		AOtherNetworkCharacter * OtherCharacter = Cast<AOtherNetworkCharacter>(Character);
		if (OtherCharacter)
		{	
			ASungminWorldGameMode* GameMode = (ASungminWorldGameMode*)GetWorld()->GetAuthGameMode();

			GameMode->HitCharacter(FCString::Atoi(*OtherCharacter->GetName()), OtherCharacter);			
		}
	}	
}

void ASungminWorldCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ASungminWorldCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ASungminWorldCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ASungminWorldCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
