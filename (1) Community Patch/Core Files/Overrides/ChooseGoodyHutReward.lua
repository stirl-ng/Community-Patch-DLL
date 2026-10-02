-------------------------------------------------
-- Choose Goody Hut Reward Popup
-- LLM integration: instant-dismiss. The DLL records the pending choice and sends the
-- options to the pipe (popup_choice_needed / choose_goody_hut_reward); the player picks
-- with the choose_goody_hut_reward command, and end_turn is blocked until then.
-------------------------------------------------

function OnPopup(popupInfo)
	if popupInfo.Type ~= ButtonPopupTypes.BUTTONPOPUP_CHOOSE_GOODY_HUT_REWARD then
		return;
	end
	Events.SerialEventGameMessagePopupProcessed.CallImmediate(ButtonPopupTypes.BUTTONPOPUP_CHOOSE_GOODY_HUT_REWARD, 0);
end
Events.SerialEventGameMessagePopup.Add(OnPopup);
