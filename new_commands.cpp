// New pipe commands to add to CvGame::HandlePipeCommand()
// Insert these handlers after the "get_turn_blockers" handler (around line 3277)

	else if (msgType == "get_visible_tiles")
	{
		// Get all tiles revealed to active player (for ASCII map rendering)
		std::string requestId = msg.get("request_id").asString();
		PlayerTypes activePlayer = getActivePlayer();
		TeamTypes activeTeam = GET_PLAYER(activePlayer).getTeam();

		std::ostringstream os;
		os << "{\"type\":\"visible_tiles_result\"";
		if (!requestId.empty())
		{
			os << ",\"request_id\":\"" << PipeJson::Escape(requestId) << "\"";
		}
		os << ",\"turn\":" << getGameTurn();
		os << ",\"map_width\":" << GC.getMap().getGridWidth();
		os << ",\"map_height\":" << GC.getMap().getGridHeight();
		os << ",\"tiles\":[";

		bool first = true;
		int numPlots = GC.getMap().numPlots();
		for (int i = 0; i < numPlots; i++)
		{
			CvPlot* pPlot = GC.getMap().plotByIndexUnchecked(i);
			if (pPlot == NULL) continue;

			// Only include revealed tiles
			if (!pPlot->isRevealed(activeTeam)) continue;

			if (!first) os << ",";
			first = false;

			os << "{";
			os << "\"x\":" << pPlot->getX();
			os << ",\"y\":" << pPlot->getY();
			os << ",\"terrain_type\":" << static_cast<int>(pPlot->getTerrainType());
			os << ",\"feature_type\":" << static_cast<int>(pPlot->getFeatureType());
			os << ",\"plot_type\":" << static_cast<int>(pPlot->getPlotType());
			os << ",\"is_water\":" << (pPlot->isWater() ? "true" : "false");
			os << ",\"is_hills\":" << (pPlot->isHills() ? "true" : "false");
			os << ",\"is_mountain\":" << (pPlot->isMountain() ? "true" : "false");
			os << ",\"is_visible\":" << (pPlot->isVisible(activeTeam) ? "true" : "false");

			// Resource (with tech visibility check)
			ResourceTypes eResource = pPlot->getResourceType(activeTeam);
			if (eResource != NO_RESOURCE)
			{
				os << ",\"resource_type\":" << static_cast<int>(eResource);
				os << ",\"resource_quantity\":" << pPlot->getNumResource();
			}

			// Improvement (revealed version for fog-of-war)
			ImprovementTypes eImprovement = pPlot->getRevealedImprovementType(activeTeam);
			if (eImprovement != NO_IMPROVEMENT)
			{
				os << ",\"improvement_type\":" << static_cast<int>(eImprovement);
				os << ",\"improvement_pillaged\":" << (pPlot->IsImprovementPillaged() ? "true" : "false");
			}

			// Route
			RouteTypes eRoute = pPlot->getRevealedRouteType(activeTeam);
			if (eRoute != NO_ROUTE)
			{
				os << ",\"route_type\":" << static_cast<int>(eRoute);
				os << ",\"route_pillaged\":" << (pPlot->IsRoutePillaged() ? "true" : "false");
			}

			// Owner (revealed version)
			PlayerTypes eOwner = pPlot->getRevealedOwner(activeTeam);
			if (eOwner != NO_PLAYER)
			{
				os << ",\"owner_id\":" << static_cast<int>(eOwner);
			}

			os << "}";
		}

		os << "]}";
		m_kGameStatePipe.SendMessage(os.str());
		return;
	}
	else if (msgType == "get_unit_build_options")
	{
		// Get available build options for a worker unit in its vicinity
		std::string requestId = msg.get("request_id").asString();
		int unitId = msg.get("unit_id").asInt(-1);
		int radius = msg.get("radius").asInt(5);
		bool includeAllTiles = msg.get("include_all_tiles").asBool(false);

		std::ostringstream os;
		os << "{\"type\":\"unit_build_options_result\"";
		if (!requestId.empty())
		{
			os << ",\"request_id\":\"" << PipeJson::Escape(requestId) << "\"";
		}

		if (unitId < 0)
		{
			os << ",\"success\":false,\"error\":{\"code\":\"INVALID_UNIT_ID\",\"message\":\"Invalid unit_id\"}";
			os << "}";
			m_kGameStatePipe.SendMessage(os.str());
			return;
		}

		// Find unit
		CvUnit* pUnit = NULL;
		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
		{
			CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
			pUnit = kPlayer.getUnit(unitId);
			if (pUnit != NULL) break;
		}

		if (pUnit == NULL)
		{
			os << ",\"success\":false,\"error\":{\"code\":\"UNIT_NOT_FOUND\",\"message\":\"Unit not found\"}";
			os << "}";
			m_kGameStatePipe.SendMessage(os.str());
			return;
		}

		CvPlot* pUnitPlot = pUnit->plot();
		int centerX = pUnitPlot->getX();
		int centerY = pUnitPlot->getY();

		os << ",\"success\":true";
		os << ",\"unit_id\":" << unitId;
		os << ",\"unit_position\":{\"x\":" << centerX << ",\"y\":" << centerY << "}";
		os << ",\"tiles\":[";

		bool firstTile = true;
		for (int dx = -radius; dx <= radius; dx++)
		{
			for (int dy = -radius; dy <= radius; dy++)
			{
				CvPlot* pPlot = plotXYWithRangeCheck(centerX, centerY, dx, dy, radius);
				if (pPlot == NULL) continue;

				// Check all build types for this plot
				std::vector<BuildTypes> validBuilds;
				for (int i = 0; i < GC.getNumBuildInfos(); i++)
				{
					BuildTypes eBuild = (BuildTypes)i;
					if (pUnit->canBuild(pPlot, eBuild))
					{
						validBuilds.push_back(eBuild);
					}
				}

				if (validBuilds.empty() && !includeAllTiles) continue;

				if (!firstTile) os << ",";
				firstTile = false;

				os << "{";
				os << "\"x\":" << pPlot->getX();
				os << ",\"y\":" << pPlot->getY();
				os << ",\"terrain_type\":" << static_cast<int>(pPlot->getTerrainType());
				os << ",\"feature_type\":" << static_cast<int>(pPlot->getFeatureType());

				ResourceTypes eResource = pPlot->getResourceType(GET_PLAYER(pUnit->getOwner()).getTeam());
				if (eResource != NO_RESOURCE)
				{
					os << ",\"resource_type\":" << static_cast<int>(eResource);
					CvResourceInfo* pResourceInfo = GC.getResourceInfo(eResource);
					if (pResourceInfo)
					{
						os << ",\"resource_name\":\"" << PipeJson::Escape(pResourceInfo->GetDescription()) << "\"";
					}
				}

				ImprovementTypes eImprovement = pPlot->getImprovementType();
				os << ",\"current_improvement\":" << static_cast<int>(eImprovement);

				os << ",\"available_builds\":[";
				for (size_t i = 0; i < validBuilds.size(); i++)
				{
					if (i > 0) os << ",";
					BuildTypes eBuild = validBuilds[i];
					CvBuildInfo* pBuildInfo = GC.getBuildInfo(eBuild);
					if (pBuildInfo == NULL) continue;

					os << "{";
					os << "\"build_type\":" << static_cast<int>(eBuild);
					os << ",\"build_name\":\"" << PipeJson::Escape(pBuildInfo->GetDescription()) << "\"";
					os << ",\"turns_required\":" << pPlot->getBuildTime(eBuild, pUnit->getOwner());

					ImprovementTypes eNewImprovement = (ImprovementTypes)pBuildInfo->getImprovement();
					if (eNewImprovement != NO_IMPROVEMENT)
					{
						CvImprovementEntry* pImprovementEntry = GC.getImprovementInfo(eNewImprovement);
						if (pImprovementEntry)
						{
							os << ",\"improvement_name\":\"" << PipeJson::Escape(pImprovementEntry->GetDescription()) << "\"";
						}
					}

					os << "}";
				}
				os << "]";

				os << "}";
			}
		}

		os << "]}";
		m_kGameStatePipe.SendMessage(os.str());
		return;
	}
	else if (msgType == "get_reachable_tiles")
	{
		// Get tiles that a unit can move to this turn
		std::string requestId = msg.get("request_id").asString();
		int unitId = msg.get("unit_id").asInt(-1);
		bool includeAttacks = msg.get("include_attacks").asBool(true);

		std::ostringstream os;
		os << "{\"type\":\"reachable_tiles_result\"";
		if (!requestId.empty())
		{
			os << ",\"request_id\":\"" << PipeJson::Escape(requestId) << "\"";
		}

		if (unitId < 0)
		{
			os << ",\"success\":false,\"error\":{\"code\":\"INVALID_UNIT_ID\",\"message\":\"Invalid unit_id\"}";
			os << "}";
			m_kGameStatePipe.SendMessage(os.str());
			return;
		}

		// Find unit
		CvUnit* pUnit = NULL;
		for (int iPlayer = 0; iPlayer < MAX_PLAYERS; ++iPlayer)
		{
			CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iPlayer);
			pUnit = kPlayer.getUnit(unitId);
			if (pUnit != NULL) break;
		}

		if (pUnit == NULL)
		{
			os << ",\"success\":false,\"error\":{\"code\":\"UNIT_NOT_FOUND\",\"message\":\"Unit not found\"}";
			os << "}";
			m_kGameStatePipe.SendMessage(os.str());
			return;
		}

		os << ",\"success\":true";
		os << ",\"unit_id\":" << unitId;
		os << ",\"unit_position\":{\"x\":" << pUnit->getX() << ",\"y\":" << pUnit->getY() << "}";
		os << ",\"moves_remaining\":" << pUnit->getMoves();
		os << ",\"tiles\":[";

		// Get reachable plots using the unit's pathfinder
		std::map<int, int> reachablePlots; // plotIndex -> movementCost
		pUnit->GetReachablePlots(reachablePlots);

		bool first = true;
		for (std::map<int, int>::iterator it = reachablePlots.begin(); it != reachablePlots.end(); ++it)
		{
			CvPlot* pPlot = GC.getMap().plotByIndexUnchecked(it->first);
			if (pPlot == NULL) continue;

			bool canEnter = pUnit->canMoveInto(*pPlot, CvUnit::MOVEFLAG_DESTINATION);
			bool canAttack = includeAttacks && pUnit->canMoveInto(*pPlot, CvUnit::MOVEFLAG_ATTACK);

			if (!first) os << ",";
			first = false;

			os << "{";
			os << "\"x\":" << pPlot->getX();
			os << ",\"y\":" << pPlot->getY();
			os << ",\"movement_cost\":" << it->second;
			os << ",\"can_enter\":" << (canEnter ? "true" : "false");

			if (includeAttacks)
			{
				os << ",\"can_attack\":" << (canAttack ? "true" : "false");

				if (canAttack)
				{
					// Check for enemy unit
					CvUnit* pDefender = pPlot->getBestDefender(NO_PLAYER, pUnit->getOwner());
					if (pDefender)
					{
						os << ",\"is_enemy_unit\":true";
						os << ",\"enemy_unit_type\":\"" << PipeJson::Escape(pDefender->getName()) << "\"";
					}
					else
					{
						os << ",\"is_enemy_unit\":false";
					}
				}
			}

			os << "}";
		}

		os << "]}";
		m_kGameStatePipe.SendMessage(os.str());
		return;
	}
