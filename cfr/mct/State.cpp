#include "State.hpp"
#include "mctree.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <algorithm>
#include "assert.h"
#include <iostream>
#include <sstream>
#include <boost/filesystem.hpp>
/*
State::State(int numPlayers, int numRounds, std::vector<int> cards, int numRaises): mTurn(0), mRound(0), mRaisesSoFar(0){
    mNumPlayers = numPlayers;
    mCards = cards;
    mBets.assign(numPlayers, 1);
    mTotalRounds = numRounds; 
    mRaises = numRaises;

    for(int i=0; i< numRounds; i++){
        mHistory.push_back(std::vector<std::string>());
    }
    
    mIn.assign(numPlayers, true);
};

State::State(const State &state, std::string action, bool search): mTurn(state.mTurn), mCards(state.mCards),
    mRound(state.mRound), mBets(state.mBets), mNumPlayers(state.mNumPlayers),
    mHistory(state.mHistory), mIn(state.mIn), mTotalRounds(state.mTotalRounds), 
    mRaises(state.mRaises), mRaisesSoFar(state.mRaisesSoFar){
        std::string lastAction = "EMPTY";
        if(!mHistory[mRound].empty()){
            lastAction = mHistory[mRound].back();
        }
        
        mHistory[mRound].push_back(action);

        if(action == "F"){
            mIn[mTurn] = false;
        }
        else if (action.find("R") != std::string::npos){
            int raiseSize = std::stoi(action.substr(0, action.size()-1));
            mBets[mTurn] += *std::max_element(mBets.begin(), mBets.end()) + raiseSize;
            mRaisesSoFar++;
        }
        else if(action == "C"){
            if(lastAction.find("R") != std::string::npos){
                int callSize = *std::max_element(mBets.begin(), mBets.end()) - mBets[mTurn];
                mBets[mTurn] += callSize;
            }
        }

        mTurn = (mTurn + 1) % mNumPlayers;

        int minActions = std::count(mIn.begin(), mIn.end(), true);
        int actionsInRound = mHistory[mRound].size();

        if(minActions <= actionsInRound && allCalledOrFolded()){
            mRound += 1;
            mRaisesSoFar = 0;
        }
}

State::~State(){
};

std::string State::infoSet(){
	vector<int> cs({ 1,2,3 });
	vector<Card> cards = int2Cards(cs);

    int player = mTurn;
    int card = mCards[player];

    std::string infoSet = std::to_string(card) + " | ";

    if(mRound > 0){
        infoSet += std::to_string(mCards[mNumPlayers]) + " | ";
    } 
    for(const auto &chunk: mHistory){
        for(const auto &piece: chunk){
            infoSet += piece;
        }
        infoSet += "|";
    }
    

    return infoSet;
};


std::string State::Cardgroup2String(CardGroup action) {

}

bool State::isTerminal(){
    int playersIn = std::count(mIn.begin(), mIn.end(), true);
    if(playersIn == 1){
        return true;
    }

    else if(mRound < mTotalRounds){
        return false;
    }
    else{
        int minActions = playersIn;
        int actionsInRound = mHistory.back().size();

        if(minActions <= actionsInRound && allCalledOrFolded()){
            return true;
        }
        return false;
    }
};

bool State::allCalledOrFolded(){
    int maxBet = *std::max_element(mBets.begin(), mBets.end());

    for(int i=0; i< mIn.size(); i++){
        if(mIn[i] && mBets[i] < maxBet){
            return false;
        }
    }

    return true;
};

std::valarray<float> State::payoff(){
    std::vector<int> winners;
    int playersIn = std::count(mIn.begin(), mIn.end(), true);
    if(playersIn == 1){
        std::vector<bool>::iterator found = std::find(mIn.begin(), mIn.end(), true);
        try{
            if(found != mIn.end()){
                int winner = found - mIn.begin();
                winners.push_back(winner);
            }
            else{
                throw std::logic_error("didn't find winner");
            }
        }
        
        catch(std::exception &ex){
            std::cout << "Something weird happened";
        }
    }
    else{
        winners = State::winners();
    }

    int pot = std::accumulate(mBets.begin(), mBets.end(), 0)/winners.size();

    std::valarray<float> payoffs(mNumPlayers);
    for(int i=0; i<mBets.size(); i++){
        payoffs[i] = -mBets[i];
    }

    for(auto win: winners){
        payoffs[win] += pot;
    }
    

    return payoffs;
};

std::vector<int> State::winners(){
    std::vector<int> scores;
    int boardCard = mCards[mNumPlayers];
    for(int player=0; player<mNumPlayers; player++){
        if(mCards[player] == boardCard){
            int score = 5*4 + boardCard;
            scores.push_back(score);
        }
        else{
            int score = 4 * std::max(boardCard, mCards[player]) + std::min(boardCard, mCards[player]);
            scores.push_back(score);
        }
    }

    std::vector<int> winners;
    int high = -1;
    for(int i=0; i<scores.size();i++){
        if(winners.size() == 0 || scores[i] > high){
            winners = {i};
            high = scores[i];
        }
        else if(scores[i] == high){
            winners.push_back(i);
        } 
    }

    return winners;
}

std::set<std::string> State::validActions(){



    std::set<std::string> actions;
    actions.insert("C");
    actions.insert("F");
    if(mRaisesSoFar < mRaises){
        int raiseSize = mRound == 0 ? 2 : 4;
        actions.insert(std::to_string(raiseSize) + "R");
    }
    return actions;
}
*/

