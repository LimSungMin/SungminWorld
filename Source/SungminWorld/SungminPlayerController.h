// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ClientSocket.h"
#include "SungminWorldCharacter.h"
#include "SungminPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SUNGMINWORLD_API ASungminPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASungminPlayerController();
	~ASungminPlayerController();	

	// ä�� �Լ�
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void ChatToServer(FString Text);

	// ���� ���̵� ��ȭ
	UFUNCTION(BlueprintPure, Category = "Properties")
	int GetSessionId();

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

	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<class AMonster> MonsterToSpawn;

	// �ı��� �� ��ƼŬ
	UPROPERTY(EditAnywhere, Category = "Spawning")
	UParticleSystem* DestroyEmiiter;

	// Ÿ���� �� ��ƼŬ
	UPROPERTY(EditAnywhere, Category = "Spawning")
	UParticleSystem* HitEmiiter;

	// ���Ͽ��� �ٸ� ĳ���� Ÿ�� ���� ����
	UFUNCTION(BlueprintCallable, Category = "Interaction")
		void HitCharacter(const int& SessionId);
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;	

	// ���Ǿ��̵� ��Ī�Ǵ� ���� ��ȯ
	AActor* FindActorBySessionId(TArray<AActor*> ActorArray, const int& SessionId);	
	// ���� Ÿ�� ���� ����
	void HitMonster(const int& MonsterId);
	// �������κ��� ���� ���� ����
	void RecvWorldInfo(cCharactersInfo * ci);
	// �������κ��� ä�� ���� ����
	void RecvChat(const string* chat);		
	// �� �÷��̾� ������Ʈ
	void RecvNewPlayer(cCharacter * NewPlayer);
	// ���� ������Ʈ
	void RecvMonsterSet(MonsterSet * MonstersInfo);	
	// �� ���� ����
	void RecvSpawnMonster(Monster * MonsterInfo);
	// ���� ���� �ı�
	void RecvDestroyMonster(Monster * MonsterInfo);
	
private:
	ClientSocket *		Socket;			// ������ ������ ����
	bool				bIsConnected;	// ������ ���� ����
	int					SessionId;		// ĳ������ ���� ���� ���̵� (������)
	cCharactersInfo *	CharactersInfo;	// �ٸ� ĳ������ ����

	void SendPlayerInfo();		// �÷��̾� ��ġ �۽�
	bool UpdateWorldInfo();		// ���� ����ȭ
	void UpdatePlayerInfo(const cCharacter & info);		// �÷��̾� ����ȭ	

	FTimerHandle SendPlayerInfoHandle;	// ����ȭ Ÿ�̸� �ڵ鷯

	// ä�� ������Ʈ
	bool bIsChatNeedUpdate;
	const string* chat;
	void UpdateChat();
	
	// �� �÷��̾� ����
	int	nPlayers;
	bool bNewPlayerEntered;
	cCharacter * NewPlayer;
	void UpdateNewPlayer();

	// ���� ������Ʈ
	MonsterSet * MonsterSetInfo;
	int nMonsters;
	void UpdateMonsterSet();

	Monster * MonsterInfo;
	bool bIsNeedToSpawnMonster;
	void SpawnMonster();
	
	bool bIsNeedToDestroyMonster;
	void DestroyMonster();
};
