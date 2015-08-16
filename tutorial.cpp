#include "stdafx.h"

#include "tutorial.h"
#include "game.h"
#include "gamestate.h"
#include "gui.h"
#include "utils.h"

Tutorial *tutorial = NULL;

void setupTutorial(const string &id) {
	if( id == "first" )
		tutorial = new Tutorial1(id);
	else if( id == "second" )
		tutorial = new Tutorial2(id);
}

void GUIHandler::resetGUI(PlayingGameState *playing_gamestate) {
	playing_gamestate->getGamePanel()->setEnabled(true);
	playing_gamestate->getScreenPage()->setEnabled(true);
}

void GUIHandlerBlockAll::setGUI(PlayingGameState *playing_gamestate) const {
	playing_gamestate->getGamePanel()->setEnabled(false);
	playing_gamestate->getScreenPage()->setEnabled(false);
	for(vector<string>::const_iterator iter = exceptions.begin(); iter != exceptions.end(); ++iter) {
		const string exception = *iter;
		PanelPage *panel = playing_gamestate->getGamePanel()->findById(exception);
		if( panel != NULL ) {
			panel->setEnabled(true);
		}
		else {
			panel = playing_gamestate->getScreenPage()->findById(exception);
			if( panel != NULL ) {
				panel->setEnabled(true);
			}
			T_ASSERT(panel != NULL);
		}
	}
	// the following are always exceptions
	PanelPage *pause_button = playing_gamestate->getScreenPage()->findById("pause_button");
	T_ASSERT( pause_button != NULL );
	if( pause_button != NULL ) {
		pause_button->setEnabled(true);
	}
	PanelPage *quit_button = playing_gamestate->getScreenPage()->findById("quit_button");
	T_ASSERT( quit_button != NULL );
	if( quit_button != NULL ) {
		quit_button->setEnabled(true);
	}
	PanelPage *speed_button = playing_gamestate->getScreenPage()->findById("speed_button");
	T_ASSERT( speed_button != NULL );
	if( speed_button != NULL ) {
		speed_button->setEnabled(true);
	}
}

void TutorialCard::setGUI(PlayingGameState *playing_gamestate) const {
	playing_gamestate->getGamePanel()->setEnabled(true);
	playing_gamestate->getScreenPage()->setEnabled(true);
	if( gui_handler != NULL ) {
		gui_handler->setGUI(playing_gamestate);
	}
}

bool TutorialCardWaitForPanelPage::canProceed(PlayingGameState *playing_gamestate) const {
	int c_page = playing_gamestate->getGamePanel()->getPage();
	return c_page == wait_page;
}

bool TutorialCardWaitForDesign::canProceed(PlayingGameState *playing_gamestate) const {
	const Sector *current_sector = playing_gamestate->getCurrentSector();
	if( wait_type == WAITTYPE_CURRENT_DESIGN ) {
		const Design *current_design = current_sector->getCurrentDesign();
		if( current_design == NULL )
			return false;
		const Invention *invention = current_design->getInvention();
		if( require_type && invention->getType() != invention_type )
			return false;
		return true;
	}
	else if( wait_type == WAITTYPE_HAS_DESIGNED ) {
		for(int i=0;i<Invention::N_TYPES;i++) {
			if( require_type && i != invention_type )
				continue;
			for(int j=0;j<n_epochs_c;j++) {
				if( current_sector->inventionKnown(static_cast<Invention::Type>(i), j) ) {
					return true;
				}
			}
		}
		return false;
	}
	ASSERT(false);
	return true;
}

