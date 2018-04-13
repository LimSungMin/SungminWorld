// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SungminWorldGameMode.h"
#include "SungminWorldCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
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
	Socket = ClientSocket::GetSingleton();
	Socket->InitSocket();
	bIsConnected = Socket->Connect("127.0.0.1", 8000);
	if (bIsConnected)
	{
		UE_LOG(LogClass, Log, TEXT("IOCP Server connect success!"));
		Socket->SetGameMode(this);
	}
}

bool ASungminWorldGameMode::LoginToServer(FString id)
{
	if (!id.IsEmpty())
	{
		UE_LOG(LogClass, Log, TEXT("Login Success"));
		return true;
	}
	UE_LOG(LogClass, Log, TEXT("Login Fail"));
	return false;
}

void ASungminWorldGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsConnected) return;

	if (!SendPlayerInfo()) return;
		
	if (!SynchronizeWorld()) return;	
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
	// ĳ���� ���
	auto Player = Cast<ASungminWorldCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
		return;

	auto MyLocation = Player->GetActorLocation();
	auto MyRotation = Player->GetActorRotation();

	cCharacter Character;
	// ��ġ
	Character.SessionId = SessionId;
	Character.X = MyLocation.X;
	Character.Y = MyLocation.Y;
	Character.Z = MyLocation.Z;
	Character.Yaw = MyRotation.Yaw;
	Character.Pitch = MyRotation.Pitch;
	Character.Roll = MyRotation.Roll;
	// �Ӽ�
	Character.IsAlive = true;
	Character.HealthValue = Player->GetHealth();
	Socket->EnrollCharacterInfo(Character);

	// Recv ������ ����
	Socket->StartListen();
}

void ASungminWorldGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Socket->LogoutCharacter(SessionId);
	Socket->CloseSocket();	
	Socket->StopListen();	
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

void ASungminWorldGameMode::HitCharacter(const int& SessionId, const AOtherNetworkCharacter* DamagedCharacter)
{
	UE_LOG(LogClass, Log, TEXT("Damaged Called %d"), SessionId);
	
	UWorld* const world = GetWorld();

	FTransform transform(DamagedCharacter->GetActorLocation());
	UGameplayStatics::SpawnEmitterAtLocation(
		world, HitEmiiter, transform, true
	);

	Socket->DamagingCharacter(SessionId);
}

void ASungminWorldGameMode::SyncCharactersInfo(cCharactersInfo * ci_)
{	
	ci = ci_;	
}

void ASungminWorldGameMode::TestDebug()
{
	UE_LOG(LogClass, Log, TEXT("DEBUGGING"));
}

bool ASungminWorldGameMode::SendPlayerInfo()
{
	auto Player = Cast<ASungminWorldCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
		return false;

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

	Socket->SendCharacterInfo(Character);

	return true;
}

bool ASungminWorldGameMode::SynchronizeWorld()
{
	UWorld* const world = GetWorld();
	if (world == nullptr)
		return false;

	if (ci == nullptr)
		return false;

	// ���� �� OtherCharacter ���� ����
	TArray<AActor*> SpawnedCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOtherNetworkCharacter::StaticClass(), SpawnedCharacters);

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		int CharacterSessionId = ci->WorldCharacterInfo[i].SessionId;
		// �÷��̾� ó��
		if (CharacterSessionId == SessionId)
		{			
			SynchronizePlayer(ci->WorldCharacterInfo[i]);
			continue;
		}
		// �ٸ� ��Ʈ��ũ ĳ���� ó��
		if (CharacterSessionId != -1)
		{
			// ���峻 �ش� ���� ���̵�� ��Ī�Ǵ� Actor �˻�			
			auto Actor = FindActorBySessionId(SpawnedCharacters, CharacterSessionId);
			// �ش�Ǵ� ���� ���̵� ���� �� ���忡 ����
			if (Actor == nullptr && ci->WorldCharacterInfo[i].IsAlive == true)
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
			else if (Actor != nullptr && ci->WorldCharacterInfo[i].IsAlive == true)
			{
				AOtherNetworkCharacter* OtherCharacter = Cast<AOtherNetworkCharacter>(Actor);
				if (OtherCharacter)
				{
					if (OtherCharacter->HealthValue != ci->WorldCharacterInfo[i].HealthValue)
					{
						// spawn damaged emitter
						FTransform transform(OtherCharacter->GetActorLocation());
						UGameplayStatics::SpawnEmitterAtLocation(
							world, HitEmiiter, transform, true
						);
					}
					OtherCharacter->HealthValue = ci->WorldCharacterInfo[i].HealthValue;

					FVector CharacterLocation;
					CharacterLocation.X = ci->WorldCharacterInfo[CharacterSessionId].X;
					CharacterLocation.Y = ci->WorldCharacterInfo[CharacterSessionId].Y;
					CharacterLocation.Z = ci->WorldCharacterInfo[CharacterSessionId].Z;

					FRotator CharacterRotation;
					CharacterRotation.Yaw = ci->WorldCharacterInfo[CharacterSessionId].Yaw;
					CharacterRotation.Pitch = ci->WorldCharacterInfo[CharacterSessionId].Pitch;
					CharacterRotation.Roll = ci->WorldCharacterInfo[CharacterSessionId].Roll;

					OtherCharacter->SetActorLocation(CharacterLocation);
					OtherCharacter->SetActorRotation(CharacterRotation);
				}				
			}
			else if (Actor != nullptr && ci->WorldCharacterInfo[i].IsAlive == false)
			{
				UE_LOG(LogClass, Log, TEXT("Destroy Actor"));
				FTransform transform(Actor->GetActorLocation());
				UGameplayStatics::SpawnEmitterAtLocation(
					world, DestroyEmiiter, transform, true
				);
				Actor->Destroy();
			}
		}
	}
	return true;
}

void ASungminWorldGameMode::SynchronizePlayer(const cCharacter & info)
{
	auto Player = Cast<ASungminWorldCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	UWorld* const world = GetWorld();

	if (!info.IsAlive)
	{
		FTransform transform(Player->GetActorLocation());
		UGameplayStatics::SpawnEmitterAtLocation(
			world, DestroyEmiiter, transform, true
		);
		Player->Destroy();
	}
	// ĳ���� �Ӽ� ������Ʈ
	if (Player->HealthValue != info.HealthValue)
	{
		FTransform transform(Player->GetActorLocation());
		UGameplayStatics::SpawnEmitterAtLocation(
			world, HitEmiiter, transform, true
		);
	}
	Player->HealthValue = info.HealthValue;
}
