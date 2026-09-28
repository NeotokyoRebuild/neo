#include "neo_avatar.h"

#include "cbase.h"
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include "VGuiMatSurface/IMatSystemSurface.h"

struct CacheAvatarValue
{
	int normal;
	int dead;
};

struct CacheAvatarKey
{
	CSteamID m_SteamID;
	int m_iAvatar = 0;

	bool operator<(const CacheAvatarKey &rhs) const
	{
		return m_SteamID.ConvertToUint64() < rhs.m_SteamID.ConvertToUint64()
			|| (m_SteamID.ConvertToUint64() == rhs.m_SteamID.ConvertToUint64()
					&& m_iAvatar < rhs.m_iAvatar);
	}
};

static inline CUtlMap<CacheAvatarKey, CacheAvatarValue> gAvatarImageCache;
static inline bool gAvatarImageCacheInit = false;

void NeoAvatar::SetSteamID(const CSteamID &steamID)
{
	if (steamID == m_SteamID)
	{
		return;
	}
	m_SteamID = steamID;
	m_iTextureID = 0;
	m_iTextureDeadID = 0;
	m_flPrevLoadAttempt = 0.0f;
}

void NeoAvatar::Fetch(const int iAvatarWH)
{
	if (!m_SteamID.IsValid()
			|| !SteamFriends()
			|| !SteamUtils()
			|| (m_iTextureID > 0 && m_iTexForAvatarWH == iAvatarWH)
			|| (m_flPrevLoadAttempt + 1.0f) > gpGlobals->realtime)
	{
		return;
	}

	if (!SteamFriends()->RequestUserInformation(m_SteamID, false))
	{
		int iAvatar = 0;
		if (iAvatarWH <= 32)
		{
			iAvatar = SteamFriends()->GetSmallFriendAvatar(m_SteamID);
		}
		else if (iAvatarWH <= 64)
		{
			iAvatar = SteamFriends()->GetMediumFriendAvatar(m_SteamID);
		}
		else
		{
			iAvatar = SteamFriends()->GetLargeFriendAvatar(m_SteamID);
		}
		m_iTexForAvatarWH = iAvatarWH;

		if (iAvatar > 0)
		{
			if (!gAvatarImageCacheInit)
			{
				SetDefLessFunc(gAvatarImageCache);
				gAvatarImageCacheInit = true;
			}

			int iTexIndex = gAvatarImageCache.Find(CacheAvatarKey{m_SteamID, iAvatar});
			if (iTexIndex == gAvatarImageCache.InvalidIndex())
			{
				uint32 u32wide = 0, u32tall = 0;
				if (SteamUtils()->GetImageSize(iAvatar, &u32wide, &u32tall)
						&& u32wide > 0 && u32tall > 0)
				{
					const int wide = u32wide;
					const int tall = u32tall;
					const int destBufferSize = wide * tall * 4;
					byte *rgbaBuf = (byte *)stackalloc(destBufferSize);
					if (SteamUtils()->GetImageRGBA(iAvatar, rgbaBuf, destBufferSize))
					{
						// Create normal avatar from RGBA without edits
						m_iTextureID = vgui::surface()->CreateNewTextureID(true);
						g_pMatSystemSurface->DrawSetTextureRGBAEx2(m_iTextureID, rgbaBuf, wide, tall, IMAGE_FORMAT_RGBA8888, true);

						// Create dead avatar from RGBA with redness edits
						for (int offset = 0; offset < (wide * tall * 4); offset += 4)
						{
							constexpr float brightness = 0.5f;
							constexpr float contrast = 1.5f;

							float r = (rgbaBuf + offset)[0] / 255.0f;
							float g = (rgbaBuf + offset)[1] / 255.0f;
							float b = (rgbaBuf + offset)[2] / 255.0f;

							// Contrast
							r = Clamp((r - 0.5f) * contrast + 0.5f, 0.0f, 1.0f);
							g = Clamp((g - 0.5f) * contrast + 0.5f, 0.0f, 1.0f);
							b = Clamp((b - 0.5f) * contrast + 0.5f, 0.0f, 1.0f);

							// Convert to grayscale - Luminosity, then gradient from black -> red -> white
							const float gray = 0.3f * r + 0.59f * g  + 0.11f * b;
							if (gray < 0.5)
							{
								r = gray * 2.0f;
								g = 0;
								b = 0;
							}
							else {
								r = 1.0f;
								g = (gray - 0.5f) * 2.0f;
								b = (gray - 0.5f) * 2.0f;
							}

							// Brightness
							r = Clamp(r * brightness, 0.0f, 1.0f);
							g = Clamp(g * brightness, 0.0f, 1.0f);
							b = Clamp(b * brightness, 0.0f, 1.0f);

							(rgbaBuf + offset)[0] = r * 255;
							(rgbaBuf + offset)[1] = g * 255;
							(rgbaBuf + offset)[2] = b * 255;
						}

						m_iTextureDeadID = vgui::surface()->CreateNewTextureID(true);
						g_pMatSystemSurface->DrawSetTextureRGBAEx2(m_iTextureDeadID, rgbaBuf, wide, tall, IMAGE_FORMAT_RGBA8888, true);

						// Add textures to global cache
						iTexIndex = gAvatarImageCache.Insert(CacheAvatarKey{m_SteamID, iAvatar});
						gAvatarImageCache[iTexIndex].normal = m_iTextureID;
						gAvatarImageCache[iTexIndex].dead = m_iTextureDeadID;
					}
					stackfree(rgbaBuf);
				}
			}
			else
			{
				m_iTextureID = gAvatarImageCache[iTexIndex].normal;
				m_iTextureDeadID = gAvatarImageCache[iTexIndex].dead;
			}
		}
	}

	if (m_iTextureID == 0)
	{
		m_flPrevLoadAttempt = gpGlobals->realtime;
	}
}

void NeoAvatar::Paint(const int x, const int y, const int widetall,
		const bool bDead) const
{
	if (m_iTextureID == 0)
	{
		return;
	}
	vgui::surface()->DrawSetTexture(bDead ? m_iTextureDeadID : m_iTextureID);
	vgui::surface()->DrawSetColor(COLOR_WHITE);
	vgui::surface()->DrawTexturedRect(x, y, x + widetall, y + widetall);
}

