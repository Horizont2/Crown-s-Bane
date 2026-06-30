// Copyright 2024 Crown's Bane. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Centralized visual design system for the Crown's Bane HUD — pirate / naval
 * tone with parchment + weathered brass + rust accents.  All HUD elements pull
 * colours, spacing, scales, and panel styles from this header so updating the
 * theme is a one-file change.
 */
namespace CrownStyle
{
	// ---- Palette (warmer pirate / parchment tones) ----
	// Backgrounds — deep navy-charcoal with subtle warm hue, not flat black.
	inline const FLinearColor BgDark     = FLinearColor(0.043f, 0.062f, 0.094f, 0.92f); // #0B1018 panel back
	inline const FLinearColor BgLight    = FLinearColor(0.086f, 0.067f, 0.039f, 0.88f); // weathered parchment dark
	inline const FLinearColor BgGradTop  = FLinearColor(0.094f, 0.118f, 0.157f, 0.95f); // panel top
	inline const FLinearColor BgGradBot  = FLinearColor(0.027f, 0.039f, 0.063f, 0.95f); // panel bottom
	inline const FLinearColor BgOverlay  = FLinearColor(0.0f,   0.0f,   0.0f,   0.62f); // full-screen dim
	inline const FLinearColor BgTransparent = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Border layers for depth (outer dark + inner highlight).
	inline const FLinearColor BorderOuter   = FLinearColor(0.012f, 0.020f, 0.031f, 1.0f);
	inline const FLinearColor BorderInner   = FLinearColor(0.20f,  0.16f,  0.10f, 0.85f);

	// Accents — mellow brass / rust / aged silver.
	inline const FLinearColor AccentGold   = FLinearColor(0.851f, 0.643f, 0.255f, 1.0f); // #D9A441 brass
	inline const FLinearColor AccentGoldBright = FLinearColor(1.0f, 0.84f, 0.40f, 1.0f);
	inline const FLinearColor AccentAmber  = FLinearColor(0.875f, 0.451f, 0.137f, 1.0f); // burnt orange
	inline const FLinearColor AccentSilver = FLinearColor(0.706f, 0.722f, 0.745f, 1.0f); // aged pewter
	inline const FLinearColor AccentTeal   = FLinearColor(0.243f, 0.553f, 0.561f, 1.0f); // sea teal

	// Semantic
	inline const FLinearColor Danger    = FLinearColor(0.761f, 0.224f, 0.180f, 1.0f); // #C2392E rust-red
	inline const FLinearColor Warning   = FLinearColor(0.945f, 0.624f, 0.137f, 1.0f);
	inline const FLinearColor Success   = FLinearColor(0.357f, 0.631f, 0.373f, 1.0f); // sage
	inline const FLinearColor Info      = FLinearColor(0.353f, 0.659f, 0.831f, 1.0f);

	// Text — warmer cream rather than pure white.
	inline const FColor TextPrimary    = FColor(238, 226, 198);   // parchment cream
	inline const FColor TextSecondary  = FColor(196, 188, 172);
	inline const FColor TextDim        = FColor(140, 132, 118);
	inline const FColor TextGold       = FColor(217, 164, 65);    // brass-on-canvas

	// Shadows
	inline const FColor TextShadow     = FColor(0, 0, 0, 200);

	// ---- Typography scale ----
	inline constexpr float ScaleDisplay  = 1.7f;
	inline constexpr float ScaleHeading  = 1.25f;
	inline constexpr float ScaleBody     = 1.0f;
	inline constexpr float ScaleCaption  = 0.85f;

	// ---- Spacing scale (4/8/16/24/32 grid) ----
	inline constexpr float Sp1 = 4.0f;
	inline constexpr float Sp2 = 8.0f;
	inline constexpr float Sp3 = 16.0f;
	inline constexpr float Sp4 = 24.0f;
	inline constexpr float Sp5 = 32.0f;

	// ---- Borders ----
	inline constexpr float PanelBorderThickness = 2.0f;
	inline constexpr float PanelHighlightThickness = 3.0f;
	inline constexpr float RivetSize = 5.0f;

	enum class EPanelStyle : uint8
	{
		Primary,   // brass border, standard panel
		Subtle,    // soft border, dim background
		Danger,    // red border, alarm
		Success,   // green border, completion
		Highlight  // thick gold border + rivets, callout
	};

	inline FLinearColor BorderForStyle(EPanelStyle Style)
	{
		switch (Style)
		{
		case EPanelStyle::Subtle:    return BorderInner;
		case EPanelStyle::Danger:    return Danger;
		case EPanelStyle::Success:   return Success;
		case EPanelStyle::Highlight: return AccentGoldBright;
		case EPanelStyle::Primary:
		default:                     return AccentGold;
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

	// Smooth ease helpers for animations.
	inline float EaseOutCubic(float T)
	{
		const float U = 1.0f - FMath::Clamp(T, 0.0f, 1.0f);
		return 1.0f - U * U * U;
	}
	inline float EaseInQuad(float T)
	{
		T = FMath::Clamp(T, 0.0f, 1.0f);
		return T * T;
	}
}
