// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Monster.generated.h"

UCLASS()
class SUNGMINWORLD_API AMonster : public ACharacter
{
	GENERATED_BODY()

public:	
	float	Health;			// 체력
	int		Id;				// 고유 id

	// 몬스터를 위치로 이동
	void MoveToLocation(const FVector& dest);
	// 타격 애니메이션 재생
	UFUNCTION(BlueprintImplementableEvent)
	void PlayAttackMontage();
	UFUNCTION(BlueprintImplementableEvent)
	void Dead();
	UFUNCTION(BlueprintImplementableEvent)
	void HitReact();
};
