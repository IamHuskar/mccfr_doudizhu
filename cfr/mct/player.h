#pragma once
#include <vector>
#include "card.h"
#include <thread>
#include <mutex>
using namespace std;

class CEnv;
class Player
{
public:
    Player();
	Player(CEnv *env);
	Player(const Player &);
	~Player();

	void reset();
	void add_card(Card card);
	void remove_card(Card card);
	void remove_cards(vector<Card> cards);
	void calc_avail_actions();
	bool over();
	bool over(bool islord, int limit_card_num);
	vector<CardGroup> candidate_1(const CardGroup &last_card);
	vector<vector<CardGroup>::iterator> candidate(const CardGroup &last_card);
	const vector<CardGroup> &get_avail_actions() const;
	virtual CardGroup respond(const CardGroup &last_card);
	string to_str();

	friend ostream& operator <<(ostream& os, const Player& c);

	vector<int> _cnts;
	vector<CardGroup> _avail_actions;
	vector<Card> _handcards;
	CardGroup find_card_group(vector<Card> actionCards);
private:
	
protected:
	
	CEnv *_env;
	
};

class RandomPlayer : public Player
{
public:
	RandomPlayer(CEnv *env) : Player(env) {};
	CardGroup respond(const CardGroup &last_card) override;
};


class MCState;

class MCPlayer : public Player
{
public:
	MCPlayer(CEnv *env) : Player(env) {};
	CardGroup respond(const CardGroup &last_card) override;
	void multisearch(vector<int> &cnts, MCState *root);

private:
	mutex mu;
};