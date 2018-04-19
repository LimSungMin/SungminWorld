// Fill out your copyright notice in the Description page of Project Settings.

#include "SungminPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include <string>
#include "ChatWindowWidget.h"

ASungminPlayerController::ASungminPlayerController()
{
	SessionId = FMath::RandRange(0, 100);

	// ������ ����
	Socket = ClientSocket::GetSingleton();
	Socket->InitSocket();
	bIsConnected = Socket->Connect("127.0.0.1", 8000);
	if (bIsConnected)
	{
		UE_LOG(LogClass, Log, TEXT("IOCP Server connect success!"));
		Socket->SetPlayerController(this);
	}
}

ASungminPlayerController::~ASungminPlayerController()
{
}

void ASungminPlayerController::ChatToServer(FString Text)
{
	UE_LOG(LogClass, Log, TEXT("%s"), *Text);
	Socket->SendChat(SessionId, TCHAR_TO_UTF8(*Text));
}

int ASungminPlayerController::GetSessionId()
{
	return SessionId;
}

void ASungminPlayerController::Tick(float DeltaSeconds)
{	
	Super::Tick(DeltaSeconds);		
	
	if (!bIsConnected) return;

	// �÷��̾� ���� �۽�
	if (!SendPlayerInfo()) return;

	// ���� ����ȭ
	if (!UpdateWorldInfo()) return;

	// ä�� ����ȭ
	if (bIsChatNeedUpdate)
	{
		UpdateChat();
	}
}

void ASungminPlayerController::BeginPlay()
{
	UE_LOG(LogClass, Log, TEXT("BeginPlay Start"));
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
	// ȸ��
	Character.Yaw = MyRotation.Yaw;
	Character.Pitch = MyRotation.Pitch;
	Character.Roll = MyRotation.Roll;
	// �ӵ�
	Character.VX = 0;
	Character.VY = 0;
	Character.VZ = 0;
	// �Ӽ�
	Character.IsAlive = Player->IsAlive();
	Character.HealthValue = Player->HealthValue;
	
	Socket->EnrollCharacterInfo(Character);

	// Recv ������ ����
	Socket->StartListen();
	UE_LOG(LogClass, Log, TEXT("BeginPlay End"));
}

void ASungminPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{	
	Socket->LogoutCharacter(SessionId);
	Socket->CloseSocket();
	Socket->StopListen();
}

AActor * ASungminPlayerController::FindActorBySessionId(TArray<AActor*> ActorArray, const int & SessionId)
{
	for (const auto& Actor : ActorArray)
	{
		if (stoi(*Actor->GetName()) == SessionId)
			return Actor;
	}
	return nullptr;
}

void ASungminPlayerController::HitCharacter(const int & SessionId, const AOtherNetworkCharacter * DamagedCharacter)
{
	UE_LOG(LogClass, Log, TEXT("Damaged Called %d"), SessionId);

	UWorld* const world = GetWorld();

	FTransform transform(DamagedCharacter->GetActorLocation());
	UGameplayStatics::SpawnEmitterAtLocation(
		world, HitEmiiter, transform, true
	);

	Socket->DamagingCharacter(SessionId);
}

void ASungminPlayerController::RecvWorldInfo(cCharactersInfo * ci_)
{
	ci = ci_;
}

void ASungminPlayerController::RecvChat(const string * chat)
{
	sChat = chat;
	bIsChatNeedUpdate = true;
}

void ASungminPlayerController::SetNeedChatUpdate(bool bUpdate)
{
	bIsChatNeedUpdate = bUpdate;
}

bool ASungminPlayerController::SendPlayerInfo()
{
	auto Player = Cast<ASungminWorldCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
		return false;

	// �÷��̾��� ��ġ�� ������	
	const auto & Location = Player->GetActorLocation();	
	const auto & Rotation = Player->GetActorRotation();
	const auto & Velocity = Player->GetVelocity();
	const bool IsFalling = Player->IsFalling();
	
	cCharacter Character;
	Character.SessionId = SessionId;

	Character.X = Location.X;
	Character.Y = Location.Y;
	Character.Z = Location.Z;

	Character.Yaw = Rotation.Yaw;
	Character.Pitch = Rotation.Pitch;
	Character.Roll = Rotation.Roll;		

	Character.VX = Velocity.X;
	Character.VY = Velocity.Y;
	Character.VZ = Velocity.Z;	

	Socket->SendCharacterInfo(Character);

	return true;
}