bool TutorialCardWaitForDeployedArmy::canProceed(PlayingGameState *playing_gamestate) const {
	if( !inverse ) {
		int str = require_bombard ? deploy_sector->getArmy(playing_gamestate->getClientPlayer())->getBombardStrength() : deploy_sector->getArmy(playing_gamestate->getClientPlayer())->getTotal();
		if( require_unoccupied && deploy_sector->getPlayer() != PLAYER_NONE ) {
			return false;
		}
		if( require_empty && str == 0 )
			return true;
		else if( !require_empty && str > 0 )
			return true;
	}
	else {
		for(int y=0;y<map_height_c;y++) {
			for(int x=0;x<map_width_c;x++) {
				if( map->isSectorAt(x, y) ) {
					const Sector *sector = map->getSector(x, y);
					if( sector == deploy_sector )
						continue;
					if( require_unoccupied && sector->getPlayer() != PLAYER_NONE ) {
						continue;
					}
					int str = require_bombard ? sector->getArmy(playing_gamestate->getClientPlayer())->getBombardStrength() : sector->getArmy(playing_gamestate->getClientPlayer())->getTotal();
					if( require_empty && str > 0 )
						return false;
					else if( !require_empty && str > 0 )
						return true;
				}
			}
		}
		if( require_empty )
			return true;
	}
	return false;
}

bool TutorialCardWaitForNewTower::canProceed(PlayingGameState *playing_gamestate) const {
	if( !inverse ) {
		bool has_tower = tower_sector->getPlayer() == playing_gamestate->getClientPlayer();
		if( has_tower )
			return true;
	}
	else {
		for(int y=0;y<map_height_c;y++) {
			for(int x=0;x<map_width_c;x++) {
				if( map->isSectorAt(x, y) ) {
					const Sector *sector = map->getSector(x, y);
					if( sector == tower_sector )
						continue;
					bool has_tower = sector->getPlayer() == playing_gamestate->getClientPlayer();
					if( has_tower )
						return true;
				}
			}
		}
	}
	return false;
}

Tutorial::~Tutorial() {
	for(vector<TutorialCard *>::const_iterator iter = cards.begin(); iter != cards.end(); ++iter) {
		TutorialCard *card = *iter;
		delete card;
	}
}

bool Tutorial::jumpTo(const string &id) {
	int index = 0;
	for(vector<TutorialCard *>::const_iterator iter = cards.begin(); iter != cards.end(); ++iter, index++) {
		TutorialCard *card = *iter;
		if( card->getId() == id ) {
			card_index = index;
			return true;
		}
	}
	return false;
}

Tutorial1::Tutorial1(const string &id) : Tutorial(id) {
	start_epoch = 0;
	island = 0;
	start_map_x = 1;
	start_map_y = 2;
	n_men = 20;
	ai_allow_growth = false;
	ai_allow_design = false;
}

