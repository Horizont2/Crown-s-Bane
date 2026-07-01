// Copyright 2024 Crown's Bane. All Rights Reserved.

#include "Systems/IntroSequence.h"
#include "UI/CrownsBaneHUD.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

AIntroSequence::AIntroSequence()
{
	PrimaryActorTick.bCanEverTick = false;

	// Default 4-line intro.  Override in the level's Details panel.
	Lines = {
		TEXT("Admiral Blackwood left ye for dead on a broken skiff..."),
		TEXT("The Crown branded ye a traitor. The Navy hunts ye still."),
		TEXT("Ye stole a ship. Ye stole a name. Ye stole a chance."),
		TEXT("Time to make them all pay.")
	};
}

void AIntroSequence::BeginPlay()
{
	Super::BeginPlay();

	UWorld* W = GetWorld();
	if (!W) return;
	APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0);
	if (!PC) return;
	ACrownsBaneHUD* HUD = Cast<ACrownsBaneHUD>(PC->GetHUD());
	if (!HUD) return;

	// Queue banners on a timer chain.
	for (int32 i = 0; i < Lines.Num(); ++i)
	{
		const FString Line = Lines[i];
		const float FireAt = IntroDelay + i * StepDelay;
		FTimerHandle H;
		W->GetTimerManager().SetTimer(H, FTimerDelegate::CreateLambda([HUD_Wk = TWeakObjectPtr<ACrownsBaneHUD>(HUD), Line]()
		{
			if (HUD_Wk.IsValid())
			{
				HUD_Wk->ShowBanner(TEXT("CROWN'S BANE"), Line,
					FLinearColor(0.85f, 0.643f, 0.255f, 1.0f), 4.5f);
			}
		}), FireAt, false);
	}
}
