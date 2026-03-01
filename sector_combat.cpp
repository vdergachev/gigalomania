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

void Sector::doCombat(int client_player) {
	//LOG("Sector::doCombat()\n");
	int looptime = game_g->getLoopTime();

	/*int army_strengths[n_players_c];
	int n_armies = 0;
	for(i=0;i<n_players_c;i++) {
	army_strengths[i] = this->getArmy(i)->getStrength();
	if( army_strengths[i] > 0 )
	n_armies++;
	}
	if( n_armies == 2 ) {
	Army *friendly = this->getArmy(human_player);
	Army *enemy = this->getArmy(enemy_player);
	}*/
	bool died[n_players_c];
	//int random = rand () % RAND_MAX;
	for(int i=0;i<n_players_c;i++) {
		died[i] = false;
		Army *army = this->getArmy(i);
		int this_strength = army->getStrength();
		int this_total = army->getTotal();
		//LOG(">>> %d , %d\n", this_strength, this_total);
		if( this->player == i ) {
			this_total += this->getNDefenders();
			this_strength += this->getDefenderStrength();
		}
		//LOG("    %d , %d\n", this_strength, this_total);
		if( this_total > 0 ) {
			int enemy_strength = 0;
			for(int j=0;j<n_players_c;j++) {
				if( i != j && !Player::isAlliance(i,j) ) {
					enemy_strength += this->getArmy(j)->getStrength();
					if( this->player == j ) {
						enemy_strength += this->getDefenderStrength();
					}
				}
			}
			if( enemy_strength > 0 ) {
				//int death_rate = ( combat_rate_c * gameticks_per_hour_c * this_strength ) / enemy_strength;
				//int death_rate = ( combat_rate_c * gameticks_per_hour_c ) / enemy_strength;
				//int death_rate = ( combat_rate_c * gameticks_per_hour_c * this_strength ) / ( enemy_total * enemy_strength );
				// need to use floats, to avoid possible overflow!!!
				// lower death rate is faster - which is why we divide by the enemy strength
				// but we multiply by the average strength of a soldier, because we need to convert a loss in terms of strength to a loss in terms of soldier numbers
				// consider if we have: A=1*100 vs B=100*1 - we want this to be an even battle. But that means B should lose soldiers on average at 100 times the rate of A, in order for it to be even
				// see docs/combat_logic.txt for more examples
				float death_rate = ((float)this_strength) / ((float)this_total);
				death_rate = death_rate * ((float)(combat_rate_c * gameticks_per_hour_c)) / ((float)enemy_strength);
				int prob = poisson((int)death_rate, looptime);
				int random = rand() % RAND_MAX;
				if( random <= prob ) {
					// soldier has died
					died[i] = true;
				}
			}
		}
	}
	for(int i=0;i<n_players_c;i++) {
		if( died[i] ) {
			Army *army = this->getArmy(i);
			/*
			// original behaviour, randomly choose between a defending army or defenders in turrets
			int this_total = army->getTotal();
			if( this->player == i )
				this_total += this->getNDefenders();

			int die = rand() % this_total;
			if( die < army->getTotal() ) {
				army->kill(die);
			}
			else {
				// kill a defender
				this->killDefender(die - army->getTotal());
			}
			*/
			// new behaviour, only attack defenders if no army present in sector
			// see https://sourceforge.net/p/gigalomania/discussion/general/thread/dbd2f751/
			// this helps make defenders more powerful
			int this_total = army->getTotal();
			if( this->player == i && this_total == 0 ) {
				// no army, so one of the defenders must die
				this_total = this->getNDefenders();
				T_ASSERT( this_total > 0 );
				if( this_total > 0 ) { // just in case
					int die = rand() % this_total;
					// kill a defender
					this->killDefender(die);
 				}
			}
			else {
				T_ASSERT( this_total > 0 );
				if( this_total > 0 ) { // just in case
					int die = rand() % this_total;
					army->kill(die);
				}
			}
		}
	}

	// damage to buildings
	if( this->player != -1 ) {
		int bombard = 0;
		// buildings not harmed if there's an army or defenders
		// see https://sourceforge.net/p/gigalomania/discussion/general/thread/dbd2f751/
		// this makes defenders more useful, and means buildings aren't destroyed so quickly when an army attacks
		if( this->getArmy(this->player)->any(true) ) {
			bombard = 0;
		}
		else if( this->getNDefenders() > 0 ) {
			bombard = 0;
		}
		else {
			for(int i=0;i<n_players_c;i++) {
				if( this->player != i && !Player::isAlliance(this->player, i) ) {
					bombard += this->getArmy(i)->getBombardStrength();
				}
			}
		}
		if( bombard > 0 ) {
			int bombard_rate = ( bombard_rate_c * gameticks_per_hour_c ) / bombard;
			bombard_rate = (int)(bombard_rate * this->getDefenceStrength());
			int prob = poisson(bombard_rate, looptime);
			int random = rand() % RAND_MAX;
			if( random <= prob ) {
				// caused some damage
				int n_buildings = 0;
				for(int i=0;i<N_BUILDINGS;i++) {
					if( this->buildings[i] != NULL )
						n_buildings++;
				}
				ASSERT( n_buildings > 0 );
				int b = rand() % n_buildings;
				for(int i=0;i<N_BUILDINGS;i++) {
					Building *building = this->buildings[i];
					if( building != NULL ) {
						if( b == 0 ) {
							building->addHealth(-1);
#ifdef _DEBUG
                            // disable in Release mode as possible performance issue on mobile devices (Symbian)
                            LOG("Sector [%d: %d, %d] caused some damage on building %d, type %d, %d remaining\n", player, xpos, ypos, building, building->getType(), building->getHealth());
#endif
							if( building->getHealth() <= 0 ) {
								// destroy building
								destroyBuilding((Type)i, client_player);
							}
							else if( this->player == client_player && building->getHealth() == 10 && building->getType() == BUILDING_TOWER ) {
								playSample(game_g->s_tower_critical);
								//((PlayingGameState *)gamestate)->setFlashingSquare(this->xpos, this->ypos);
								gamestate->setFlashingSquare(this->xpos, this->ypos);
							}
							break;
						}
						b--;
					}
				}
			}
		}
	}
}

