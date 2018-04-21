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

	// �ı��� �� ��ƼŬ
	UPROPERTY(EditAnywhere, Category = "Spawning")
	UParticleSystem* DestroyEmiiter;

	// Ÿ���� �� ��ƼŬ
	UPROPERTY(EditAnywhere, Category = "Spawning")
	UParticleSystem* HitEmiiter;
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;	

	// ���Ǿ��̵� ��Ī�Ǵ� ���� ��ȯ
	AActor* FindActorBySessionId(TArray<AActor*> ActorArray, const int& SessionId);

	// ���Ͽ��� �ٸ� ĳ���� Ÿ�� ���� ����
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void HitCharacter(const int& SessionId, const ASungminWorldCharacter* DamagedCharacter);

	// �������κ��� ���� ���� ����
	void RecvWorldInfo(cCharactersInfo * ci);

	// �������κ��� ä�� ���� ����
	void RecvChat(const string* chat);		

	// �� �÷��̾� ������Ʈ
	void RecvNewPlayer(cCharactersInfo * NewPlayer);
	
private:
	ClientSocket * Socket;			// ������ ������ ����
	bool			bIsConnected;	// ������ ���� ����
	int				SessionId;		// ĳ������ ���� ���� ���̵� (������)
	cCharactersInfo * ci;			// �ٸ� ĳ������ ����

	bool SendPlayerInfo();			// �÷��̾� ��ġ �۽�
	bool UpdateWorldInfo();		// ���� ����ȭ
	void UpdatePlayerInfo(const cCharacter & info);		// �÷��̾� ����ȭ	

	// ä�� ������Ʈ
	bool bIsChatNeedUpdate;
	const string* chat;
	void UpdateChat();
	
	// �� �÷��̾� ����
	bool bNewPlayerEntered;
	cCharactersInfo * NewPlayer;
	void UpdateNewPlayer();
};
