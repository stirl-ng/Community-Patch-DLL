-------------------------------------------------
-- Great Work Popup
-- LLM integration: instant-dismiss informational popup
-------------------------------------------------

function OnPopup(popupInfo)
	if popupInfo.Type ~= ButtonPopupTypes.BUTTONPOPUP_GREAT_WORK_COMPLETED_ACTIVE_PLAYER then
		return;
	end
	Events.SerialEventGameMessagePopupProcessed.CallImmediate(ButtonPopupTypes.BUTTONPOPUP_GREAT_WORK_COMPLETED_ACTIVE_PLAYER, 0);
end
Events.SerialEventGameMessagePopup.Add(OnPopup);
