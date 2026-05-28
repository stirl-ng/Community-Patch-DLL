-------------------------------------------------
-- Choose Goody Hut Reward Popup
-- Modified for LLM integration: auto-selects first valid goody hut bonus
-------------------------------------------------

local AUTO_CLOSE_SECONDS = 1.0
local g_autoCloseTimer = 0

local m_PopupInfo = nil
local pPlayer = nil
local pUnit = nil
local pPlot = nil
local g_selectedGoody = nil

local function SelectFirstValidGoody()
	local iIndex = 0
	for info in GameInfo.GoodyHuts() do
		if pPlayer:CanGetGoody(pPlot, iIndex, pUnit) then
			return iIndex
		end
		iIndex = iIndex + 1
	end
	return nil
end

function OnPopup(popupInfo)
	if popupInfo.Type ~= ButtonPopupTypes.BUTTONPOPUP_CHOOSE_GOODY_HUT_REWARD then
		return
	end
	if not ContextPtr:IsHidden() then
		return
	end

	m_PopupInfo = popupInfo
	pPlayer = Players[popupInfo.Data1]
	pUnit = pPlayer:GetUnitByID(popupInfo.Data2)
	pPlot = pUnit:GetPlot()

	g_selectedGoody = SelectFirstValidGoody()
	g_autoCloseTimer = 0

	ContextPtr:SetHide(false)
end
Events.SerialEventGameMessagePopup.Add(OnPopup)


ContextPtr:SetUpdate(function(fDTime)
	if not ContextPtr:IsHidden() then
		g_autoCloseTimer = g_autoCloseTimer + fDTime
		if g_autoCloseTimer >= AUTO_CLOSE_SECONDS then
			g_autoCloseTimer = 0
			if g_selectedGoody ~= nil then
				Network.SendGoodyChoice(m_PopupInfo.Data1, pPlot:GetX(), pPlot:GetY(), g_selectedGoody, pUnit:GetID())
			end
			ContextPtr:SetHide(true)
		end
	end
end)


function ShowHideHandler(bIsHide, bInitState)
	if not bInitState then
		if not bIsHide then
			UI.incTurnTimerSemaphore()
			Events.SerialEventGameMessagePopupShown(m_PopupInfo)
		else
			UI.decTurnTimerSemaphore()
			Events.SerialEventGameMessagePopupProcessed.CallImmediate(ButtonPopupTypes.BUTTONPOPUP_CHOOSE_GOODY_HUT_REWARD, 0)
		end
	end
end
ContextPtr:SetShowHideHandler(ShowHideHandler)


function OnActivePlayerChanged(iActivePlayer, iPrevActivePlayer)
	if not ContextPtr:IsHidden() then
		ContextPtr:SetHide(true)
	end
end
Events.GameplaySetActivePlayer.Add(OnActivePlayerChanged)
