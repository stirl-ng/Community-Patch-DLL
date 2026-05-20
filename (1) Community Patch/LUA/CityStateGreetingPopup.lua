-------------------------------------------------
-- City State Greeting Popup
-- Auto-close version for LLM integration
-------------------------------------------------
include( "IconSupport" );
include( "InfoTooltipInclude" );
include( "CityStateStatusHelper" );

local m_PopupInfo = nil;
local lastBackgroundImage = "citystatebackgroundculture.dds"

-- Auto-close configuration for LLM integration
local AUTO_CLOSE_SECONDS = 3.0
local g_autoCloseTimer = 0

-------------------------------------------------
-------------------------------------------------
function OnPopup( popupInfo )

	local bGreeting = popupInfo.Type == ButtonPopupTypes.BUTTONPOPUP_CITY_STATE_GREETING;

	if (not bGreeting) then
		return;
	end

	--------------------------
	-- City State saying hi for the first time
	--------------------------

	m_PopupInfo = popupInfo;

	local iPlayer = popupInfo.Data1;
	local pPlayer = Players[iPlayer];

	local strNameKey = pPlayer:GetCivilizationShortDescriptionKey();

	local strTitle = "";
	local strDescription = "";

	-- Set Title Icon
	local sMinorCivType = pPlayer:GetMinorCivType();
	local trait = GameInfo.MinorCivilizations[sMinorCivType].MinorCivTrait;
	Controls.TitleIcon:SetTexture(GameInfo.MinorCivTraits[trait].TraitTitleIcon);

	-- Set Background Image
	lastBackgroundImage = GameInfo.MinorCivTraits[trait].BackgroundImage;
	Controls.BackgroundImage:SetTexture(lastBackgroundImage);

	-- Update colors
	local primaryColor, secondaryColor = pPlayer:GetPlayerColors();
	primaryColor, secondaryColor = secondaryColor, primaryColor;
	local textColor = {x = primaryColor.x, y = primaryColor.y, z = primaryColor.z, w = 1};

	civType = pPlayer:GetCivilizationType();
	civInfo = GameInfo.Civilizations[civType];

	local iconColor = textColor;
	IconHookup( civInfo.PortraitIndex, 32, civInfo.AlphaIconAtlas, Controls.CivIcon );
	Controls.CivIcon:SetColor(iconColor);

	local strShortDescKey = pPlayer:GetCivilizationShortDescriptionKey();

	-- Title
	strTitle = Locale.ConvertTextKey("{" .. strShortDescKey.. ":upper}");

	local iActivePlayer = Game.GetActivePlayer();

	-- Greeting popup - don't show status or quests here
	Controls.CityStateMeterThingy:SetHide(true);
	Controls.QuestLabel:SetHide(true);

	-- Info on their Trait
	local strTraitText = GetCityStateTraitText(iPlayer);
	local strTraitTT = GetCityStateTraitToolTip(iPlayer);

	strTraitText = "[COLOR_POSITIVE_TEXT]" .. strTraitText .. "[ENDCOLOR]";

	Controls.TraitInfo:SetText(strTraitText);
	Controls.TraitInfo:SetToolTipString(strTraitTT);
	Controls.TraitLabel:SetToolTipString(strTraitTT);

	-- Personality
	local strPersonalityKey = "";
	local strPersonalityText = "";
	local strPersonalityTT = "";
	local iPersonality = pPlayer:GetPersonality();
	if (iPersonality == MinorCivPersonalityTypes.MINOR_CIV_PERSONALITY_FRIENDLY) then
		strPersonalityKey = "TXT_KEY_CITY_STATE_PERSONALITY_FRIENDLY"
	elseif (iPersonality == MinorCivPersonalityTypes.MINOR_CIV_PERSONALITY_NEUTRAL) then
		strPersonalityKey = "TXT_KEY_CITY_STATE_PERSONALITY_NEUTRAL"
	elseif (iPersonality == MinorCivPersonalityTypes.MINOR_CIV_PERSONALITY_HOSTILE) then
		strPersonalityKey = "TXT_KEY_CITY_STATE_PERSONALITY_HOSTILE"
	elseif (iPersonality == MinorCivPersonalityTypes.MINOR_CIV_PERSONALITY_IRRATIONAL) then
		strPersonalityKey = "TXT_KEY_CITY_STATE_PERSONALITY_IRRATIONAL"
	end

	strPersonalityText = "[COLOR_POSITIVE_TEXT]" .. Locale.ConvertTextKey(strPersonalityKey) .. "[ENDCOLOR]";

	strPersonalityTT = Locale.ConvertTextKey(strPersonalityKey .. "_TT");

	Controls.PersonalityInfo:SetText(strPersonalityText);
	Controls.PersonalityInfo:SetToolTipString(strPersonalityTT);
	Controls.PersonalityLabel:SetToolTipString(strPersonalityTT);

	-- Ally Status
	local iAlly = pPlayer:GetAlly();
	local bHideIcon = true;
	local bHideText = true;
	if (iAlly ~= nil and iAlly ~= -1) then
		if (iAlly ~= iActivePlayer) then
			if (Teams[Players[iAlly]:GetTeam()]:IsHasMet(Game.GetActiveTeam())) then
				bHideIcon = false;
				CivIconHookup(iAlly, 32, Controls.AllyIcon, Controls.AllyIconBG, Controls.AllyIconShadow, false, true);
			else
				bHideIcon = false;
				CivIconHookup(-1, 32, Controls.AllyIcon, Controls.AllyIconBG, Controls.AllyIconShadow, false, true);
			end
		else
			bHideText = false;
		end
	else
		bHideText = false;
	end
	local strAlly = GetAllyText(iActivePlayer, iPlayer);
	Controls.AllyText:SetText(strAlly);
	local strAllyTT = GetAllyToolTip(iActivePlayer, iPlayer);
	Controls.AllyIcon:SetToolTipString(strAllyTT);
	Controls.AllyIconBG:SetToolTipString(strAllyTT);
	Controls.AllyIconShadow:SetToolTipString(strAllyTT);
	Controls.AllyText:SetToolTipString(strAllyTT);
	Controls.AllyLabel:SetToolTipString(strAllyTT);
	Controls.AllyIconContainer:SetHide(bHideIcon);
	Controls.AllyText:SetHide(bHideText);

	-- Protected by anyone?
	local sProtectingPlayers = getProtectingPlayers(iPlayer);

	if (sProtectingPlayers ~= "") then
		Controls.ProtectInfo:SetText("[COLOR_POSITIVE_TEXT]" .. sProtectingPlayers .. "[ENDCOLOR]");
	else
		Controls.ProtectInfo:SetText(Locale.ConvertTextKey("TXT_KEY_CITY_STATE_NOBODY"));
	end

	Controls.ProtectInfo:SetToolTipString(Locale.ConvertTextKey("TXT_KEY_POP_CSTATE_PROTECTED_BY_TT"));
	Controls.ProtectLabel:SetToolTipString(Locale.ConvertTextKey("TXT_KEY_POP_CSTATE_PROTECTED_BY_TT"));

	-- Nearby Resources
	local pCapital = pPlayer:GetCapitalCity();
	if (pCapital ~= nil) then

		local strResourceText = "";

		local iNumResourcesFound = 0;

		local thisX = pCapital:GetX();
		local thisY = pCapital:GetY();

		local iRange = GameDefines["MINOR_CIV_RESOURCE_SEARCH_RADIUS"]; --5
		local iCloseRange = math.floor(iRange/2); --2
		local tResourceList = {};

		for iDX = -iRange, iRange, 1 do
			for iDY = -iRange, iRange, 1 do
				local pTargetPlot = Map.GetPlotXY(thisX, thisY, iDX, iDY);

				if pTargetPlot ~= nil then

					local iOwner = pTargetPlot:GetOwner();

					if (iOwner == iPlayer or iOwner == -1) then
						local plotX = pTargetPlot:GetX();
						local plotY = pTargetPlot:GetY();
						local plotDistance = Map.PlotDistance(thisX, thisY, plotX, plotY);

						if (plotDistance <= iRange and (plotDistance <= iCloseRange or iOwner == iPlayer)) then

							local iResourceType = pTargetPlot:GetResourceType(Game.GetActiveTeam());

							if (iResourceType ~= -1) then

								if (Game.GetResourceUsageType(iResourceType) ~= ResourceUsageTypes.RESOURCEUSAGE_BONUS) then

									if (tResourceList[iResourceType] == nil) then
										tResourceList[iResourceType] = 0;
									end

									tResourceList[iResourceType] = tResourceList[iResourceType] + pTargetPlot:GetNumResource();

								end
							end
						end
					end

				end
			end
		end

		for iResourceType, iAmount in pairs(tResourceList) do
			if (iNumResourcesFound > 0) then
				strResourceText = strResourceText .. ", ";
			end
			local pResource = GameInfo.Resources[iResourceType];
			strResourceText = strResourceText .. pResource.IconString .. " [COLOR_POSITIVE_TEXT]" .. Locale.ConvertTextKey(pResource.Description) .. " (" .. iAmount .. ") [ENDCOLOR]";
			iNumResourcesFound = iNumResourcesFound + 1;
		end

		Controls.ResourcesInfo:SetText(strResourceText);

		Controls.ResourcesLabel:SetHide(false);
		Controls.ResourcesInfo:SetHide(false);

		local strResourceTextTT = Locale.ConvertTextKey("TXT_KEY_CITY_STATE_RESOURCES_TT");
		Controls.ResourcesInfo:SetToolTipString(strResourceTextTT);
		Controls.ResourcesLabel:SetToolTipString(strResourceTextTT);

	else
		Controls.ResourcesLabel:SetHide(true);
		Controls.ResourcesInfo:SetHide(true);
	end

	-- Gifts
	local iGoldGift = popupInfo.Data2;
	local iFaithGift = popupInfo.Data3;
	local bFirstMajorCiv = popupInfo.Option1;
	local strGiftString = "";

	if (iGoldGift > 0) then
		if (bFirstMajorCiv) then
			strGiftString = strGiftString .. Locale.ConvertTextKey("TXT_KEY_CITY_STATE_GIFT_FIRST", iGoldGift);
		else
			strGiftString = strGiftString .. Locale.ConvertTextKey("TXT_KEY_CITY_STATE_GIFT_OTHER", iGoldGift);
		end
	end

	if (iFaithGift > 0) then
		if (iGoldGift > 0) then
			strGiftString = strGiftString .. " ";
		end

		if (bFirstMajorCiv) then
			strGiftString = strGiftString .. Locale.ConvertTextKey("TXT_KEY_CITY_STATE_GIFT_FAITH_FIRST", iFaithGift);
		else
			strGiftString = strGiftString .. Locale.ConvertTextKey("TXT_KEY_CITY_STATE_GIFT_FAITH_OTHER", iFaithGift);
		end
	end

	local strSpeakAgainString = Locale.ConvertTextKey("TXT_KEY_MINOR_SPEAK_AGAIN", strNameKey);

	strDescription = Locale.ConvertTextKey("TXT_KEY_CITY_STATE_MEETING", strNameKey, strGiftString, strSpeakAgainString);

	Controls.TitleLabel:SetText(strTitle);
	Controls.TitleLabel:SetColor(textColor, 0);
	Controls.DescriptionLabel:SetText(strDescription);

	UIManager:QueuePopup( ContextPtr, PopupPriority.CityStateGreeting );
