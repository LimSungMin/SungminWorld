// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SungminWorldCharacter.h"
#include "OtherNetworkCharacter.generated.h"

/**
 * 
 */
UCLASS()
class SUNGMINWORLD_API AOtherNetworkCharacter : public ASungminWorldCharacter
{
	GENERATED_BODY()
public:	
	AOtherNetworkCharacter();
	
	UFUNCTION(BlueprintCallable)
	bool GetFalling();

// 	UFUNCTION(BlueprintCallable)
// 	FVector GetVelocity();

	void SetFalling(bool IsFall);
	void SetVelocity(FVector velocity);

private:	
	bool	bIsFalling;
	FVector Velocity;
};