State::State() {
	init_str_card_map();
}

State::State(const State &s) {
	mNodeMap = s.mNodeMap;
	_publiccards = s._publiccards;
	_last_group = s._last_group;
	_pre_group = s._pre_group;
	_isOver = s._isOver;
	_str_card_map = s._str_card_map;
	_card_str_map = s._card_str_map;
	mHistory = s.mHistory;
	_last_group = s._last_group;
	std::string action = _last_group.to_str();
	if (action.empty()) {
		mHistory.push_back("EMPTY");
	}
	else {
		mHistory.push_back(action);
	}
	mTurn = s.mTurn;
	for (size_t i = 0; i < 2; i++)
	{
		_players.push_back(new Player(*s._players[i]));
	}

}
State::State(const State &state, std::string action, int limitnum, bool search){
	mNodeMap = state.mNodeMap;
	_publiccards = state._publiccards;
	_str_card_map = state._str_card_map;
	_card_str_map = state._card_str_map;
	for (size_t i = 0; i < 2; i++)
	{
		_players.push_back(new Player(*state._players[i]));
	}
	mTurn = state.mTurn;
	mHistory = state.mHistory;
	_last_group = str2group(action);
	_pre_group = state._last_group;
	_players[mTurn]->remove_cards(_last_group._cards);
	//std::string lastAction = "EMPTY";
	if (action.empty()) {
		mHistory.push_back("EMPTY");
	}
	else {
		mHistory.push_back(action);
	}
	
	if (action.size() == 0) {
		_isOver = false;
	}
	else {
		bool islord = false;
		if (mTurn == 0) {
			islord = true;
			if (_players[mTurn]->over(islord, limitnum)) {
				_isOver = true;
				payoffs[0] = 1;
				payoffs[1] = -1;
			}
			else {
				_isOver = false;
			}
		}
		else {
			if (_players[mTurn]->over(islord, limitnum)) {
				_isOver = true;
				payoffs[0] = -1;
				payoffs[1] = 1;
			}
			else {
				_isOver = false;
			}
		}
	}
	
	mTurn = (mTurn + 1) % mNumPlayers;
}


State::~State() {
	for (auto player : _players)
	{
		delete player;
	}
};

vector<Card> State::int2Cards(vector<int> action) {
	mNumPlayers = 2;
	int e_size = action.size();
	vector<Card> cards;
	for (int i = 0; i < e_size; i++) {
		cards.push_back(Card(action[i]));
	}
	return cards;
}
void State::init_str_card_map() {
	vector<string> initcards({ "3","4","5","6","7","8","9","X","J","Q","K","A","2","*","$" });
	for (int i = 0; i < 15; i++) {
		_str_card_map.insert({ initcards[i],i });
		_card_str_map.insert({ i,initcards[i] });
	}
	
	if (boost::filesystem::exists("mcts")) {
		std::ifstream ifs("mcts");
		boost::archive::text_iarchive ia(ifs);
		ia >> mNodeMap;
		std::cout << "loaded mcts";
	}
	
}

CardGroup State::str2group(std::string s) {
	int e_size = s.size();
	vector<Card> cards;
	for (int i = 0; i < e_size; i++) {
		auto search = _str_card_map.find(string(1, s[i]));
		if (search == _str_card_map.end()) {
		}
		else {
			cards.push_back(Card(_str_card_map[string(1, s[i])]));
		}
	}
	if (cards.size() == 0) {
		return CardGroup(cards, Category::EMPTY, 0);
	}
	sort(cards.begin(), cards.end());
	return _players[mTurn]->find_card_group(cards);
}

