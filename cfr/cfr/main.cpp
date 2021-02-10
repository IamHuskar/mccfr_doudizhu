#include <iostream>
#include <valarray>
#include <fstream>
#include <filesystem>
#include "Pluribus.hpp"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "card.h"
#include <boost/filesystem.hpp>

int main01(){
	
    std::cout << "Running Pluribus\n";
    Pluribus train(2);

	//vector<int> hands({ 2,4,2,5,6});
	vector<int> hands({ 6, 8, 8, 11, 11, 11 });
	
	vector<Card> self_cards;
	for (int i = 0; i < hands.size(); i++) {
		self_cards.push_back(Card(hands[i]));
	}
	vector<int> last_hands({ 1,1 });
	vector<Card> last_cards;
	for (int i = 0; i < last_hands.size(); i++) {
		last_cards.push_back(Card(last_hands[i]));
	}
	CardGroup last_cardgroup = CardGroup(last_cards, Category::DOUBLE,1);

	State *s = new State();
	s->mTurn = 0;
	s->_last_group = last_cardgroup;
	for (size_t i = 0; i < 2; i++)
	{
		s->_players.push_back(new Player());
	}

	vector<int> next_hands({ 9,10,10,11 });
	vector<Card> next_cards;
	for (int i = 0; i < next_hands.size(); i++) {
		next_cards.push_back(Card(next_hands[i]));
	}

	s->_players[0]->_handcards = self_cards;
	s->_players[1]->_handcards = next_cards;
	sort(s->_players[0]->_handcards.begin(), s->_players[0]->_handcards.end());
	sort(s->_players[1]->_handcards.begin(), s->_players[1]->_handcards.end());

	s->_players[0]->calc_avail_actions();
	s->_players[1]->calc_avail_actions();
	train.mCurrentState = *s;
/*
	std::string action_last = last_cardgroup.to_str();
	if (action_last.empty()) {
		train.mCurrentState.mHistory.push_back("EMPTY");
	}
	else {
		train.mCurrentState.mHistory.push_back(action_last);
	}
	*/

    if(!boost::filesystem::exists("blueprint")){
         auto t1 = std::chrono::high_resolution_clock::now();
        train.train(50);
		/*
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
        

        for(auto map: train.mNodeMap){
            std::cout << "\nplayer " << map.first << "\n";
            for(auto infoSet: map.second){
                auto strat = infoSet.second.getAverageStrategy();
                std::cout << "\n" + infoSet.first + "\n";
                for(auto prob: strat){
                    std::cout << prob.first << "\t";
                    std::cout << prob.second << "\n";
                }
            }
        }
        std::valarray<float> expectedUtility = train.expectedUtility();

        for(int i=0; i<expectedUtility.size(); i++){
            std::cout << "\nPlayer " << i << " utility \n";
            std::cout << expectedUtility[i] << "\n";
        }

        std::cout <<"duration "<< duration/pow(10, 6) <<"\n";

        std::ofstream ofs("blueprint");
        boost::archive::text_oarchive oa(ofs);
        oa << train;
		*/
    }
    else{
        std::ifstream ifs("blueprint");
        boost::archive::text_iarchive ia(ifs);
        ia >> train;
        std::cout << "loaded blueprint";
		//train.play();
    } 
	
	std::string infoSet = train.mCurrentState.infoSet();
	auto search = train.mNodeMap[train.mCurrentState.mTurn].find(infoSet);
	if (search == train.mNodeMap[train.mCurrentState.mTurn].end()) {
		std::set<std::string> validActions = train.mCurrentState.validActions();
		train.mNodeMap[train.mCurrentState.mTurn].insert({ infoSet, InfoNode(validActions) });
		train.mValidActionsMap[infoSet] = validActions;
	}
	std::set<std::string> validActions = train.mValidActionsMap[infoSet];
	std::unordered_map<std::string, double> strategy = train.mNodeMap[train.mCurrentState.mTurn].at(infoSet).getStrategy(validActions);

	std::vector<std::string> actions;
	std::vector<double> probabilities;
	for (auto map : strategy) {
		actions.push_back(map.first);
		probabilities.push_back(map.second);
	}
	std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
	auto action = random_choice(train.mActionEng);
	std::string action_str = actions[action];

	std::cout << "zzx" << action_str << endl;
    return 0;
}

