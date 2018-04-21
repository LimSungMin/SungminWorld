// Fill out your copyright notice in the Description page of Project Settings.

#include "SungminPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include <string>
#include "ChatWindowWidget.h"

ASungminPlayerController::ASungminPlayerController()
{
	SessionId = FMath::RandRange(0, 10000);

	// ������ ����
	Socket = ClientSocket::GetSingleton();
	Socket->InitSocket();
	bIsConnected = Socket->Connect("127.0.0.1", 8000);
	if (bIsConnected)
	{
		UE_LOG(LogClass, Log, TEXT("IOCP Server connect success!"));
		Socket->SetPlayerController(this);
	}

	bIsChatNeedUpdate = false;
	bNewPlayerEntered = false;

	PrimaryActorTick.bCanEverTick = true;
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

	if (bNewPlayerEntered)
	{
		UpdateNewPlayer();
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
	auto Player = Cast<ASungminWorldCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Player)
		return;

	Player->SetSessionId(SessionId);	

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
	Character.IsAttacking = Player->IsAttacking();
	
	Socket->EnrollPlayer(Character);

	// Recv ������ ����
	Socket->StartListen();
	UE_LOG(LogClass, Log, TEXT("BeginPlay End"));
}

void ASungminPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{	
	Socket->LogoutPlayer(SessionId);
	Socket->CloseSocket();
	Socket->StopListen();
}

AActor * ASungminPlayerController::FindActorBySessionId(TArray<AActor*> ActorArray, const int & SessionId)
{
	for (const auto& Actor : ActorArray)
	{
		ASungminWorldCharacter * swc = Cast<ASungminWorldCharacter>(Actor);
		if (swc && swc->GetSessionId() == SessionId)
			return Actor;		
	}
	return nullptr;
}

void ASungminPlayerController::HitCharacter(const int & SessionId, const ASungminWorldCharacter * DamagedCharacter)
{
	UE_LOG(LogClass, Log, TEXT("Damaged Called %d"), SessionId);

	UWorld* const world = GetWorld();

	if (DamagedCharacter != nullptr)
	{
		Socket->DamagePlayer(SessionId);
	}
}

void ASungminPlayerController::RecvWorldInfo(cCharactersInfo * ci_)
{
	if (ci_ != nullptr)
	{
		ci = ci_;
	}
	
}

void ASungminPlayerController::RecvChat(const string * chat_)
{
	if (chat_ != nullptr)
	{
		chat = chat_;
		bIsChatNeedUpdate = true;
	}	
}

void ASungminPlayerController::RecvNewPlayer(cCharactersInfo * NewPlayer_)
{
	if(NewPlayer_ != nullptr)
	{
		bNewPlayerEntered = true;
		NewPlayer = NewPlayer_;
	}	
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

	Character.IsAlive = Player->IsAlive();
	Character.HealthValue = Player->HealthValue;
	Character.IsAttacking = Player->IsAttacking();

	Socket->SendPlayer(Character);

	return true;
}

bool ASungminPlayerController::UpdateWorldInfo()
{
	UWorld* const world = GetWorld();
	if (world == nullptr)
		return false;

	if (ci == nullptr)
		return false;

	// �÷��̾� ������Ʈ
	UpdatePlayerInfo(ci->players[SessionId]);

	// �ٸ� �÷��̾� ������Ʈ
	TArray<AActor*> SpawnedCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASungminWorldCharacter::StaticClass(), SpawnedCharacters);

	for (const auto& Character : SpawnedCharacters)
	{		
		ASungminWorldCharacter * OtherPlayer = Cast<ASungminWorldCharacter>(Character);
		
		if (!OtherPlayer || OtherPlayer->GetSessionId() == -1 ||OtherPlayer->GetSessionId() == SessionId)
		{
			continue;
		}

		cCharacter * info = &ci->players[OtherPlayer->GetSessionId()];
		
		if (info->IsAlive)
		{
			if (OtherPlayer->HealthValue != info->HealthValue)
			{
				// �ǰ� ��ƼŬ ��ȯ
				FTransform transform(OtherPlayer->GetActorLocation());
				UGameplayStatics::SpawnEmitterAtLocation(
					world, HitEmiiter, transform, true
				);
				// �ǰ� �ִϸ��̼� �÷���
				OtherPlayer->PlayDamagedAnimation();
				OtherPlayer->HealthValue = info->HealthValue;
			}			
			
			// �������϶� Ÿ�� �ִϸ��̼� �÷���
			if (info->IsAttacking)
			{				
				UE_LOG(LogClass, Log, TEXT("Other character is hitting"));
				OtherPlayer->PlayHitAnimation();
			}

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

			OtherPlayer->AddMovementInput(CharacterVelocity);
			OtherPlayer->SetActorRotation(CharacterRotation);
			OtherPlayer->SetActorLocation(CharacterLocation);
		}
		else
		{
			UE_LOG(LogClass, Log, TEXT("Destroy Actor"));			
			FTransform transform(Character->GetActorLocation());
			UGameplayStatics::SpawnEmitterAtLocation(
				world, DestroyEmiiter, transform, true
			);
			Character->Destroy();
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
		UE_LOG(LogClass, Log, TEXT("Player Die"));
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
			UE_LOG(LogClass, Log, TEXT("Player damaged"));
			// �ǰ� ��ƼŬ ����
			FTransform transform(Player->GetActorLocation());
			UGameplayStatics::SpawnEmitterAtLocation(
				world, HitEmiiter, transform, true
			);			
			// �ǰ� �ִϸ��̼� ����
			Player->PlayDamagedAnimation();
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
			FText::FromString(*FString(chat->c_str()))
		);
	}
	bIsChatNeedUpdate = false;
}

void ASungminPlayerController::UpdateNewPlayer()
{
	UWorld* const world = GetWorld();

	// ���� �� OtherNetworkCharacter �ҷ���
	TArray<AActor*> SpawnedCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASungminWorldCharacter::StaticClass(), SpawnedCharacters);

	for (const auto& kvp : NewPlayer->players)
	{
		if (kvp.first == SessionId)
			continue;

		const cCharacter * player = &kvp.second;
		if (player->IsAlive)
		{
			auto Actor = FindActorBySessionId(SpawnedCharacters, player->SessionId);
			if (Actor == nullptr)
			{
				FVector SpawnLocation;
				SpawnLocation.X = player->X;
				SpawnLocation.Y = player->Y;
				SpawnLocation.Z = player->Z;

				FRotator SpawnRotation;
				SpawnRotation.Yaw = player->Yaw;
				SpawnRotation.Pitch = player->Pitch;
				SpawnRotation.Roll = player->Roll;

				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.Instigator = Instigator;
				SpawnParams.Name = FName(*FString(to_string(player->SessionId).c_str()));

				ASungminWorldCharacter* SpawnCharacter = world->SpawnActor<ASungminWorldCharacter>(WhoToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
				SpawnCharacter->SpawnDefaultController();
				SpawnCharacter->SetSessionId(player->SessionId);				
			}
		}
	}

	bNewPlayerEntered = false;
	NewPlayer = nullptr;
}
