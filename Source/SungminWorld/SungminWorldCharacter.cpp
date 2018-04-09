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
#include "Blueprint/UserWidget.h"
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

	// 체력, 에너지, 기분 기정
	HealthValue = 0.5f;
	EnergyValue = 0.5f;
	MoodValue = 0.5f;

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
	// PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASungminWorldCharacter::Jump);
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


AActor * ASungminWorldCharacter::FindActorBySessionId(TArray<AActor*> ActorArray, const int& SessionId)
{
	for (const auto& Actor : ActorArray)
	{
		if (stoi(*Actor->GetName()) == SessionId)
			return Actor;
	}
	return nullptr;
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
	// 서버와 연결되어 있을때만 동기화
	if (!bIsConnected)
		return;							
	// 플레이어의 위치를 가져옴	
	auto MyLocation = GetActorLocation();	
	auto MyRotation = GetActorRotation();

	cCharacter Character;
	Character.SessionId = SessionId;
	Character.X = MyLocation.X;
	Character.Y = MyLocation.Y;
	Character.Z = MyLocation.Z;
	Character.Yaw = MyRotation.Yaw;
	Character.Pitch = MyRotation.Pitch;
	Character.Roll = MyRotation.Roll;

	// 플레이어의 세션 아이디와 위치를 서버에게 보냄
	cCharactersInfo* ci = Socket.SyncCharacters(Character);
	if (ci == nullptr)
		return;

	UWorld* const world = GetWorld();
	if (world == nullptr)
		return;

	// 월드 내 OtherCharacter 액터 수집
	TArray<AActor*> SpawnedCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOtherNetworkCharacter::StaticClass(), SpawnedCharacters);
		
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		int CharacterSessionId = ci->WorldCharacterInfo[i].SessionId;
		// 유효한 세션 아이디면서 플레이어의 세션아이디가 아닐때
		if (CharacterSessionId != -1 && CharacterSessionId != SessionId && ci->WorldCharacterInfo[i].X != -1)
		{
			// 월드내 해당 세션 아이디와 매칭되는 Actor 검색			
			auto Actor = FindActorBySessionId(SpawnedCharacters, CharacterSessionId);
			// 해당되는 세션 아이디가 없을 시 월드에 스폰
			if (Actor == nullptr)
			{
				FVector SpawnLocation;
				SpawnLocation.X = ci->WorldCharacterInfo[i].X;
				SpawnLocation.Y = ci->WorldCharacterInfo[i].Y;
				SpawnLocation.Z = ci->WorldCharacterInfo[i].Z;
	
				FRotator SpawnRotation;
				SpawnRotation.Yaw = ci->WorldCharacterInfo[i].Yaw;
				SpawnRotation.Pitch = ci->WorldCharacterInfo[i].Pitch;
				SpawnRotation.Roll = ci->WorldCharacterInfo[i].Roll;
	
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.Instigator = Instigator;
				SpawnParams.Name = FName(*FString(to_string(ci->WorldCharacterInfo[i].SessionId).c_str()));
		
				ACharacter* const SpawnCharacter = world->SpawnActor<ACharacter>(WhoToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
			}
			// 해당되는 세션 아이다가 있으면 위치 동기화
			else
			{				
				FVector CharacterLocation;				
				CharacterLocation.X = ci->WorldCharacterInfo[CharacterSessionId].X;
				CharacterLocation.Y = ci->WorldCharacterInfo[CharacterSessionId].Y;
				CharacterLocation.Z = ci->WorldCharacterInfo[CharacterSessionId].Z;

				FRotator CharacterRotation;
				CharacterRotation.Yaw = ci->WorldCharacterInfo[CharacterSessionId].Yaw;
				CharacterRotation.Pitch = ci->WorldCharacterInfo[CharacterSessionId].Pitch;
				CharacterRotation.Roll = ci->WorldCharacterInfo[CharacterSessionId].Roll;

				Actor->SetActorLocation(CharacterLocation);
				Actor->SetActorRotation(CharacterRotation);
			}
		}
	}	
}

void ASungminWorldCharacter::BeginPlay()
{
	Super::BeginPlay();	

	if (HUDWidgetClass != nullptr)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
		if (CurrentWidget != nullptr)
		{
			CurrentWidget->AddToViewport();
		}
	}
}

void ASungminWorldCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Socket.LogoutCharacter(SessionId);
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
