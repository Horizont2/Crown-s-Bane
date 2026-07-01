// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IntroSequence.generated.h"

/**
 * Drop this actor in the starting level.  It BeginPlay-waits IntroDelay
 * seconds, then fires a sequence of banners telling the setup story of
 * Crown's Bane so new players have context for what they're doing.
 *
 * Once seen, sets a persistent flag in GameInstance so subsequent
 * sessions skip the intro.
 */
UCLASS()
class CROWNSBANE_API AIntroSequence : public AActor
{
	GENERATED_BODY()

public:
	AIntroSequence();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro")
	float IntroDelay = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro")
	float StepDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro")
	TArray<FString> Lines;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Intro")
	bool bAlwaysShow = false;
};
