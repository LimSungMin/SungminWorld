// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster.h"
#include "MonsterAIController.h"

void AMonster::MoveToLocation(const FVector& dest)
{
	AMonsterAIController * Controller = Cast<AMonsterAIController>(GetController());
	if (Controller)
	{
		Controller->MoveToLocation(dest);		
	}
}
