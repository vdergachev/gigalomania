#include "stdafx.h"

#include <cassert>
#include <algorithm>
using std::min;
using std::max;
#include <stdexcept>
#include <sstream>

#include "sector.h"
#include "game.h"
#include "utils.h"
#include "gamestate.h"
#include "panel.h"
#include "gui.h"
#include "player.h"
#include "screen.h"
#include "image.h"
#include "sound.h"
#include "tutorial.h"

void Sector::doPlayer(int client_player) {
	//LOG("Sector::doPlayer()\n");
	// stuff for sectors owned by a player

	// n.b., only update the particle systems that are specific to sectors owned by a player
	if( this->smokeParticleSystem != NULL ) {
		this->smokeParticleSystem->update();
	}

	if( game_g->getGameMode() == GAMEMODE_MULTIPLAYER_CLIENT ) {
		// rest of function is for game logic done by server
		return;
	}

	int time = game_g->getGameTime();
	int looptime = game_g->getLoopTime();

	if( this->current_design != NULL ) {
		if( this->researched_lasttime == -1 )
			this->researched_lasttime = time;
		while( time - this->researched_lasttime > gameticks_per_hour_c ) {
			this->researched += this->getDesigners();
			this->researched_lasttime += gameticks_per_hour_c;
		}
		int cost = this->getInventionCost();
		if( this->researched > cost ) {
			//LOG("Sector [%d: %d, %d] has made a %s\n", player, xpos, ypos, this->current_design->getInvention()->getName());
			this->invent(client_player);
		}
	}

	if( this->current_manufacture != NULL ) {
		if( this->manufactured_lasttime == -1 )
			this->manufactured_lasttime = time;
		if( this->manufactured == 0 && this->getWorkers() > 0 ) {
			if( !this->canBuildDesign( this->current_manufacture ) ) {
				// not enough elements
				// therefore production run completed
				if( this->player == client_player ) {
					playSample(game_g->s_fcompleted);
					gamestate->setFlashingSquare(this->xpos, this->ypos);
				}
				this->setCurrentManufacture(NULL);
				if( this == gamestate->getCurrentSector() ) {
					//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
					gamestate->getGamePanel()->refresh();
				}
			}
			else {
				this->consumeStocks( this->current_manufacture );
			}
		}
	}
	// a new if statement, as production run may have ended due to lack of elements
	if( this->current_manufacture != NULL ) {
		while( time - this->manufactured_lasttime > gameticks_per_hour_c ) {
			this->manufactured += this->getWorkers();
			this->manufactured_lasttime += gameticks_per_hour_c;
		}
		if( this->manufactured == 0 && this->getWorkers() > 0 ) {
			this->manufactured++; // a bit hacky; just to avoid consuming stocks again
		}
		int cost = this->getManufactureCost();
		if( this->manufactured > cost ) {
			this->buildDesign();
			this->manufactured = 0;
			if( this->n_famount != infinity_c )
				this->setFAmount( this->n_famount - 1 );
			if( this->n_famount == 0 ) {
				// production run completed
				//LOG("production run completed\n");
				if( this->player == client_player ) {
					playSample(game_g->s_fcompleted);
					gamestate->setFlashingSquare(this->xpos, this->ypos);
				}
				this->setCurrentManufacture(NULL);
			}
			if( this == gamestate->getCurrentSector() ) {
				//((PlayingGameState *)gamestate)->getGamePanel()->refresh();
				gamestate->getGamePanel()->refresh();
			}
		}
	}

	bool new_stocks = false;
	for(int i=0;i<N_ID;i++) {
		if( this->elements[i] == 0 ) {
			continue; // no more of this element
		}
		Element *element = game_g->elements[i];
		if( element->getType() != Element::GATHERABLE || this->elementstocks[i] < max_gatherables_stored_c * element_multiplier_c )
		{
			int n_gatherers = 0;
			if( element->getType() == Element::GATHERABLE ) {
				n_gatherers = n_gatherable_rate_c;
			}
			else
				n_gatherers = this->getMiners((Id)i);
			this->partial_elementstocks[i] += element_multiplier_c * n_gatherers * looptime;
			while( this->partial_elementstocks[i] > mine_rate_c * gameticks_per_hour_c ) {
				new_stocks = true;
				this->partial_elementstocks[i] -= mine_rate_c * gameticks_per_hour_c;
				if( this->mineElement(client_player, (Id)i) ) {
					break;
				}
			}
		}
	}
	if( new_stocks && this == gamestate->getCurrentSector() ) {
		/*((PlayingGameState *)gamestate)->getGamePanel()->refreshCanDesign();
		((PlayingGameState *)gamestate)->getGamePanel()->refreshDesignInventions();
		//((PlayingGameState *)gamestate)->getGamePanel()->refreshDeployInventions();
		((PlayingGameState *)gamestate)->getGamePanel()->refreshManufactureInventions();*/

		gamestate->getGamePanel()->refreshCanDesign();
		gamestate->getGamePanel()->refreshDesignInventions();
		gamestate->getGamePanel()->refreshManufactureInventions();
	}

	if( this->built_lasttime == -1 )
		this->built_lasttime = time;
	while( time - this->built_lasttime > gameticks_per_hour_c ) {
		for(int i=0;i<N_BUILDINGS;i++) {
			this->built[i] += this->getBuilders((Type)i);
		}
		this->built_lasttime += gameticks_per_hour_c;
	}
	for(int i=0;i<N_BUILDINGS;i++) {
		int cost = getBuildingCost((Type)i, this->player);
		if( this->built[i] > cost ) {
			this->buildBuilding((Type)i);
		}
	}

	if( this->growth_lasttime == -1 )
		this->growth_lasttime = time;
	else if( game_g->getTutorial() != NULL && !game_g->getTutorial()->aiAllowGrowth() && !game_g->players[this->player]->isHuman() ) {
		// don't allow growth
	}
	else if( this->getSparePopulation() > 0 ) {
		int spare_pop = this->getSparePopulation();
		if( spare_pop > max_grow_population_c/2 ) {
			spare_pop = max_grow_population_c - spare_pop;
			spare_pop = max(spare_pop, 0);
		}
		if( spare_pop > 0 ) {
			int delay = ( growth_rate_c * gameticks_per_hour_c ) / spare_pop;
			if( time - this->growth_lasttime > delay ) {
				int old_pop = this->population;
				if( this->getSparePopulation() < max_grow_population_c )
					this->population++;
				this->growth_lasttime = time;
				int births = this->population - old_pop;
				if( births > 0 ) {
					game_g->players[this->player]->registerBirths(births);
				}
			}
		}
	}
}

