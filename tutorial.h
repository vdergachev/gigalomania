#pragma once

/** Classes to handle the Tutorials.
*/

using std::vector;
using std::string;

class PlayingGameState;

#include "sector.h"

void setupTutorial(const string &id);

class TutorialCard {
	string id;
	string text;
	string next_text;
	bool has_arrow;
	int arrow_x, arrow_y;
	bool auto_proceed;
public:
	TutorialCard(const string &id, const string &text) : id(id), text(text), next_text("Next"), has_arrow(false), arrow_x(-1), arrow_y(-1), auto_proceed(false) {
	}

	string getId() const {
		return id;
	}
	string getText() const {
		return text;
	}

	void setNextText(const string &next_text) {
		this->next_text = next_text;
	}
	string getNextText() const {
		return next_text;
	}

	void setArrow(int arrow_x, int arrow_y) {
		this->has_arrow = true;
		this->arrow_x = arrow_x;
		this->arrow_y = arrow_y;
	}
	bool hasArrow() const {
		return has_arrow;
	}
	int getArrowX() const {
		return arrow_x;
	}
	int getArrowY() const {
		return arrow_y;
	}

	void setAutoProceed(bool auto_proceed) {
		this->auto_proceed = auto_proceed;
	}
	bool autoProceed() const {
		return auto_proceed;
	}

	virtual bool canProceed(PlayingGameState *playing_gamestate) const {
		return true;
	}
};

class TutorialCardWaitForPanelPage : public TutorialCard {
	int wait_page;
public:
	TutorialCardWaitForPanelPage(const string &id, const string &text, int wait_page) : TutorialCard(id, text), wait_page(wait_page) {
		this->setAutoProceed(true);
	}

	virtual bool canProceed(PlayingGameState *playing_gamestate) const;
};

class TutorialCardWaitForDesign : public TutorialCard {
public:
	enum WaitType {
		WAITTYPE_CURRENT_DESIGN = 0,
		WAITTYPE_HAS_DESIGNED = 1
	};
private:
	WaitType wait_type;
	bool require_type;
	Invention::Type invention_type;
public:
	TutorialCardWaitForDesign(const string &id, const string &text, WaitType wait_type, bool require_type, Invention::Type invention_type) : TutorialCard(id, text), wait_type(wait_type), require_type(require_type), invention_type(invention_type) {
		this->setAutoProceed(true);
	}

	virtual bool canProceed(PlayingGameState *playing_gamestate) const;
};

class TutorialCardWaitForDeployedArmy : public TutorialCard {
	Sector *deploy_sector;
public:
	TutorialCardWaitForDeployedArmy(const string &id, const string &text, Sector *deploy_sector) : TutorialCard(id, text), deploy_sector(deploy_sector) {
		this->setAutoProceed(true);
	}

	virtual bool canProceed(PlayingGameState *playing_gamestate) const;
};

class Tutorial {
protected:
	string id;
	int start_map_x, start_map_y;
	int n_men;
	int card_index;
	vector<TutorialCard *> cards;

public:
	Tutorial(const string &id) : id(id), start_map_x(0), start_map_y(0), n_men(0), card_index(0) {
	}
	virtual ~Tutorial();

	string getId() const {
		return id;
	}
	int getStartMapX() const {
		return start_map_x;
	}
	int getStartMapY() const {
		return start_map_y;
	}
	int getNMen() const {
		return n_men;
	}
	const TutorialCard *getCard() const {
		if( card_index >= cards.size() )
			return NULL;
		return cards.at(card_index);
	}
	void proceed() {
		card_index++;
	}
	bool jumpTo(const string &id);
	void jumpToEnd() {
		card_index = cards.size();
	}

	virtual void initCards()=0;
};

class Tutorial1 : public Tutorial {
public:
	Tutorial1(const string &id);

	virtual void initCards();
};

extern Tutorial *tutorial;