bool ASungminPlayerController::UpdateWorldInfo()
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
		cCharacter* info = &ci->WorldCharacterInfo[i];
		// �÷��̾� ó��
		if (info->SessionId == SessionId)
		{
			UpdatePlayerInfo(ci->WorldCharacterInfo[i]);
			continue;
		}
		// �ٸ� ��Ʈ��ũ ĳ���� ó��
		if (info->SessionId != -1)
		{
			// ���峻 �ش� ���� ���̵�� ��Ī�Ǵ� Actor �˻�			
			auto Actor = FindActorBySessionId(SpawnedCharacters, info->SessionId);
			// �ش�Ǵ� ���� ���̵� ���� �� ���忡 ����
			if (Actor == nullptr && ci->WorldCharacterInfo[i].IsAlive == true)
			{
				FVector SpawnLocation;
				SpawnLocation.X = info->X;
				SpawnLocation.Y = info->Y;
				SpawnLocation.Z = info->Z;

				FRotator SpawnRotation;
				SpawnRotation.Yaw = info->Yaw;
				SpawnRotation.Pitch = info->Pitch;
				SpawnRotation.Roll = info->Roll;

				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.Instigator = Instigator;
				SpawnParams.Name = FName(*FString(to_string(info->SessionId).c_str()));

				ACharacter* const SpawnCharacter = world->SpawnActor<ACharacter>(WhoToSpawn, SpawnLocation, SpawnRotation, SpawnParams);		
				SpawnCharacter->SpawnDefaultController();
			}
			// �ش�Ǵ� ���� ���̴ٰ� ������ ��ġ ����ȭ
			else if (Actor != nullptr && ci->WorldCharacterInfo[i].IsAlive == true)
			{
				AOtherNetworkCharacter* OtherCharacter = Cast<AOtherNetworkCharacter>(Actor);
				if (OtherCharacter)
				{
					if (OtherCharacter->HealthValue != info->HealthValue)
					{
						// spawn damaged emitter
						FTransform transform(OtherCharacter->GetActorLocation());
						UGameplayStatics::SpawnEmitterAtLocation(
							world, HitEmiiter, transform, true
						);
					}
					OtherCharacter->HealthValue = info->HealthValue;

					FVector CharacterLocation;
					CharacterLocation.X = info->X;
					CharacterLocation.Y = info->Y;
					CharacterLocation.Z = info->Z;

					FRotator CharacterRotation;
					CharacterRotation.Yaw = info->Yaw;
					CharacterRotation.Pitch = info->Pitch;
					CharacterRotation.Roll = info->Roll;

					FVector CharacterVelocity;
					CharacterVelocity.X = info->VX;
					CharacterVelocity.Y = info->VY;
					CharacterVelocity.Z = info->VZ;

					OtherCharacter->AddMovementInput(CharacterVelocity);
					OtherCharacter->SetActorRotation(CharacterRotation);	
					OtherCharacter->SetActorLocation(CharacterLocation);
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

void ASungminPlayerController::UpdatePlayerInfo(const cCharacter & info)
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

		CurrentWidget->RemoveFromParent();
		GameOverWidget = CreateWidget<UUserWidget>(GetWorld(), GameOverWidgetClass);
		if (GameOverWidget != nullptr)
		{
			GameOverWidget->AddToViewport();
		}
	}
	else
	{
		// ĳ���� �Ӽ� ������Ʈ
		if (Player->HealthValue != info.HealthValue)
		{
			FTransform transform(Player->GetActorLocation());
			UGameplayStatics::SpawnEmitterAtLocation(
				world, HitEmiiter, transform, true
			);
			Player->HealthValue = info.HealthValue;
		}
	}
}

void ASungminPlayerController::UpdateChat()
{
	UChatWindowWidget* temp = Cast<UChatWindowWidget>(CurrentWidget);
	if (temp != nullptr)
	{
		UE_LOG(LogClass, Log, TEXT("Casting"));
		temp->CallUpdateChat(
			FText::FromString(*FString(sChat->c_str()))
		);
	}
	bIsChatNeedUpdate = false;
}
