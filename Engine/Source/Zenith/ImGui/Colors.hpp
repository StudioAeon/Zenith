#pragma once
#include <imgui.h>

namespace Zenith::Colors
{
	// Primary theme colors
	constexpr ImU32 Accent           = IM_COL32(255, 138, 48, 255);   // Warm orange
	constexpr ImU32 Highlight        = IM_COL32(99, 162, 255, 255);   // Soft blue
	constexpr ImU32 Secondary        = IM_COL32(138, 99, 255, 255);   // Purple accent
	constexpr ImU32 Success          = IM_COL32(72, 187, 120, 255);   // Fresh green

	// Background variants
	constexpr ImU32 Background       = IM_COL32(24, 26, 31, 255);     // Deep charcoal
	constexpr ImU32 BackgroundDark   = IM_COL32(16, 18, 22, 255);     // Darker variant
	constexpr ImU32 BackgroundLight  = IM_COL32(32, 35, 42, 255);     // Lighter variant
	constexpr ImU32 BackgroundPopup  = IM_COL32(28, 31, 38, 255);     // Modal/popup bg
	constexpr ImU32 PropertyField    = IM_COL32(18, 20, 25, 255);     // Input fields
	constexpr ImU32 GroupHeader      = IM_COL32(40, 44, 52, 255);     // Section headers

	// Titlebar colors
	constexpr ImU32 Titlebar         = IM_COL32(20, 22, 27, 255);     // Main titlebar
	constexpr ImU32 TitlebarActive   = IM_COL32(255, 138, 48, 180);   // Active window
	constexpr ImU32 TitlebarWarning  = IM_COL32(255, 193, 48, 180);   // Warning state
	constexpr ImU32 TitlebarError    = IM_COL32(255, 82, 82, 180);    // Error state

	// Text variants
	constexpr ImU32 Text             = IM_COL32(220, 223, 228, 255);  // Primary text
	constexpr ImU32 TextBright       = IM_COL32(255, 255, 255, 255);  // Emphasized text
	constexpr ImU32 TextMuted        = IM_COL32(142, 148, 158, 255);  // Secondary text
	constexpr ImU32 TextDim          = IM_COL32(96, 102, 112, 255);   // Tertiary text
	constexpr ImU32 TextError        = IM_COL32(255, 106, 106, 255);  // Error text

	// Interactive states
	constexpr ImU32 Selection        = IM_COL32(255, 138, 48, 60);    // Selected items
	constexpr ImU32 SelectionBorder  = IM_COL32(255, 138, 48, 180);  // Selection outline
	constexpr ImU32 Hover            = IM_COL32(99, 162, 255, 30);    // Hover state
	constexpr ImU32 Active           = IM_COL32(99, 162, 255, 80);    // Active/pressed

	// Status indicators
	constexpr ImU32 ValidPrefab      = IM_COL32(72, 187, 120, 255);   // Valid/success
	constexpr ImU32 InvalidPrefab    = IM_COL32(255, 82, 82, 255);    // Invalid/error
	constexpr ImU32 MissingAsset     = IM_COL32(255, 193, 48, 255);   // Warning/missing
	constexpr ImU32 ModifiedAsset    = IM_COL32(138, 99, 255, 255);   // Modified/unsaved

	// Utility function for ImVec4 conversion
	constexpr ImVec4 ToVec4(ImU32 color) {
		return ImGui::ColorConvertU32ToFloat4(color);
	}
}