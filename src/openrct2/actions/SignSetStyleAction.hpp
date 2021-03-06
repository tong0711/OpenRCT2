/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../Context.h"
#include "../core/MemoryStream.h"
#include "../drawing/Drawing.h"
#include "../localisation/StringIds.h"
#include "../ui/UiContext.h"
#include "../windows/Intent.h"
#include "../world/Banner.h"
#include "../world/Scenery.h"
#include "../world/Sprite.h"
#include "GameAction.h"

DEFINE_GAME_ACTION(SignSetStyleAction, GAME_COMMAND_SET_SIGN_STYLE, GameActionResult)
{
private:
    int32_t _bannerIndex;
    uint8_t _mainColour;
    uint8_t _textColour;
    bool _isLarge;

public:
    SignSetStyleAction() = default;
    SignSetStyleAction(int32_t bannerIndex, uint8_t mainColour, uint8_t textColour, bool isLarge)
        : _bannerIndex(bannerIndex)
        , _mainColour(mainColour)
        , _textColour(textColour)
        , _isLarge(isLarge)
    {
    }

    uint16_t GetActionFlags() const override
    {
        return GameAction::GetActionFlags() | GA_FLAGS::ALLOW_WHILE_PAUSED;
    }

    void Serialise(DataSerialiser & stream) override
    {
        GameAction::Serialise(stream);
        stream << DS_TAG(_bannerIndex) << DS_TAG(_mainColour) << DS_TAG(_textColour) << DS_TAG(_isLarge);
    }

    GameActionResult::Ptr Query() const override
    {
        if ((BannerIndex)_bannerIndex >= MAX_BANNERS || _bannerIndex < 0)
        {
            log_warning("Invalid game command for setting sign style, banner id '%d' out of range", _bannerIndex);
            return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
        }

        rct_banner* banner = &gBanners[_bannerIndex];

        CoordsXY coords{ banner->x * 32, banner->y * 32 };

        if (_isLarge)
        {
            TileElement* tileElement = banner_get_tile_element((BannerIndex)_bannerIndex);
            if (tileElement == nullptr)
            {
                log_warning("Invalid game command for setting sign style, banner id '%d' not found", _bannerIndex);
                return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
            }
            if (tileElement->GetType() != TILE_ELEMENT_TYPE_LARGE_SCENERY)
            {
                log_warning("Invalid game command for setting sign style, banner id '%d' is not large", _bannerIndex);
                return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
            }
        }
        else
        {
            TileElement* tileElement = map_get_first_element_at(coords.x / 32, coords.y / 32);
            bool wallFound = false;
            do
            {
                if (tileElement->GetType() != TILE_ELEMENT_TYPE_WALL)
                    continue;

                rct_scenery_entry* scenery_entry = tileElement->AsWall()->GetEntry();
                if (scenery_entry->wall.scrolling_mode == SCROLLING_MODE_NONE)
                    continue;
                if (tileElement->AsWall()->GetBannerIndex() != (BannerIndex)_bannerIndex)
                    continue;
                wallFound = true;
                break;
            } while (!(tileElement++)->IsLastForTile());

            if (!wallFound == false)
            {
                log_warning("Invalid game command for setting sign style, banner id '%d' not found", _bannerIndex);
                return MakeResult(GA_ERROR::INVALID_PARAMETERS, STR_NONE);
            }
        }

        return MakeResult();
    }

    GameActionResult::Ptr Execute() const override
    {
        rct_banner* banner = &gBanners[_bannerIndex];

        CoordsXY coords{ banner->x * 32, banner->y * 32 };

        if (_isLarge)
        {
            TileElement* tileElement = banner_get_tile_element((BannerIndex)_bannerIndex);
            if (!sign_set_colour(
                    coords.x, coords.y, tileElement->base_height, tileElement->GetDirection(),
                    tileElement->AsLargeScenery()->GetSequenceIndex(), _mainColour, _textColour))
            {
                return MakeResult(GA_ERROR::UNKNOWN, STR_NONE);
            }
        }
        else
        {
            TileElement* tileElement = map_get_first_element_at(coords.x / 32, coords.y / 32);
            tileElement->AsWall()->SetPrimaryColour(_mainColour);
            tileElement->AsWall()->SetSecondaryColour(_textColour);
            map_invalidate_tile(coords.x, coords.y, tileElement->base_height * 8, tileElement->clearance_height * 8);
        }

        auto intent = Intent(INTENT_ACTION_UPDATE_BANNER);
        intent.putExtra(INTENT_EXTRA_BANNER_INDEX, (BannerIndex)_bannerIndex);
        context_broadcast_intent(&intent);

        return MakeResult();
    }
};
