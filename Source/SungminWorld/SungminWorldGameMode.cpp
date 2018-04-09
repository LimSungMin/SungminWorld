// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SungminWorldGameMode.h"
#include "SungminWorldCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "OtherNetworkCharacter.h"
#include <string>

ASungminWorldGameMode::ASungminWorldGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	// AGameModeBase �� ��ӹ޴� ���Ӹ�忡���� 
	// �Ʒ� �ڵ尡 �־�� ĳ���� ������ tick �� Ȱ��ȭ �ȴ�
	PrimaryActorTick.bCanEverTick = true;

	// ���� ���̵� ���� (������ ������)
	SessionId = FMath::RandRange(0, 100);

	// ������ ����
	Socket.InitSocket();
	bIsConnected = Socket.Connect("127.0.0.1", 8000);
	if (bIsConnected)
	{
		UE_LOG(LogClass, Log, TEXT("IOCP Server connect success!"));
	}
}

void ASungminWorldGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bIsConnected)
		return;
	// UE_LOG(LogClass, Log, TEXT("TEST"));
	auto Player = Cast<ASungminWorldCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player) 
		return;	
	// UE_LOG(LogClass, Log, TEXT("%f"), PlayerLocation.X);
	
	// �÷��̾��� ��ġ�� ������	
	auto MyLocation = Player->GetActorLocation();
	auto MyRotation = Player->GetActorRotation();

	cCharacter Character;
	Character.SessionId = SessionId;
	Character.X = MyLocation.X;
	Character.Y = MyLocation.Y;
	Character.Z = MyLocation.Z;
	Character.Yaw = MyRotation.Yaw;
	Character.Pitch = MyRotation.Pitch;
	Character.Roll = MyRotation.Roll;

	// �÷��̾��� ���� ���̵�� ��ġ�� �������� ����
	cCharactersInfo* ci = Socket.SyncCharacters(Character);
	if (ci == nullptr)
		return;

	UWorld* const world = GetWorld();
	if (world == nullptr)
		return;

	// ���� �� OtherCharacter ���� ����
	TArray<AActor*> SpawnedCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOtherNetworkCharacter::StaticClass(), SpawnedCharacters);
	
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		int CharacterSessionId = ci->WorldCharacterInfo[i].SessionId;
		// ��ȿ�� ���� ���̵�鼭 �÷��̾��� ���Ǿ��̵� �ƴҶ�
		if (CharacterSessionId != -1 && CharacterSessionId != SessionId && ci->WorldCharacterInfo[i].X != -1)
		{
			// ���峻 �ش� ���� ���̵�� ��Ī�Ǵ� Actor �˻�			
			auto Actor = FindActorBySessionId(SpawnedCharacters, CharacterSessionId);
			// �ش�Ǵ� ���� ���̵� ���� �� ���忡 ����
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
			// �ش�Ǵ� ���� ���̴ٰ� ������ ��ġ ����ȭ
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

void ASungminWorldGameMode::BeginPlay()
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

void ASungminWorldGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	Socket.LogoutCharacter(SessionId);
}

AActor * ASungminWorldGameMode::FindActorBySessionId(TArray<AActor*> ActorArray, const int & SessionId)
{
	for (const auto& Actor : ActorArray)
	{
		if (stoi(*Actor->GetName()) == SessionId)
			return Actor;
	}
	return nullptr;
}
