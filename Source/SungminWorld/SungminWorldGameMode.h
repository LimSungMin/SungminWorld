// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ClientSocket.h"
#include "SungminWorldGameMode.generated.h"

UCLASS(minimalapi)
class ASungminWorldGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASungminWorldGameMode();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	AActor* FindActorBySessionId(TArray<AActor*> ActorArray, const int& SessionId);

	// HUD ȭ�鿡�� �� ���� Ŭ����
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Properties", Meta = (BlueprintProtect = "true"))
	TSubclassOf<class UUserWidget> HUDWidgetClass;

	// HUD ��ü
	UPROPERTY()
	class UUserWidget* CurrentWidget;

	// ������ų �ٸ� ĳ����
	UPROPERTY(EditAnywhere, Category = "Spawning")
	TSubclassOf<class ACharacter> WhoToSpawn;

private:
	ClientSocket	Socket;			// ������ ������ ����
	bool			bIsConnected;	// ������ ���� ����
	int				SessionId;		// ĳ������ ���� ���� ���̵� (������)		
};



