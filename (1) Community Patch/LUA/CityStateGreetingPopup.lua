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

	m_PopupInfo = popupInfo;

	-- Skip display: purely informational; queuing would block end_turn while popup sits in queue.
	-- LLM queries city-state info via game state tools rather than reading this popup.
	Events.SerialEventGameMessagePopupProcessed.CallImmediate(ButtonPopupTypes.BUTTONPOPUP_CITY_STATE_GREETING, 0);
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
