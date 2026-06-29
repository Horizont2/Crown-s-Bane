// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Centralized visual design system for the Crown's Bane HUD.
 *
 * All colours, font scales, spacing units, and panel-drawing helpers live
 * here so changes propagate across the entire HUD by editing this one file.
 * Built on top of AHUD::Canvas — no UMG required for the core in-game HUD.
 */
namespace CrownStyle
{
	// ---- Palette ----
	// Backgrounds
	inline const FLinearColor BgDark     = FLinearColor(0.05f, 0.05f, 0.07f, 0.85f); // panel back
	inline const FLinearColor BgLight    = FLinearColor(0.12f, 0.10f, 0.08f, 0.85f); // row back
	inline const FLinearColor BgOverlay  = FLinearColor(0.00f, 0.00f, 0.00f, 0.55f); // full-screen dim
	inline const FLinearColor BgTransparent = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Accents
	inline const FLinearColor AccentGold   = FLinearColor(1.00f, 0.78f, 0.20f, 1.0f);
	inline const FLinearColor AccentAmber  = FLinearColor(1.00f, 0.55f, 0.10f, 1.0f);
	inline const FLinearColor AccentSilver = FLinearColor(0.85f, 0.85f, 0.90f, 1.0f);

	// Semantic
	inline const FLinearColor Danger    = FLinearColor(0.95f, 0.20f, 0.20f, 1.0f);
	inline const FLinearColor Warning   = FLinearColor(1.00f, 0.70f, 0.15f, 1.0f);
	inline const FLinearColor Success   = FLinearColor(0.30f, 0.90f, 0.40f, 1.0f);
	inline const FLinearColor Info      = FLinearColor(0.40f, 0.75f, 1.00f, 1.0f);

	// Text
	inline const FColor TextPrimary    = FColor(245, 240, 220);
	inline const FColor TextSecondary  = FColor(200, 200, 200);
	inline const FColor TextDim        = FColor(150, 150, 150);
	inline const FColor TextGold       = FColor(255, 220, 100);

	// ---- Typography scale ----
	inline constexpr float ScaleDisplay  = 1.5f; // section titles, banners
	inline constexpr float ScaleHeading  = 1.2f; // panel titles, hotkey labels
	inline constexpr float ScaleBody     = 1.0f; // default
	inline constexpr float ScaleCaption  = 0.85f; // hint text, distance readouts

	// ---- Spacing scale (4/8/16/24/32 grid) ----
	inline constexpr float Sp1 = 4.0f;
	inline constexpr float Sp2 = 8.0f;
	inline constexpr float Sp3 = 16.0f;
	inline constexpr float Sp4 = 24.0f;
	inline constexpr float Sp5 = 32.0f;

	// ---- Common panel sizes ----
	inline constexpr float PanelBorderThickness = 1.5f;
	inline constexpr float PanelHighlightThickness = 2.5f;

	// Panel styles — used by DrawPanel helper in HUD.cpp.
	enum class EPanelStyle : uint8
	{
		Primary,   // gold border, standard panel
		Subtle,    // no border, dim background
		Danger,    // red border, alarm
		Success,   // green border, completion
		Highlight  // thick gold border, callout
	};

	inline FLinearColor BorderForStyle(EPanelStyle Style)
	{
		switch (Style)
		{
		case EPanelStyle::Subtle:    return BgTransparent;
		case EPanelStyle::Danger:    return Danger;
		case EPanelStyle::Success:   return Success;
		case EPanelStyle::Highlight: return AccentGold;
		case EPanelStyle::Primary:
		default:                     return AccentGold * FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);
		}
	}

	inline float ThicknessForStyle(EPanelStyle Style)
	{
		return (Style == EPanelStyle::Highlight) ? PanelHighlightThickness : PanelBorderThickness;
	}

	// Smooth pulse 0..1 (for low-HP warnings, ready-to-fire glows).
	inline float Pulse(float Time, float Hz = 1.0f)
	{
		return 0.5f + 0.5f * FMath::Sin(Time * Hz * 2.0f * PI);
	}
}