end
Events.SerialEventGameMessagePopup.Add( OnPopup );

----------------------------------------------------------------
----------------------------------------------------------------
function getProtectingPlayers(iMinorCivID)
	local sProtecting = "";

	for iPlayerLoop = 0, GameDefines.MAX_MAJOR_CIVS-1, 1 do
		pOtherPlayer = Players[iPlayerLoop];

		if (iPlayerLoop ~= Game.GetActivePlayer()) then
			if (pOtherPlayer:IsAlive()) then
				if (pOtherPlayer:IsProtectingMinor(iMinorCivID)) then
					if (sProtecting ~= "") then
						sProtecting = sProtecting .. ", "
					end

					sProtecting = sProtecting .. Locale.ConvertTextKey(Players[iPlayerLoop]:GetCivilizationShortDescriptionKey());
				end
			end
		end
	end

	return sProtecting
end

----------------------------------------------------------------
-- Input processing
----------------------------------------------------------------
function OnCloseButtonClicked ()
	UIManager:DequeuePopup( ContextPtr );
end
Controls.CloseButton:RegisterCallback( Mouse.eLClick, OnCloseButtonClicked );
Controls.ScreenButton:RegisterCallback( Mouse.eRClick, OnCloseButtonClicked );

----------------------------------------------------------------
-- Find On Map
----------------------------------------------------------------
function OnFindOnMapButtonClicked ()
	local iPlayer = m_PopupInfo.Data1;
	local pPlayer = Players[iPlayer];
	if (pPlayer) then
		local pCity = pPlayer:GetCapitalCity();
		if (pCity) then
			local pPlot = pCity:Plot();
			if (pPlot) then
				UI.LookAt(pPlot, 0);
			end
		end
	end
