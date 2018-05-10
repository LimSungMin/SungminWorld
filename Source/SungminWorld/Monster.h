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
	float	Health;			// ü��
	int		Id;				// ���� id

	// ���͸� ��ġ�� �̵�
	void MoveToLocation(const FVector& dest);
	// Ÿ�� �ִϸ��̼� ���
	UFUNCTION(BlueprintImplementableEvent)
	void PlayAttackMontage();
	UFUNCTION(BlueprintImplementableEvent)
	void Dead();
	UFUNCTION(BlueprintImplementableEvent)
	void HitReact();
};
