// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SungminWorldCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "OtherNetworkCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include <string>

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
	
	// 세션 아이디 지정 (지금은 랜덤값)
	SessionId = FMath::RandRange(0, 100);
	bIsSpawned = false;

	// 서버와 연결
	Socket.InitSocket();
	bIsConnected = Socket.Connect("127.0.0.1", 8000);
	if (bIsConnected)
	{				
		UE_LOG(LogClass, Log, TEXT("IOCP Server connect success!"));
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASungminWorldCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

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

void ASungminWorldCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bIsConnected)
	{		
		auto MyLocation = GetActorLocation();				
		int ci = Socket.SendMyLocation(SessionId, MyLocation);						
		
		UE_LOG(LogClass, Log, TEXT("asdasd : %d"), ci);

		TArray<AActor*> SpawnedCharacters;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOtherNetworkCharacter::StaticClass(), SpawnedCharacters);

		auto DummyLocation = MyLocation;
		DummyLocation.X += 100;
		DummyLocation.Y += 100;		

		for (auto Item : SpawnedCharacters)
		{
			auto Character = Cast<AOtherNetworkCharacter>(Item);
			Character->SetActorLocation(DummyLocation);						
		}
	}	
	
}

void ASungminWorldCharacter::BeginPlay()
{
	Super::BeginPlay();	

	FVector t;
	t.X = 69;
	t.Y = 0;
	t.Z = 0;
	// auto ci = Socket.SendMyLocation(99999, t);

	// UE_LOG(LogClass, Log, TEXT("XXXXXXX : %f"), ci->m[99999].x);

	UWorld* const world = GetWorld();

	if (world)
	{
		FVector SpawnLocation;
		SpawnLocation.X = -709;
		SpawnLocation.Y = -14;
		SpawnLocation.Z = 230;

		FRotator SpawnRotation(0.f, 0.f, 0.f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Instigator;
		SpawnParams.Name = "Dummy1";

		ACharacter* const SpawnCharacter = world->SpawnActor<ACharacter>(WhoToSpawn, SpawnLocation, SpawnRotation, SpawnParams);		
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