end
Controls.FindOnMapButton:RegisterCallback( Mouse.eLClick, OnFindOnMapButtonClicked );


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
function InputHandler( uiMsg, wParam, lParam )
	if uiMsg == KeyEvents.KeyDown then
		if wParam == Keys.VK_ESCAPE or wParam == Keys.VK_RETURN then
			OnCloseButtonClicked();
			return true;
		end
	end
end
ContextPtr:SetInputHandler( InputHandler );


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
function ShowHideHandler( bIsHide, bInitState )
	if( not bInitState ) then
		Controls.BackgroundImage:UnloadTexture();
		if( not bIsHide ) then
			Controls.BackgroundImage:SetTexture(lastBackgroundImage);
			-- No semaphore: this popup is informational; the LLM should not be blocked from ending the turn
			Events.SerialEventGameMessagePopupShown(m_PopupInfo);
			-- Reset auto-close timer when popup is shown
			g_autoCloseTimer = 0
		else
			Events.SerialEventGameMessagePopupProcessed.CallImmediate(m_PopupInfo.Type, 0);
		end
	end
end
ContextPtr:SetShowHideHandler( ShowHideHandler );

-------------------------------------------------
-- Auto-close timer for LLM integration
-------------------------------------------------
ContextPtr:SetUpdate(function(fDTime)
	if not ContextPtr:IsHidden() then
		g_autoCloseTimer = g_autoCloseTimer + fDTime
		if g_autoCloseTimer >= AUTO_CLOSE_SECONDS then
			g_autoCloseTimer = 0
			OnCloseButtonClicked()
		end
	end
end)

----------------------------------------------------------------
-- 'Active' (local human) player has changed
----------------------------------------------------------------
Events.GameplaySetActivePlayer.Add(OnCloseButtonClicked);
