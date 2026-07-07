#pragma once

#include <GlobalScaling.h>
#include <Item.h>
#include <UIDefines.h>
#include <UIRenderUtils.h>

#include <raylib.h>

#include <memory>

namespace Nawia::UI::InventoryRender {

	inline void drawItemSlot(
		const Rectangle slot_rect,
		const bool is_hovered,
		const std::shared_ptr<Item::Item>& item,
		const float slot_padding)
	{
		const float scaled_padding = Core::GlobalScaling::scaled(slot_padding);

		if (is_hovered)
			DrawRectangleRec(slot_rect, withAlpha(COLOR_ACCENT, 0.20f));

		if (!item)
			return;

		const Texture2D icon = item->getIcon();
		if (icon.id <= 0)
			return;

		const Rectangle destination = {
			slot_rect.x + scaled_padding,
			slot_rect.y + scaled_padding,
			slot_rect.width - (scaled_padding * 2.0f),
			slot_rect.height - (scaled_padding * 2.0f)
		};
		DrawTexturePro(icon, {0.0f, 0.0f, static_cast<float>(icon.width), static_cast<float>(icon.height)}, destination, {0, 0}, 0.0f, WHITE);
	}

	inline void drawItemTooltip(
		const Font& font,
		const std::shared_ptr<Item::Item>& item,
		const float x,
		const float y,
		const float font_size)
	{
		if (!item)
			return;

		const char* item_name = item->getName().c_str();
		const Vector2 text_size = MeasureTextEx(font, item_name, font_size, 1.0f);
		const float padding = 12.0f;
		const Rectangle tooltip_rect = {x, y, text_size.x + (padding * 2.0f), text_size.y + (padding * 2.0f)};
		drawPanelFrame(tooltip_rect, 0.98f, 1.0f, COLOR_PANEL_BG, COLOR_ACCENT, 1.0f);
		DrawTextEx(font, item_name, {x + padding, y + padding}, font_size, 1.0f, COLOR_GOLDEN_TEXT);
	}

} // namespace Nawia::UI::InventoryRender
