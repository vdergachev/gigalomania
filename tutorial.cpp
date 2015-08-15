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
	if( deploy_sector->getArmy(playing_gamestate->getClientPlayer())->getBombardStrength() > 0 )
	//if( deploy_sector->getArmy(playing_gamestate->getClientPlayer())->getTotal() > 0 ) // test
		return true;
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
	start_map_x = 1;
	start_map_y = 2;
	n_men = 20;
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

	card = new TutorialCardWaitForDeployedArmy("15", "Then click on the enemy's sector in the map - that's the\nright hand square - to send your army to attack.", enemy_sector);
	card->setArrow(48, 56);
	GUIHandler *gui_handler_15 = NULL;
	{
		GUIHandlerBlockAll *gui_handler = new GUIHandlerBlockAll();
		gui_handler_15 = gui_handler;
		gui_handler->addException("button_deploy_attackers_0");
		gui_handler->addException("button_deploy_attackers_1");
		gui_handler->addException("button_deploy_attackers_2");
		gui_handler->addException("button_deploy_attackers_3");
		gui_handler->addException("map_1_2"); // need to allow the user to return to the player sector if necessary
		gui_handler->addException("map_2_2");
		gui_handler->addException("button_attack"); // allow the user to get back to send more attackers
		gui_handler->addException("button_bigattack");
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
	card->setGUIHandler(gui_handler_15);
	cards.push_back(card);

	// for debugging
	//this->card_index = 13;
}
