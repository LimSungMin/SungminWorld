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

	// �α��� �Լ�
	UFUNCTION(BlueprintCallable, Category = "Login")
	bool LoginToServer(FString id);

	// ä�� �Լ�
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void ChatToServer(FString Text);

	// ���� ���̵� ��ȭ
	UFUNCTION(BlueprintPure, Category = "Properties")
	int GetSessionId();

	// ĳ���� �����
	UFUNCTION(BlueprintCallable, Category = "Game")
	void RestartGame();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	AActor* FindActorBySessionId(TArray<AActor*> ActorArray, const int& SessionId);

	// HUD ȭ�鿡�� �� ���� Ŭ����
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Properties", Meta = (BlueprintProtect = "true"))
	TSubclassOf<class UUserWidget> HUDWidgetClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Properties", Meta = (BlueprintProtect = "true"))
	TSubclassOf<class UUserWidget> GameOverWidgetClass;

	// HUD ��ü
	UPROPERTY()
	class UUserWidget* CurrentWidget;

	UPROPERTY()
	class UUserWidget* GameOverWidget;

	// ������ų �ٸ� ĳ����
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<class ACharacter> WhoToSpawn;	

	// �ı��� �� ��ƼŬ
	UPROPERTY(EditAnywhere, Category = "Spawning")
	UParticleSystem* DestroyEmiiter;

	// Ÿ���� �� ��ƼŬ
	UPROPERTY(EditAnywhere, Category = "Spawning")
	UParticleSystem* HitEmiiter;

	void HitCharacter(const int& SessionId, const AOtherNetworkCharacter* DamagedCharacter);
	
	void SyncCharactersInfo(cCharactersInfo * ci);
	void SynchronizeChat(const string* chat);

	void TestDebug();
	void SetNeedChatUpdate(bool bUpdate);

private:
	ClientSocket*	Socket;			// ������ ������ ����
	bool			bIsConnected;	// ������ ���� ����
	int				SessionId;		// ĳ������ ���� ���� ���̵� (������)
	cCharactersInfo * ci;

	bool SendPlayerInfo();			// �÷��̾� ��ġ �۽�
	bool SynchronizeWorld();		// ���� ����ȭ
	void SynchronizePlayer(const cCharacter & info);		// �÷��̾� ����ȭ	
	bool bIsChatNeedUpdate;
	void UpdateChat();
	const string* sChat;	
};