void Tutorial1::initCards() {
	Sector *enemy_sector = map->getSector(2, 2);
	ASSERT(enemy_sector != NULL);

	TutorialCard *card = NULL;

	card = new TutorialCard("0", "Welcome to Gigalomania!\nThis tutorial will introduce you to the game,\nand show you how to win your first island.\nClick 'next' to continue when you're ready.");
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("1", "Here you can see a map of the current island.\nThe island is split up into sectors.\nThe coloured squares represent sectors controlled by a player.");
	card->setArrow(40, 56);
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("2", "The age of this sector is 10,000 bc.\nThis represents the state of technology. \nIt's possible for different sectors to be in different ages.");
	card->setArrow(80, 110);
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("3", "Next we'll go over the control panel, which\nallows you to control your sector.");
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("4", "This shows the number of people in your sector that are available.\nThe population will grow gradually with time.");
	card->setArrow(16, 136);
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("5", "Now let's design a weapon!\nClick on the lightbulb icon to start researching.", (int)GamePanel::STATE_DESIGN);
	card->setArrow(36, 156);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_design");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCard("6", "This page allows you to design new inventions:\nThe left hand column shows shield, which are used to repair buildings.\nThe middle shows defences for your sector.\nThe right shows weapons to attack the enemy!");
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCardWaitForDesign("7", "For this tutorial, we're going to design a weapon.\nClick on one of the weapons.\nI recommend the Rock weapon, but any will do.", TutorialCardWaitForDesign::WAITTYPE_CURRENT_DESIGN, true, Invention::WEAPON);
	card->setArrow(90, 168);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	if( oneMouseButtonMode() ) {
		card = new TutorialCard("8", "Now put some of your people to work designing the weapon.\nClick here, then use the arrows to increase or decrease\nthe number of designers.");
	}
	else {
		card = new TutorialCard("8", "Now put some of your people to work designing the weapon.\nUse the right mouse button to increase the number of designers,\nleft mouse button to decrease.");
	}
	card->setArrow(50, 130);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		gui_handler->addException("button_designers");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCard("9", "While your designers are working, the clock shows the remaining time\nuntil the invention is ready.");
	card->setArrow(86, 130);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		gui_handler->addException("button_designers");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	if( oneMouseButtonMode() ) {
		card = new TutorialCardWaitForDesign("10", "To hurry things up, you can make time go faster.\nClick this icon to cycle through different time rates.", TutorialCardWaitForDesign::WAITTYPE_HAS_DESIGNED, true, Invention::WEAPON);
	}
	else {
		card = new TutorialCardWaitForDesign("10", "To hurry things up, you can make time go faster.\nRight click on this icon to speed time up.", TutorialCardWaitForDesign::WAITTYPE_HAS_DESIGNED, true, Invention::WEAPON);
	}
	card->setArrow(100, 10);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		gui_handler->addException("button_designers");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("11", "Great! Now click here to go back to the main interface.", (int)GamePanel::STATE_SECTORCONTROL);
	card->setArrow(50, 110);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		gui_handler->addException("button_designers");
		gui_handler->addException("button_bigdesign");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForPanelPage("11", "Time to attack our enemy. This button allows you to\nassemble your army.", (int)GamePanel::STATE_ATTACK);
	card->setArrow(80, 125);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_attack");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCard("12", "This page shows you the available soldiers to deploy.\nThey can be unarmed or, preferably, armed with weapons\nyou have invented.");
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("13", "Not only are armed soldiers stronger, but they are required to destory\nan enemy's buildings. Unarmed soldiers can fight other soldiers,\nbut won't knock down the enemy tower.");
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("14", "Click to assemble soldiers with the weapon you've just invented.\nAssemble as many soldiers as we have people available!");
	card->setArrow(15, 170);
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler->addException("button_deploy_attackers_0");
		gui_handler->addException("button_deploy_attackers_1");
		gui_handler->addException("button_deploy_attackers_2");
		gui_handler->addException("button_deploy_attackers_3");
		card->setGUIHandler(gui_handler);
	}
	cards.push_back(card);

	card = new TutorialCardWaitForDeployedArmy("15", "Then click on the enemy's sector in the map - that's the\nright hand square - to send your army to attack.", enemy_sector, true);
	card->setArrow(48, 56);
	GUIHandler *gui_handler_15 = NULL;
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler_15 = gui_handler;
		gui_handler->addException("button_deploy_attackers_0");
		gui_handler->addException("button_deploy_attackers_1");
		gui_handler->addException("button_deploy_attackers_2");
		gui_handler->addException("button_deploy_attackers_3");
		gui_handler->addException("map_1_2"); // need to allow the user to return to the player sector if necessary (as user can switch to the other sector without sending men)
		gui_handler->addException("map_2_2");
		gui_handler->addException("button_attack"); // allow the user to get back to the attack screen (a GUI resets to main if user switches current sector)
		gui_handler->addException("button_bigattack"); // ...and for consistency, allow the user to go back to the main screen
		// and in case the user wants to design some more:
		gui_handler->addException("button_design");
		gui_handler->addException("button_weapons_0");
		gui_handler->addException("button_weapons_1");
		gui_handler->addException("button_weapons_2");
		gui_handler->addException("button_weapons_3");
		gui_handler->addException("button_designers");
		gui_handler->addException("button_bigdesign");
	}
	card->setGUIHandler(gui_handler_15);
	cards.push_back(card);

	card = new TutorialCard("16", "Now wait until the battle is won!\nRemember to speed up the time rate if you prefer.");
	card->setNextText("Done");
	cards.push_back(card);

	// for debugging
	//this->card_index = 13;
}

Tutorial2::Tutorial2(const string &id) : Tutorial(id) {
	start_epoch = 0;
	island = 2;
	start_map_x = 2;
	start_map_y = 2;
	n_men = 25;
	auto_end = true;
	ai_allow_growth = false;
	ai_allow_design = false;
}

