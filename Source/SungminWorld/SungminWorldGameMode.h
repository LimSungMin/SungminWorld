// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ClientSocket.h"
#include "OtherNetworkCharacter.h"
#include "SungminWorldGameMode.generated.h"

UCLASS(minimalapi)
class ASungminWorldGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASungminWorldGameMode();

	// 로그인 함수
	UFUNCTION(BlueprintCallable, Category = "Login")
	bool LoginToServer(FString id);

	// 채팅 함수
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void ChatToServer(FString Text);

	// 세션 아이디 반화
	UFUNCTION(BlueprintPure, Category = "Properties")
	int GetSessionId();

	// 캐릭터 재시작
	UFUNCTION(BlueprintCallable, Category = "Game")
	void RestartGame();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	AActor* FindActorBySessionId(TArray<AActor*> ActorArray, const int& SessionId);

	// HUD 화면에서 쓸 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Properties", Meta = (BlueprintProtect = "true"))
	TSubclassOf<class UUserWidget> HUDWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Properties", Meta = (BlueprintProtect = "true"))
	TSubclassOf<class UUserWidget> GameOverWidgetClass;

	// HUD 객체
	UPROPERTY()
	class UUserWidget* CurrentWidget;

	UPROPERTY()
	class UUserWidget* GameOverWidget;

	// 스폰시킬 다른 캐릭터
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<class ACharacter> WhoToSpawn;	

	// 파괴될 때 파티클
	UPROPERTY(EditAnywhere, Category = "Spawning")
	UParticleSystem* DestroyEmiiter;

	// 타격할 때 파티클
	UPROPERTY(EditAnywhere, Category = "Spawning")
	UParticleSystem* HitEmiiter;

	void HitCharacter(const int& SessionId, const AOtherNetworkCharacter* DamagedCharacter);
	
	void SyncCharactersInfo(cCharactersInfo * ci);
	void SynchronizeChat(const string* chat);

	void TestDebug();
	void SetNeedChatUpdate(bool bUpdate);

private:
	ClientSocket*	Socket;			// 서버와 접속할 소켓
	bool			bIsConnected;	// 서버와 접속 유무
	int				SessionId;		// 캐릭터의 세션 고유 아이디 (랜덤값)
	cCharactersInfo * ci;

	bool SendPlayerInfo();			// 플레이어 위치 송신
	bool SynchronizeWorld();		// 월드 동기화
	void SynchronizePlayer(const cCharacter & info);		// 플레이어 동기화	
	bool bIsChatNeedUpdate;
	void UpdateChat();
	const string* sChat;	
};