int main() {
	vector<int> hands({ 3,4,4,4,4,5,5,7,8,8,8,9,9,9,10,10,11,11,12,12 });
	vector<Card> self_cards;
	for (int i = 0; i < hands.size(); i++) {
		self_cards.push_back(Card(hands[i]));
	}
	vector<int> last_hands({});
	vector<Card> last_cards;
	for (int i = 0; i < last_hands.size(); i++) {
		last_cards.push_back(Card(last_hands[i]));
	}
	CardGroup last_cardgroup = CardGroup(last_cards, Category::EMPTY, 0);
	vector<int> unseen_hands({ 2,2,2,3,5,5,6,6,7,7,7,9,10,11,12,13,14 });
	vector<Card> unseen_cards;
	for (int i = 0; i < unseen_hands.size(); i++) {
		unseen_cards.push_back(Card(unseen_hands[i]));
	}


	int current_idx = 0;
	int n_r_weight = -1;
	int next_handcards_cnt = 17;
	int REWARD_WEIGHT = n_r_weight;
	int max_d = 10;

	auto seed = random_device{}();
	auto generator = mt19937(seed);
	Pluribus train(2);

	/*
	State *s = new State();
	s->mTurn = current_idx;
	int idx1 = (s->mTurn + 1) % 2;
	s->_last_group = last_cardgroup;
	for (size_t i = 0; i < 2; i++)
	{
		s->_players.push_back(new Player());
	}

	s->_players[current_idx]->_handcards = self_cards;
	sort(s->_players[current_idx]->_handcards.begin(), s->_players[0]->_handcards.end());
	s->_players[current_idx]->calc_avail_actions();

	for (int d = 0; d < max_d; d++)
	{
		State *ss = new State(*s);

		shuffle(unseen_cards.begin(), unseen_cards.end(), generator);
		ss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
		sort(ss->_players[idx1]->_handcards.begin(), ss->_players[idx1]->_handcards.end());
		ss->_players[idx1]->calc_avail_actions();
		train.mCurrentState = *ss;
		auto t1 = std::chrono::high_resolution_clock::now();
		train.train(50);
	}


	std::string infoSet = train.mCurrentState.infoSet();
	std::string actionSet = train.change_infoset2actionset(infoSet);
	auto search = train.mNodeMap[train.mCurrentState.mTurn].find(infoSet);
	if (search == train.mNodeMap[train.mCurrentState.mTurn].end()) {
		std::set<std::string> validActions = train.mCurrentState.validActions();
		train.mNodeMap[train.mCurrentState.mTurn].insert({ infoSet, InfoNode(validActions) });
		train.mValidActionsMap[actionSet] = validActions;
	}
	std::set<std::string> validActions = train.mValidActionsMap[actionSet];
	std::unordered_map<std::string, double> strategy = train.mNodeMap[train.mCurrentState.mTurn].at(infoSet).getStrategy(validActions);

	std::vector<std::string> actions;
	std::vector<double> probabilities;
	for (auto map : strategy) {
		actions.push_back(map.first);
		probabilities.push_back(map.second);
	}
	std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
	auto action = random_choice(train.mActionEng);
	std::string action_str = actions[action];

	CardGroup cand = train.mCurrentState.str2group(action_str);
	delete s;
	*/
	State *s = new State();
	s->mTurn = current_idx;
	int idx1 = (s->mTurn + 1) % 2;
	s->_last_group = last_cardgroup;
	for (size_t i = 0; i < 2; i++)
	{
		s->_players.push_back(new Player());
	}

	s->_players[current_idx]->_handcards = self_cards;
	sort(s->_players[current_idx]->_handcards.begin(), s->_players[0]->_handcards.end());
	s->_players[current_idx]->calc_avail_actions();

	for (size_t d = 0; d < max_d; d++)
	{
		State *ss = new State(*s);

		shuffle(unseen_cards.begin(), unseen_cards.end(), generator);
		ss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
		sort(ss->_players[idx1]->_handcards.begin(), ss->_players[idx1]->_handcards.end());
		ss->_players[idx1]->calc_avail_actions();
		train.mCurrentState = *ss;
		auto t1 = std::chrono::high_resolution_clock::now();
		train.train(100);
	}


	std::string infoSet = train.mCurrentState.infoSet();
	std::string actionSet = train.mCurrentState.actionSet();
	auto search = train.mNodeMap[train.mCurrentState.mTurn].find(infoSet);
	if (search == train.mNodeMap[train.mCurrentState.mTurn].end()) {
		std::set<std::string> validActions = train.mCurrentState.validActions();
		train.mNodeMap[train.mCurrentState.mTurn].insert({ infoSet, InfoNode(validActions) });
		train.mValidActionsMap[actionSet] = validActions;
	}
	std::set<std::string> validActions = train.mValidActionsMap[actionSet];
	std::unordered_map<std::string, double> strategy = train.mNodeMap[train.mCurrentState.mTurn].at(infoSet).getStrategy(validActions);

	std::vector<std::string> actions;
	std::vector<double> probabilities;
	for (auto map : strategy) {
		actions.push_back(map.first);
		probabilities.push_back(map.second);
	}
	std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
	auto action = random_choice(train.mActionEng);
	std::string action_str = actions[action];

	CardGroup cand = train.mCurrentState.str2group(action_str);
	delete s;
	return 0;
}