// Fill out your copyright notice in the Description page of Project Settings.

#include "SungminPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include <string>
#include "ChatWindowWidget.h"

ASungminPlayerController::ASungminPlayerController()
{
	SessionId = FMath::RandRange(0, 100);

	// 서버와 연결
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
	//UE_LOG(LogClass, Log, TEXT("Tick Start"));	
	Super::Tick(DeltaSeconds);

	if (!bIsConnected) return;

	// 플레이어 정보 송신
	if (!SendPlayerInfo()) return;

	// 월드 동기화
	if (!UpdateWorldInfo()) return;

	// 채팅 동기화
	if (bIsChatNeedUpdate)
	{
		UpdateChat();
	}
	//UE_LOG(LogClass, Log, TEXT("Tick End"));
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
	auto Player = Cast<ASungminWorldCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!Player)
		return;

	auto MyLocation = Player->GetActorLocation();
	auto MyRotation = Player->GetActorRotation();

	

	cCharacter Character;
	// 위치
	Character.SessionId = SessionId;
	Character.X = MyLocation.X;
	Character.Y = MyLocation.Y;
	Character.Z = MyLocation.Z;
	Character.Yaw = MyRotation.Yaw;
	Character.Pitch = MyRotation.Pitch;
	Character.Roll = MyRotation.Roll;
	// 속성
	Character.IsAlive = true;
	Character.HealthValue = 0.5;
	Socket->EnrollCharacterInfo(Character);

	// Recv 스레드 시작
	Socket->StartListen();
	UE_LOG(LogClass, Log, TEXT("BeginPlay End"));
}

void ASungminPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
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

	// 플레이어의 위치를 가져옴	
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

bool ASungminPlayerController::UpdateWorldInfo()
{
	UWorld* const world = GetWorld();
	if (world == nullptr)
		return false;

	if (ci == nullptr)
		return false;

	// 월드 내 OtherCharacter 액터 수집
	TArray<AActor*> SpawnedCharacters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOtherNetworkCharacter::StaticClass(), SpawnedCharacters);

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		int CharacterSessionId = ci->WorldCharacterInfo[i].SessionId;
		// 플레이어 처리
		if (CharacterSessionId == SessionId)
		{
			UpdatePlayerInfo(ci->WorldCharacterInfo[i]);
			continue;
		}
		// 다른 네트워크 캐릭터 처리
		if (CharacterSessionId != -1)
		{
			// 월드내 해당 세션 아이디와 매칭되는 Actor 검색			
			auto Actor = FindActorBySessionId(SpawnedCharacters, CharacterSessionId);
			// 해당되는 세션 아이디가 없을 시 월드에 스폰
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
			// 해당되는 세션 아이다가 있으면 위치 동기화
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
		// 캐릭터 속성 업데이트
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
