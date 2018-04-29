// Fill out your copyright notice in the Description page of Project Settings.

#include "SungminPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include <string>
#include "ChatWindowWidget.h"
#include "TimerManager.h"
#include "Monster.h"
#include <vector>
#include <algorithm>

ASungminPlayerController::ASungminPlayerController()
{
	SessionId = FMath::RandRange(0, 10000);

	// 서버와 연결
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
	bIsNeedToSpawnMonster = false;
	bIsNeedToDestroyMonster = false;

	nMonsters = -1;
	nPlayers = -1;

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

	// FIXME
	SendPlayerInfo();

	// 새로운 플레이어 입장
	if (bNewPlayerEntered)
		UpdateNewPlayer();
	// 월드 동기화
	UpdateWorldInfo();
	// 채팅 동기화
	if (bIsChatNeedUpdate)
		UpdateChat();	
	// 새로운 몬스터 소환
	if (bIsNeedToSpawnMonster)
		SpawnMonster();
	// 몬스터 파괴
	if (bIsNeedToDestroyMonster)
		DestroyMonster();
	// 몬스터 업데이트
	UpdateMonsterSet();
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

	// 캐릭터 등록
	auto Player = Cast<ASungminWorldCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Player)
		return;

	Player->SessionId = SessionId;

	auto MyLocation = Player->GetActorLocation();
	auto MyRotation = Player->GetActorRotation();

	cCharacter Character;
	// 위치
	Character.SessionId = SessionId;
	Character.X = MyLocation.X;
	Character.Y = MyLocation.Y;
	Character.Z = MyLocation.Z;
	// 회전
	Character.Yaw = MyRotation.Yaw;
	Character.Pitch = MyRotation.Pitch;
	Character.Roll = MyRotation.Roll;
	// 속도
	Character.VX = 0;
	Character.VY = 0;
	Character.VZ = 0;
	// 속성
	Character.IsAlive = Player->IsAlive();
	Character.HealthValue = Player->HealthValue;
	Character.IsAttacking = Player->IsAttacking();

	Socket->EnrollPlayer(Character);

	// Recv 스레드 시작
	Socket->StartListen();
	UE_LOG(LogClass, Log, TEXT("BeginPlay End"));

	// FIXME 
	// 플레이어 동기화 시작	
	// GetWorldTimerManager().SetTimer(SendPlayerInfoHandle, this, &ASungminPlayerController::SendPlayerInfo, 0.016f, true);
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
		if (swc && swc->SessionId == SessionId)
			return Actor;
	}
	return nullptr;
}

void ASungminPlayerController::HitCharacter(const int & SessionId)
{
	UE_LOG(LogClass, Log, TEXT("Player Hit Called %d"), SessionId);

	Socket->HitPlayer(SessionId);
}

void ASungminPlayerController::HitMonster(const int & MonsterId)
{
	UE_LOG(LogClass, Log, TEXT("Monster Hit Called %d"), MonsterId);

	Socket->HitMonster(MonsterId);
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

void ASungminPlayerController::RecvNewPlayer(cCharacter * NewPlayer_)
{
	if (NewPlayer_ != nullptr)
	{
		bNewPlayerEntered = true;
		NewPlayer = NewPlayer_;
	}
}

void ASungminPlayerController::RecvMonsterSet(MonsterSet * MonstersInfo_)
{
	if (MonstersInfo_ != nullptr)
	{
		MonsterSetInfo = MonstersInfo_;
	}
}

void ASungminPlayerController::RecvSpawnMonster(Monster * MonsterInfo_)
{
	if (MonsterInfo_ != nullptr)
	{
		MonsterInfo = MonsterInfo_;
		bIsNeedToSpawnMonster = true;
	}
}

void ASungminPlayerController::RecvDestroyMonster(Monster * MonsterInfo_)
{
	if (MonsterInfo_ != nullptr)
	{
		MonsterInfo = MonsterInfo_;
		bIsNeedToDestroyMonster = true;
	}
}

void ASungminPlayerController::SendPlayerInfo()
{
	auto Player = Cast<ASungminWorldCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Player)
		return;

	// 플레이어의 위치를 가져옴	
	const auto & Location = Player->GetActorLocation();
	const auto & Rotation = Player->GetActorRotation();
	const auto & Velocity = Player->GetVelocity();

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
}

