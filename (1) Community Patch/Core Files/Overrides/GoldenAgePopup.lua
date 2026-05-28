-------------------------------------------------
-- Golden Age Popup
-- LLM integration: instant-dismiss informational popup
-------------------------------------------------

function OnPopup(popupInfo)
	if popupInfo.Type ~= ButtonPopupTypes.BUTTONPOPUP_GOLDEN_AGE_REWARD then
		return;
	end
	Events.SerialEventGameMessagePopupProcessed.CallImmediate(ButtonPopupTypes.BUTTONPOPUP_GOLDEN_AGE_REWARD, 0);
end
Events.SerialEventGameMessagePopup.Add(OnPopup);
