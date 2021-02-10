#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <valarray>
#include <set>
#include "card.h"
#include <boost/serialization/unordered_map.hpp>
#include "player.h"
/*
class State{
    public:
        State(int numPlayers, int numRound, std::vector<int> cards, int numRaises=0);
        State(const State &state, std::string action, bool search=false);
        ~State();
        std::string infoSet();
        bool isTerminal();
        std::valarray<float> payoff();
        std::vector<int> winners();
        bool allCalledOrFolded();
        std::set<std::string> validActions();
        
        std::vector<int> mCards;
        std::vector<int> mBets;
        std::vector<std::vector<std::string>> mHistory;
        std::vector<bool> mIn;
        
        int mNumPlayers;
        int mTurn;
        
        int mTotalRounds;
        int mRound;
        int mRaises;
        int mRaisesSoFar;
};
*/
class State {
public:
	State();
	State(const State &state, std::string action, bool search = false);
	State(const State &s);
	~State();
	std::string infoSet();
	std::string actionSet();
	bool isTerminal();
	std::valarray<float> payoffs{0.0,0.0};
	std::valarray<float> payoff();
	std::vector<int> winners();
	std::set<std::string> validActions();
	std::vector<int> mCards;
	std::vector<std::string> mHistory;
	int mNumPlayers = 2;
	int mTurn;
	std::string lastAction = "EMPTY";

	bool _isOver = false;
	int _winner = -1;
	vector<Card> int2Cards(vector<int> action);
	std::unordered_map<std::string, int> _str_card_map;
	std::unordered_map<int, std::string> _card_str_map;
	CardGroup str2group(std::string s);
	void init_str_card_map();
	vector<Player*> _players;
	CardGroup _last_group;
};
