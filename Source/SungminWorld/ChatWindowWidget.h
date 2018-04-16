// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatWindowWidget.generated.h"

/**
 * 
 */
UCLASS()
class SUNGMINWORLD_API UChatWindowWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent)
	void CallUpdateChat(const FText& Text);	
		
};
