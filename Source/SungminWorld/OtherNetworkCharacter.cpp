// Fill out your copyright notice in the Description page of Project Settings.

#include "OtherNetworkCharacter.h"

bool AOtherNetworkCharacter::GetFalling()
{
	return bIsFalling;	
}

// FVector AOtherNetworkCharacter::GetVelocity()
// {
// 	return Velocity;
// }

void AOtherNetworkCharacter::SetFalling(bool IsFall)
{
	bIsFalling = IsFall;
}

void AOtherNetworkCharacter::SetVelocity(FVector velocity)
{
	Velocity = velocity;
}