void Tutorial2::initCards() {
	Sector *start_sector = map->getSector(start_map_x, start_map_y);
	ASSERT(start_sector != NULL);

	TutorialCard *card = NULL;

	card = new TutorialCardWaitForPanelPage("0", "In this tutorial we'll learn some combat maneuvers.\nSelect the attack menu option to deploy some soldiers.", (int)GamePanel::STATE_ATTACK);
	card->setArrow(80, 125);
	cards.push_back(card);

	card = new TutorialCard("1", "In the first tutorial you learnt how to design weapons.\nHere you'll be learning how to deploy and move your soldiers,\nso unarmed men will do.");
	card->setGUIHandler(new GUIHandlerBlockAll());
	cards.push_back(card);

	card = new TutorialCard("2", "Click to assemble some unarmed men.");
	card->setArrow(15, 145);
	GUIHandler *gui_handler_2 = NULL;
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler_2 = gui_handler;
		gui_handler->addException("button_deploy_unarmedmen");
		gui_handler->addException("button_deploy_attackers_0");
		gui_handler->addException("button_deploy_attackers_1");
		gui_handler->addException("button_deploy_attackers_2");
		gui_handler->addException("button_deploy_attackers_3");
		gui_handler->addException("button_return_attackers");
	}
	card->setGUIHandler(gui_handler_2);
	cards.push_back(card);

	card = new TutorialCard("3", "Note that if you make a mistake assembling your army,\nyou can cancel by clicking here.");
	card->setArrow(87, 195);
	card->setGUIHandler(gui_handler_2);
	cards.push_back(card);

	card = new TutorialCardWaitForDeployedArmy("4", "Assemble some unarmed men, and send them to a\nsector by clicking on a new map square of your choice.", start_sector, false);
	static_cast<TutorialCardWaitForDeployedArmy *>(card)->setInverse(true);
	card->setArrow(60, 70);
	cards.push_back(card);

	if( oneMouseButtonMode() ) {
		card = new TutorialCard("5", "Now see if you can return them to the home sector.\nYou can select an army by clicking,\neither on the current map square,\nor clicking on the main area to the right.");
	}
	else {
		card = new TutorialCard("5", "Now see if you can return them to the home sector.\nYou can select an army by right clicking,\neither on the current map square,\nor right clicking on the main area to the right.");
	}
	cards.push_back(card);

	card = new TutorialCardWaitForDeployedArmy("6", "When the army is selected, the shield icon will show.\nMove the army back home by clicking on the square of your sector", start_sector, false);
	card->setArrow(47, 54);
	cards.push_back(card);

	// return back home

	if( oneMouseButtonMode() ) {
		card = new TutorialCardWaitForDeployedArmy("7", "Now let's return our men back to the tower.\nClick to select the army as before, then\nclick on your tower.", NULL, false);
	}
	else {
		card = new TutorialCardWaitForDeployedArmy("7", "Now let's return our men back to the tower.\nRight click to select the army as before, then\nleft click on your tower.", NULL, false);
	}
	static_cast<TutorialCardWaitForDeployedArmy *>(card)->setInverse(true);
	static_cast<TutorialCardWaitForDeployedArmy *>(card)->setRequireEmpty(true);
	card->setArrow(260, 80);
	cards.push_back(card);

	// build new tower

	card = new TutorialCard("8", "So far you've only had a single tower,\nbut you can build new towers.");
	cards.push_back(card);

	card = new TutorialCard("9", "Each tower can act independently, researching and\nconstructing different weapons.\nEach tower needs to invent its own weapons.");
	cards.push_back(card);

	card = new TutorialCardWaitForDeployedArmy("10", "Assemble some unarmed men again, and this time\nsend them to a square that isn't occupied by the enemy", start_sector, false);
	static_cast<TutorialCardWaitForDeployedArmy *>(card)->setInverse(true);
	static_cast<TutorialCardWaitForDeployedArmy *>(card)->setRequireUnoccupied(true);
	cards.push_back(card);

	card = new TutorialCardWaitForNewTower("11", "Now sit back and wait until your new tower is constructed.\nRemember to speed up the rate of time if you want.", start_sector);
	static_cast<TutorialCardWaitForNewTower *>(card)->setInverse(true);
	cards.push_back(card);

	card = new TutorialCard("12", "You have completed this tutorial!");
	cards.push_back(card);

	// for debugging
	//this->card_index = 11;
}