std::string State::infoSet() {
	/*
	int player = mTurn;
	vector<Card> cards = _players[player]->_handcards;
	CardGroup g = CardGroup(cards, Category::SINGLE, 1);
	if (cards.size() == 0) {
		g = CardGroup(cards, Category::EMPTY, 1);
	}
	
	std::string infoSet = g.to_str() + " | ";

	for (const auto &chunk : mHistory) {
		for (const auto &piece : chunk) {
			infoSet += piece;
		}
		infoSet += "|";
	}
	return infoSet;
	*/
	return actionSet();
};

std::string State::actionSet() {
	/*
	int player = mTurn;
	vector<Card> cards = _players[player]->_handcards;
	CardGroup g = CardGroup(cards, Category::SINGLE, 1);
	if (cards.size() == 0) {
		g = CardGroup(cards, Category::EMPTY, 1);
	}

	std::string infoSet = g.to_str() + " | ";

	infoSet += mHistory.back();
	return infoSet;
	*/
	int player = mTurn;
	vector<Card> cards = _players[player]->_handcards;
	CardGroup g = CardGroup(cards, Category::SINGLE, 1);
	if (cards.size() == 0) {
		g = CardGroup(cards, Category::EMPTY, 1);
	}

	std::string infoSet = to_string(player) + "|" + g.to_str() + "|";

	int next_id = (player + 1) % 2;
	vector<Card> cards1 = _players[next_id]->_handcards;
	cards1.insert(cards1.end(), _publiccards.begin(), _publiccards.end());
	sort(cards1.begin(), cards1.end());
	CardGroup u = CardGroup(cards1, Category::SINGLE, 1);
	std::string unseencards = u.to_str();
	infoSet += unseencards;
	infoSet += "|";

	sort(_last_group._cards.begin(), _last_group._cards.end());
	sort(_pre_group._cards.begin(), _pre_group._cards.end());

	std::string ls = _last_group.to_str();
	std::string pre = _pre_group.to_str();

	if (pre.empty()) {
		infoSet += "EMPTY";
	}
	else {
		infoSet += pre;
	}

	infoSet += "|";

	if (ls.empty()) {
		infoSet += "EMPTY";
	}
	else {
		infoSet += ls;
	}


	return infoSet;
};
std::string State::mcts_actionSet() {
	int player = mTurn;
	vector<Card> cards = _players[player]->_handcards;
	CardGroup g = CardGroup(cards, Category::SINGLE, 1);
	if (cards.size() == 0) {
		g = CardGroup(cards, Category::EMPTY, 1);
	}

	std::string infoSet = to_string(player) + "|" + g.to_str() + "|";

	int next_id = (player + 1) % 2;
	vector<Card> cards1 = _players[next_id]->_handcards;
	cards1.insert(cards1.end(), _publiccards.begin(), _publiccards.end());
	sort(cards1.begin(), cards1.end());
	CardGroup u = CardGroup(cards1, Category::SINGLE, 1);
	std::string unseencards = u.to_str();
	infoSet += unseencards;
	infoSet += "|";

	std::string ls = _last_group.to_str();
	if (ls.empty()) {
		infoSet += "EMPTY";
	}
	else {
		infoSet += ls;
	}

	return infoSet;
};


bool State::isTerminal() {
	return _isOver;
};

std::valarray<float> State::payoff() {
	return payoffs;
}

std::set<std::string> State::validActions() {
	std::set<std::string> actions;
	vector<CardGroup> group_actions = _players[mTurn]->candidate_1(_last_group);
	for (int i = 0; i < group_actions.size(); i++) {
		CardGroup g = group_actions[i];
		std::string s = g.to_str();
		actions.insert(s);
	}
	if (actions.size() > 300000) {
		std::string inf = actionSet();
		if (boost::filesystem::exists("mcts")) {

			float maxq = -100000;
			string taction;
			int q0 = 0;
			auto search = mNodeMap.find(inf);
			if (search == mNodeMap.end()) {
				cout << "shit" << endl;
			}
			else {
				cout << "find" << endl;
				std::vector<std::string> action_space;
				std::vector<double> probabilities;

				for (auto map : mNodeMap.at(inf)) {
					action_space.push_back(map.first);
					probabilities.push_back(map.second);

				}
				if (action_space.size() > 3) {
					vector<int> index(probabilities.size(), 0);
					for (int i = 0; i != index.size(); i++) {
						index[i] = i;
					}
					sort(index.begin(), index.end(),
						[&](const int& a, const int& b) {
						return (probabilities[a] > probabilities[b]);
					}
					);
					std::set<std::string> str_actions;
					for (int i = 0; i != index.size(); i++) {
						if (i < 3) {
							CardGroup g = str2group(action_space[index[i]]);
							std::string s = g.to_str();
							str_actions.insert(s);
						}
					}
					return str_actions;
				}

			}

		}

	}
	
	return actions;
}