bool ASungminPlayerController::UpdateWorldInfo()
{
	UWorld* const world = GetWorld();
	if (world == nullptr)
		return false;

	if (ci == nullptr)
		return false;

	// 플레이어 업데이트
	UpdatePlayerInfo(ci->players[SessionId]);

	// 다른 플레이어 업데이트
	TArray<AActor*> SpawnedCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASungminWorldCharacter::StaticClass(), SpawnedCharacters);

	if (nPlayers == -1)
	{	
		for (auto & player : ci->players)
		{
			if (player.first == SessionId)
				continue;

			FVector SpawnLocation;
			SpawnLocation.X = player.second.X;
			SpawnLocation.Y = player.second.Y;
			SpawnLocation.Z = player.second.Z;

			FRotator SpawnRotation;
			SpawnRotation.Yaw = player.second.Yaw;
			SpawnRotation.Pitch = player.second.Pitch;
			SpawnRotation.Roll = player.second.Roll;

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = Instigator;
			SpawnParams.Name = FName(*FString(to_string(player.second.SessionId).c_str()));

			ASungminWorldCharacter* SpawnCharacter = world->SpawnActor<ASungminWorldCharacter>(WhoToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
			SpawnCharacter->SpawnDefaultController();
			SpawnCharacter->SessionId = player.second.SessionId;
		}

		nPlayers = ci->players.size();
	} 
	else
	{
		for (auto& Character : SpawnedCharacters)
		{
			ASungminWorldCharacter * OtherPlayer = Cast<ASungminWorldCharacter>(Character);

			if (!OtherPlayer || OtherPlayer->SessionId == -1 || OtherPlayer->SessionId == SessionId)
			{
				continue;
			}

			cCharacter * info = &ci->players[OtherPlayer->SessionId];

			if (info->IsAlive)
			{
				if (OtherPlayer->HealthValue != info->HealthValue)
				{
					UE_LOG(LogClass, Log, TEXT("other player damaged."));
					// 피격 파티클 소환
					FTransform transform(OtherPlayer->GetActorLocation());
					UGameplayStatics::SpawnEmitterAtLocation(
						world, HitEmiiter, transform, true
					);
					// 피격 애니메이션 플레이
					OtherPlayer->PlayDamagedAnimation();
					OtherPlayer->HealthValue = info->HealthValue;
				}

				// 공격중일때 타격 애니메이션 플레이
				if (info->IsAttacking)
				{
					UE_LOG(LogClass, Log, TEXT("other player hit."));
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
				UE_LOG(LogClass, Log, TEXT("other player dead."));
				FTransform transform(Character->GetActorLocation());
				UGameplayStatics::SpawnEmitterAtLocation(
					world, DestroyEmiiter, transform, true
				);
				Character->Destroy();
			}
		}

	}

	
	return true;
}

void ASungminPlayerController::UpdatePlayerInfo(const cCharacter & info)
{
	auto Player = Cast<ASungminWorldCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
		return;

	UWorld* const world = GetWorld();
	if (!world)
		return;

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
		// 캐릭터 속성 업데이트
		if (Player->HealthValue != info.HealthValue)
		{
			UE_LOG(LogClass, Log, TEXT("Player damaged"));
			// 피격 파티클 스폰
			FTransform transform(Player->GetActorLocation());
			UGameplayStatics::SpawnEmitterAtLocation(
				world, HitEmiiter, transform, true
			);
			// 피격 애니메이션 스폰
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

	if (NewPlayer->SessionId == SessionId)
	{
		bNewPlayerEntered = false;
		NewPlayer = nullptr;
		return;
	}
		
	FVector SpawnLocation;
	SpawnLocation.X = NewPlayer->X;
	SpawnLocation.Y = NewPlayer->Y;
	SpawnLocation.Z = NewPlayer->Z;

	FRotator SpawnRotation;
	SpawnRotation.Yaw = NewPlayer->Yaw;
	SpawnRotation.Pitch = NewPlayer->Pitch;
	SpawnRotation.Roll = NewPlayer->Roll;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = Instigator;
	SpawnParams.Name = FName(*FString(to_string(NewPlayer->SessionId).c_str()));

	ASungminWorldCharacter* SpawnCharacter = world->SpawnActor<ASungminWorldCharacter>(WhoToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
	SpawnCharacter->SpawnDefaultController();
	SpawnCharacter->SessionId = NewPlayer->SessionId;

	UE_LOG(LogClass, Log, TEXT("other player spawned."));

	/*
	// 월드 내 OtherNetworkCharacter 불러옴
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
				SpawnCharacter->SessionId = player->SessionId;

				UE_LOG(LogClass, Log, TEXT("other player spawned."));
			}
		}
	}
	*/
	bNewPlayerEntered = false;
	NewPlayer = nullptr;
}

void ASungminPlayerController::UpdateMonsterSet()
{
	if (MonsterSetInfo == nullptr)
		return;

	UWorld* const world = GetWorld();
	if (world)
	{
		TArray<AActor*> SpawnedMonsters;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMonster::StaticClass(), SpawnedMonsters);		

		if (nMonsters == -1)
		{
			nMonsters = MonsterSetInfo->monsters.size();

			for (const auto& kvp : MonsterSetInfo->monsters)
			{
				const Monster * monster = &kvp.second;
				FVector SpawnLocation;
				SpawnLocation.X = monster->X;
				SpawnLocation.Y = monster->Y;
				SpawnLocation.Z = monster->Z;

				FRotator SpawnRotation(0, 0, 0);

				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = this;
				SpawnParams.Instigator = Instigator;
				SpawnParams.Name = FName(*FString(to_string(monster->Id).c_str()));

				AMonster* SpawnMonster = world->SpawnActor<AMonster>(MonsterToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
				if (SpawnMonster)
				{
					SpawnMonster->SpawnDefaultController();
					SpawnMonster->Id = monster->Id;
					SpawnMonster->Health = monster->Health;
				}

			}
		}
		else
		{
			for (auto actor : SpawnedMonsters)
			{
				AMonster * monster = Cast<AMonster>(actor);
				if (monster)
				{
					const Monster * MonsterInfo = &MonsterSetInfo->monsters[monster->Id];

					FVector Location;
					Location.X = MonsterInfo->X;
					Location.Y = MonsterInfo->Y;
					Location.Z = MonsterInfo->Z;

					monster->MoveToLocation(Location);

					if (MonsterInfo->IsAttacking)
					{
						monster->PlayAttackMontage();
					}
				}
			}
		}
	}
}

void ASungminPlayerController::SpawnMonster()
{
	UWorld* const world = GetWorld();
	if (world)
	{
		// 새로운 몬스터를 스폰
		FVector SpawnLocation;
		SpawnLocation.X = MonsterInfo->X;
		SpawnLocation.Y = MonsterInfo->Y;
		SpawnLocation.Z = MonsterInfo->Z;

		FRotator SpawnRotation(0, 0, 0);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = Instigator;
		SpawnParams.Name = FName(*FString(to_string(MonsterInfo->Id).c_str()));

		AMonster* SpawnMonster = world->SpawnActor<AMonster>(MonsterToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
		if (SpawnMonster)
		{
			SpawnMonster->SpawnDefaultController();
			SpawnMonster->Id = MonsterInfo->Id;
			SpawnMonster->Health = MonsterInfo->Health;
		}

		// 업데이트 후 초기화
		MonsterInfo = nullptr;
		bIsNeedToSpawnMonster = false;
	}
}

void ASungminPlayerController::DestroyMonster()
{
	UWorld* const world = GetWorld();
	if (world)
	{
		// 스폰된 몬스터에서 찾아 파괴
		TArray<AActor*> SpawnedMonsters;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMonster::StaticClass(), SpawnedMonsters);

		for (auto Actor : SpawnedMonsters)
		{
			AMonster * Monster = Cast<AMonster>(Actor);
			if (Monster && Monster->Id == MonsterInfo->Id)
			{
				Monster->Dead();
				break;
			}
		}

		// 업데이트 후 초기화
		MonsterInfo = nullptr;
		bIsNeedToDestroyMonster = false;
	}
}
