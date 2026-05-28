-------------------------------------------------
-- Who's Winning Popup
-- LLM integration: instant-dismiss informational popup
-------------------------------------------------

function OnPopup(popupInfo)
	if popupInfo.Type ~= ButtonPopupTypes.BUTTONPOPUP_WHOS_WINNING then
		return;
	end
	Events.SerialEventGameMessagePopupProcessed.CallImmediate(ButtonPopupTypes.BUTTONPOPUP_WHOS_WINNING, 0);
end
Events.SerialEventGameMessagePopup.Add(OnPopup);
