// Fill out your copyright notice in the Description page of Project Settings.

#include "LoginGameMode.h"
#include "Blueprint/UserWidget.h"

void ALoginGameMode::BeginPlay()
{
	if (HUDWidgetClass != nullptr)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
		if (CurrentWidget != nullptr)
		{
			CurrentWidget->AddToViewport();
		}
	}

	Socket = ClientSocket::GetSingleton();
	Socket->InitSocket();
	bIsConnected = Socket->Connect("127.0.0.1", 8000);
	if (bIsConnected)
	{
		UE_LOG(LogClass, Log, TEXT("IOCP Server connect success!"));		
	}
}

bool ALoginGameMode::Login(const FText & Id, const FText & Pw)
{
	if (Id.IsEmpty() || Pw.IsEmpty())
		return false;

	if (!bIsConnected)
		return false;

	bool IsSuccess = Socket->Login(Id, Pw);

	if (!IsSuccess)
		return false;
	
	Socket->CloseSocket();
	return true;
}

bool ALoginGameMode::SignUp(const FText & Id, const FText & Pw)
{
	if (Id.IsEmpty() || Pw.IsEmpty())
		return false;

	if (!bIsConnected)
		return false;

	bool IsSuccess = Socket->SignUp(Id, Pw);

	if (!IsSuccess)
		return false;
	
	return true;
}
