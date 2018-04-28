// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ClientSocket.h"
#include "LoginGameMode.generated.h"

/**
 * 
 */
UCLASS()
class SUNGMINWORLD_API ALoginGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	
	// HUD ȭ�鿡�� �� ���� Ŭ����
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Properties", Meta = (BlueprintProtect = "true"))
	TSubclassOf<class UUserWidget> HUDWidgetClass;

	// HUD ��ü
	UPROPERTY()
	class UUserWidget* CurrentWidget;	

	virtual void BeginPlay() override;	

	UFUNCTION(BlueprintCallable, Category = "Login")
	bool Login(const FText& Id, const FText& Pw);

	UFUNCTION(BlueprintCallable, Category = "Login")
	bool SignUp(const FText& Id, const FText& Pw);

private:
	ClientSocket * Socket;
	bool bIsConnected;
};
