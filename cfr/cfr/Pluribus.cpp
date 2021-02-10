#include "Pluribus.hpp"
/*
Pluribus::Pluribus(int numPlayers): mCards({1,2,3,1,2,3}), 
            mCurrentState(State(numPlayers, 2, mCards, 2)), mPublicState(0){
    mNumPlayers = numPlayers;

    std::random_device mRd;
    std::mt19937 mActionEng(mRd());
};

Pluribus::~Pluribus(){
};

void Pluribus::train(int iterations){
    std::random_device rd;
    std::mt19937 randEng(rd());
    for(int i=1; i<=iterations; i++){
        if(i%1000 == 0){
            std::cout << "\nIteration "<< i;
        }
        std::shuffle(mCards.begin(), mCards.end(), randEng);
        State state(mNumPlayers, 2, mCards, 2);
        for(int player=0; player<mNumPlayers;player++){
            if(i % mStrategyInterval == 0){
                Pluribus::updateStrategy(state, player);
            }
            if(i > mPruneThreshold){
                float prune = (float) rand()/RAND_MAX;
                if(prune < .05){
                    Pluribus::mccfr(state, player);
                }
                else{
                    Pluribus::mccfr(state, player, true);
                }
            }
            else{
                Pluribus::mccfr(state, player);
            }
        }
        if(i < mLCFRThreshold && i % mDiscountInterval == 0){
            float discount = (i/mDiscountInterval)/((i/mDiscountInterval) + 1.);
            for(auto map: mNodeMap){
                std::unordered_map<std::string, InfoNode> playerNodes = mNodeMap[map.first];
                for(auto keyValue: playerNodes){
                    auto validActions = mValidActionsMap[keyValue.first];
                    for(auto action: validActions){
                        keyValue.second.regretSum.at(action) *= discount;
                        keyValue.second.strategySum.at(action) *= discount;
                    }
                }
            }
        }
    }
};

std::valarray<float> Pluribus::mccfr(State state, int player, bool prune){
    if(state.isTerminal()){
        std::valarray<float> util = state.payoff();
        return util;
    }

    int currentPlayer = state.mTurn;

    if(currentPlayer == player){
        std::string infoSet = state.infoSet();
        auto search = mNodeMap[currentPlayer].find(infoSet);
        if(search == mNodeMap[currentPlayer].end()){
            std::set<std::string> validActions = state.validActions();
            mNodeMap[currentPlayer].insert({infoSet, InfoNode(validActions)});
            mValidActionsMap[infoSet] = validActions;
        }

        std::set<std::string> validActions = mValidActionsMap[infoSet];
        std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);

        std::unordered_map<std::string, double> utilities;

        std::valarray<float> nodeUtil(state.mNumPlayers);

        
        std::valarray<float> returned;
        float regret = 0.0;

        if(prune){
            std::set<std::string> explored;
            for(auto action: validActions){
                if(mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) > mRegretMinimum){
                    returned = mccfr(State(state, action), player, prune);
                    utilities[action] = returned[currentPlayer];
					nodeUtil += returned * (float)strategy.at(action);
                    explored.insert(action);
                }
            }
            for(auto action: validActions){
                auto search = explored.find(action);
                if(search != explored.end()){
                    regret = utilities.at(action) - nodeUtil[currentPlayer];
                    mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) += regret;
                }
            }
        }
        else{
            for(auto action: validActions){
                returned = mccfr(State(state, action), player, prune);
                utilities[action] = returned[currentPlayer];
                //nodeUtil += returned * strategy.at(action); 
				nodeUtil += returned * (float)strategy.at(action);
            }
            for(auto action: validActions){
                regret = utilities.at(action) - nodeUtil[currentPlayer];
                mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) += regret;
            }  
        }
        return nodeUtil;
    }
    else{
        std::string infoSet = state.infoSet();
        auto search = mNodeMap[currentPlayer].find(infoSet);
        if(search == mNodeMap[currentPlayer].end()){
            std::set<std::string> validActions = state.validActions();
            mNodeMap[currentPlayer].insert({infoSet, InfoNode(validActions)});
            mValidActionsMap[infoSet] = validActions;
        }
        std::set<std::string> validActions = mValidActionsMap[infoSet];
        std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);

        std::vector<std::string> actions;
        std::vector<double> probabilities;
        for(auto map: strategy){
            actions.push_back(map.first);
            probabilities.push_back(map.second);
        }
        std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
        auto action = random_choice(mActionEng);
        return mccfr(State(state, actions[action]), player, prune);
    }
};

void Pluribus::updateStrategy(State state, int player){
    if(state.isTerminal()){
        return;
    }
    int currentPlayer = state.mTurn;

    if(currentPlayer == player){
        std::string infoSet = state.infoSet();
        auto search = mNodeMap[currentPlayer].find(infoSet);
        if(search == mNodeMap[currentPlayer].end()){
            std::set<std::string> validActions = state.validActions();
            mNodeMap[currentPlayer].insert({infoSet, InfoNode(validActions)});
            mValidActionsMap[infoSet] = validActions;
        }
        std::set<std::string> validActions = mValidActionsMap[infoSet];
        std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);

        std::vector<std::string> actions;
        std::vector<double> probabilities;
        for(auto map: strategy){
            actions.push_back(map.first);
            probabilities.push_back(map.second);
        }
        std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
        auto action = random_choice(mActionEng);

        mNodeMap[currentPlayer].at(infoSet).strategySum.at(actions[action]) += 1;
        updateStrategy(State(state, actions[action]), player);
    }
    else{
        std::string infoSet = state.infoSet();
        auto search = mValidActionsMap.find(infoSet);
        if(search == mValidActionsMap.end()){
            std::set<std::string> validActions = state.validActions();
            mNodeMap[currentPlayer].insert({infoSet, InfoNode(validActions)});
            mValidActionsMap[infoSet] = validActions;
        }
        std::set<std::string> validActions = mValidActionsMap[infoSet];
        for(auto action: validActions){
            updateStrategy(State(state, action), player);
        }
    }
};

std::valarray<float> Pluribus::expectedUtility(){
    std::valarray<float> expectedUtility(mNumPlayers);

    std::sort(mCards.begin(), mCards.end());
    int numPermutations = 0;
    do{
        State state(mNumPlayers, 2, mCards, 2);
        expectedUtility += traverseTree(state);
        numPermutations += 1;
    }while(std::next_permutation(mCards.begin(), mCards.end()));

    return expectedUtility/(float)numPermutations;
};

std::valarray<float> Pluribus::traverseTree(State state){
    if(state.isTerminal()){
        std::valarray<float> util = state.payoff();
        return util;
    }

    int player = state.mTurn;
    std::string infoSet = state.infoSet();
	//zzx
	auto search = mNodeMap[player].find(infoSet);
	if (search == mNodeMap[player].end()) {
		std::set<std::string> validActions = state.validActions();
		mNodeMap[player].insert({ infoSet, InfoNode(validActions) });
		mValidActionsMap[infoSet] = validActions;
	}
	//zzx
    std::set<std::string> validActions = mValidActionsMap[infoSet];
	InfoNode node = mNodeMap[player].at(infoSet);
    std::unordered_map<std::string, double> strategy = node.getAverageStrategy();

    std::valarray<float> expectedUtility(mNumPlayers);

    for(auto action: validActions){
        expectedUtility += traverseTree(State(state, action)) * (float)strategy.at(action);
    }

    return expectedUtility;
};

void Pluribus::search(int iterations){
    std::random_device rd;
    std::mt19937 randEng(rd());
    for(int i=1; i<=iterations; i++){
        if(i%1000 == 0){
            std::cout << "\nSolving, Iteration "<< i;
        }
        std::shuffle(mCards.begin(), mCards.end(), randEng);
        State state(mNumPlayers, 2, mCards, 2);
        for(int player=0; player<mNumPlayers;player++){
            if(i % mStrategyInterval == 0){
                Pluribus::subgameUpdate(state, player);
            }
            if(i > mPruneThreshold){
                float prune = (float) rand()/RAND_MAX;
                if(prune < .05){
                    Pluribus::subgameSolve(state, player);
                }
                else{
                    Pluribus::subgameSolve(state, player, true);
                }
            }
            else{
                Pluribus::subgameSolve(state, player);
            }
        }
        if(i < mLCFRThreshold && i % mDiscountInterval == 0){
            float discount = (i/mDiscountInterval)/((i/mDiscountInterval) + 1.);
            for(auto map: mNodeMap){
                std::unordered_map<std::string, InfoNode> playerNodes = mNodeMap[map.first];
                for(auto keyValue: playerNodes){
                    auto validActions = mValidActionsMap[keyValue.first];
                    for(auto action: validActions){
                        keyValue.second.regretSum.at(action) *= discount;
                        keyValue.second.strategySum.at(action) *= discount;
                    }
                }
            }
        }
    }
};

std::valarray<float> Pluribus::subgameSolve(State state, int player, bool prune){
    if(state.isTerminal()){
        std::valarray<float> util = state.payoff();
        return util;
    }

    int currentPlayer = state.mTurn;

    if(currentPlayer == player){
        std::string infoSet = state.infoSet();
        auto frozenAction = mFrozenNodes.find(infoSet);
        if(frozenAction == mFrozenNodes.end()){
            auto search = mNodeMap[currentPlayer].find(infoSet);
            if(search == mNodeMap[currentPlayer].end()){
                std::set<std::string> validActions = state.validActions();
                mNodeMap[currentPlayer].insert({infoSet, InfoNode(validActions)});
                mValidActionsMap[infoSet] = validActions;
            }

            std::set<std::string> validActions = mValidActionsMap[infoSet];
            std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);
            std::unordered_map<std::string, double> utilities;

            std::valarray<float> nodeUtil(state.mNumPlayers);
            std::valarray<float> returned;
            float regret = 0.0;
            if(prune){
                std::set<std::string> explored;
                for(auto action: validActions){
                    if(mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) > mRegretMinimum){
                        returned = subgameSolve(State(state, action), player, prune);
                        utilities[action] = returned[currentPlayer];
                        //nodeUtil += returned * strategy.at(action);
						nodeUtil += returned * (float)strategy.at(action);
                        explored.insert(action);
                    }
                }
                for(auto action: validActions){
                    auto search = explored.find(action);
                    if(search != explored.end()){
                        regret = utilities.at(action) - nodeUtil[currentPlayer];
                        mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) += regret;
                    }
                }
            }
            else{
                for(auto action: validActions){
                    returned = subgameSolve(State(state, action), player, prune);
                    utilities[action] = returned[currentPlayer];
                    //nodeUtil += returned * strategy.at(action); 
					nodeUtil += returned * (float)strategy.at(action);

                }
                for(auto action: validActions){
                    regret = utilities.at(action) - nodeUtil[currentPlayer];
                    mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) += regret;
                }  
            }
            return nodeUtil;
        }
        else{
            // may need to check that we are not at a leaf node
            return subgameSolve(State(state, frozenAction->second), player, prune);
        }
        
    }
    else{
        std::string infoSet = state.infoSet();
        auto frozenAction = mFrozenNodes.find(infoSet);
        if(frozenAction == mFrozenNodes.end()){
            auto search = mNodeMap[currentPlayer].find(infoSet);
            if(search == mNodeMap[currentPlayer].end()){
                std::set<std::string> validActions = state.validActions();
                mNodeMap[currentPlayer].insert({infoSet, InfoNode(validActions)});
                mValidActionsMap[infoSet] = validActions;
            }
            std::set<std::string> validActions = mValidActionsMap[infoSet];
            std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);

            std::vector<std::string> actions;
            std::vector<double> probabilities;
            for(auto map: strategy){
                actions.push_back(map.first);
                probabilities.push_back(map.second);
            }
            std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
            auto action = random_choice(mActionEng);
            return subgameSolve(State(state, actions[action]), player, prune);
        }
        else{
            return subgameSolve(State(state, frozenAction->second), player, prune);
        }
    }
};

void Pluribus::subgameUpdate(State state, int player){
    if(state.isTerminal()){
        return;
    }
    int currentPlayer = state.mTurn;

    if(currentPlayer == player){
        std::string infoSet = state.infoSet();
        auto frozenAction = mFrozenNodes.find(infoSet);
        if(frozenAction == mFrozenNodes.end()){
            auto search = mNodeMap[currentPlayer].find(infoSet);
            if(search == mNodeMap[currentPlayer].end()){
                std::set<std::string> validActions = state.validActions();
                mNodeMap[currentPlayer].insert({infoSet, InfoNode(validActions)});
                mValidActionsMap[infoSet] = validActions;
            }
            
            std::set<std::string> validActions = mValidActionsMap[infoSet];
            std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);

            std::vector<std::string> actions;
            std::vector<double> probabilities;
            for(auto map: strategy){
                actions.push_back(map.first);
                probabilities.push_back(map.second);
            }
            std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
            auto action = random_choice(mActionEng);

            mNodeMap[currentPlayer].at(infoSet).strategySum.at(actions[action]) += 1;
            subgameUpdate(State(state, actions[action]), player);
        }
        else{
            subgameUpdate(State(state, frozenAction->second), player);
        }
    }
    else{
        std::string infoSet = state.infoSet();
        auto frozenAction = mFrozenNodes.find(infoSet);
        if(frozenAction == mFrozenNodes.end()){
            auto search = mValidActionsMap.find(infoSet);
            if(search == mValidActionsMap.end()){
                std::set<std::string> validActions = state.validActions();
                mNodeMap[currentPlayer].insert({infoSet, InfoNode(validActions)});
                mValidActionsMap[infoSet] = validActions;
            }
            std::set<std::string> validActions = mValidActionsMap[infoSet];
            for(auto action: validActions){
                subgameUpdate(State(state, action), player);
            }
        }
        else{
            subgameUpdate(State(state, frozenAction->second), player);
        }
    }
};

void Pluribus::play(){
    std::random_device rd;
    std::mt19937 randEng(rd());
    std::shuffle(mCards.begin(), mCards.end(), randEng);
    mFrozenNodes.clear();

    while(!mCurrentState.isTerminal()){
        int turn = mCurrentState.mTurn;
        if(turn == 0){
            std::string action;
            std::cout << "Play an action; ";
            std::cin >> action;
            opponentTurn(action);
        }
        else{
            pluribusTurn();
        }
    }
};

void Pluribus::opponentTurn(std::string action){
    std::string infoSet = mCurrentState.infoSet();
    std::set<std::string> validActions = mValidActionsMap[infoSet];
    if(validActions.find(action) == validActions.end()){
        // add action to all public states that share infoset
        search(10000);
    }
    mCurrentState = State(mCurrentState, action);
    checkNewRound();
};

void Pluribus::pluribusTurn(){
    std::string infoSet = mCurrentState.infoSet();
    std::set<std::string> validActions = mValidActionsMap[infoSet];
    int player = mCurrentState.mTurn;
    // do we want average strategy or just strategy?
    std::unordered_map<std::string, double> strategy = mNodeMap[player].at(infoSet).getStrategy(validActions);

    std::vector<std::string> actions;
    std::vector<double> probabilities;
    for(auto map: strategy){
        actions.push_back(map.first);
        probabilities.push_back(map.second);
    }
    std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
    auto action = random_choice(mActionEng);

    mFrozenNodes[infoSet] = actions[action];
    mCurrentState = State(mCurrentState, actions[action]);
    checkNewRound();
};

void Pluribus::checkNewRound(){
    if(mCurrentState.mRound > mPublicState){
        mPublicState++;
        search(10000);
    }
}
*/
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
Pluribus::Pluribus(int numPlayers) :mPublicState(0) {
	mNumPlayers = numPlayers;

	std::random_device mRd;
	std::mt19937 mActionEng(mRd());
};

Pluribus::~Pluribus() {
};

void Pluribus::train(int iterations) {
	std::random_device rd;
	std::mt19937 randEng(rd());
	for (int i = 1; i <= iterations; i++) {
		if (i % 10 == 0) {
			std::cout << "\nIteration " << i;
		}

		for (int player = 0; player<mNumPlayers; player++) {
			if (i % mStrategyInterval == 0) {
				Pluribus::updateStrategy(mCurrentState, player);
			}
			if (i > mPruneThreshold) {
				float prune = (float)rand() / RAND_MAX;
				if (prune < .05) {
					Pluribus::mccfr(mCurrentState, player);
				}
				else {
					Pluribus::mccfr(mCurrentState, player, true);
				}
			}
			else {
				Pluribus::mccfr(mCurrentState, player);
			}
		}
		if (i < mLCFRThreshold && i % mDiscountInterval == 0) {
			float discount = (i / mDiscountInterval) / ((i / mDiscountInterval) + 1.);
			for (auto map : mNodeMap) {
				std::unordered_map<std::string, InfoNode> playerNodes = mNodeMap[map.first];
				for (auto keyValue : playerNodes) {
					//auto validActions = mValidActionsMap[keyValue.first];
					std::string actionSet = change_infoset2actionset(keyValue.first);
					auto validActions = mValidActionsMap[actionSet];

					for (auto action : validActions) {
						keyValue.second.regretSum.at(action) *= discount;
						keyValue.second.strategySum.at(action) *= discount;
					}
				}
			}
		}
	}
};

std::valarray<float> Pluribus::mccfr(State state, int player, bool prune) {
	if (state.isTerminal()) {
		std::valarray<float> util = state.payoff();
		return util;
	}

	int currentPlayer = state.mTurn;

	if (currentPlayer == player) {
		std::string infoSet = state.infoSet();
		std::string actionSet = state.actionSet();
		auto search = mNodeMap[currentPlayer].find(infoSet);
		if (search == mNodeMap[currentPlayer].end()) {
			std::set<std::string> validActions = state.validActions();
			mNodeMap[currentPlayer].insert({ infoSet, InfoNode(validActions) });
			mValidActionsMap[actionSet] = validActions;
		}

		std::set<std::string> validActions = mValidActionsMap[actionSet];
		std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);

		std::unordered_map<std::string, double> utilities;

		std::valarray<float> nodeUtil(state.mNumPlayers);


		std::valarray<float> returned;
		float regret = 0.0;

		if (prune) {
			std::set<std::string> explored;
			for (auto action : validActions) {
				if (mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) > mRegretMinimum) {
					returned = mccfr(State(state, action), player, prune);
					utilities[action] = returned[currentPlayer];
					nodeUtil += returned * (float)strategy.at(action);
					explored.insert(action);
				}
			}
			for (auto action : validActions) {
				auto search = explored.find(action);
				if (search != explored.end()) {
					regret = utilities.at(action) - nodeUtil[currentPlayer];
					mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) += regret;
				}
			}
		}
		else {
			for (auto action : validActions) {
				returned = mccfr(State(state, action), player, prune);
				utilities[action] = returned[currentPlayer];
				//nodeUtil += returned * strategy.at(action); 
				nodeUtil += returned * (float)strategy.at(action);
			}
			for (auto action : validActions) {
				regret = utilities.at(action) - nodeUtil[currentPlayer];
				mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) += regret;
			}
		}
		return nodeUtil;
	}
	else {
		std::string infoSet = state.infoSet();
		std::string actionSet = state.actionSet();
		auto search = mNodeMap[currentPlayer].find(infoSet);
		if (search == mNodeMap[currentPlayer].end()) {
			std::set<std::string> validActions = state.validActions();
			mNodeMap[currentPlayer].insert({ infoSet, InfoNode(validActions) });
			mValidActionsMap[actionSet] = validActions;
		}
		std::set<std::string> validActions = mValidActionsMap[actionSet];
		std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);

		std::vector<std::string> actions;
		std::vector<double> probabilities;
		for (auto map : strategy) {
			actions.push_back(map.first);
			probabilities.push_back(map.second);
		}
		std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
		auto action = random_choice(mActionEng);
		return mccfr(State(state, actions[action]), player, prune);
	}
};

void Pluribus::updateStrategy(State state, int player) {
	if (state.isTerminal()) {
		return;
	}
	int currentPlayer = state.mTurn;

	if (currentPlayer == player) {
		std::string infoSet = state.infoSet();
		std::string actionSet = state.actionSet();
		auto search = mNodeMap[currentPlayer].find(infoSet);
		if (search == mNodeMap[currentPlayer].end()) {
			std::set<std::string> validActions = state.validActions();
			mNodeMap[currentPlayer].insert({ infoSet, InfoNode(validActions) });
			mValidActionsMap[actionSet] = validActions;
		}
		std::set<std::string> validActions = mValidActionsMap[actionSet];
		std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);

		std::vector<std::string> actions;
		std::vector<double> probabilities;
		for (auto map : strategy) {
			actions.push_back(map.first);
			probabilities.push_back(map.second);
		}
		std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
		auto action = random_choice(mActionEng);

		mNodeMap[currentPlayer].at(infoSet).strategySum.at(actions[action]) += 1;
		updateStrategy(State(state, actions[action]), player);
	}
	else {
		std::string infoSet = state.infoSet();
		std::string actionSet = state.actionSet();
		auto search = mValidActionsMap.find(actionSet);
		if (search == mValidActionsMap.end()) {
			std::set<std::string> validActions = state.validActions();
			mNodeMap[currentPlayer].insert({ infoSet, InfoNode(validActions) });
			mValidActionsMap[actionSet] = validActions;
		}
		std::set<std::string> validActions = mValidActionsMap[actionSet];
		for (auto action : validActions) {
			updateStrategy(State(state, action), player);
		}
	}
};


std::valarray<float> Pluribus::traverseTree(State state) {
	if (state.isTerminal()) {
		std::valarray<float> util = state.payoff();
		return util;
	}

	int player = state.mTurn;
	std::string infoSet = state.infoSet();
	std::string actionSet = state.actionSet();
	//zzx
	auto search = mNodeMap[player].find(infoSet);
	if (search == mNodeMap[player].end()) {
		std::set<std::string> validActions = state.validActions();
		mNodeMap[player].insert({ infoSet, InfoNode(validActions) });
		mValidActionsMap[actionSet] = validActions;
	}
	//zzx
	std::set<std::string> validActions = mValidActionsMap[actionSet];
	InfoNode node = mNodeMap[player].at(infoSet);
	std::unordered_map<std::string, double> strategy = node.getAverageStrategy();

	std::valarray<float> expectedUtility(mNumPlayers);

	for (auto action : validActions) {
		expectedUtility += traverseTree(State(state, action)) * (float)strategy.at(action);
	}

	return expectedUtility;
};

void Pluribus::search(int iterations) {
	std::random_device rd;
	std::mt19937 randEng(rd());
	for (int i = 1; i <= iterations; i++) {
		if (i % 1000 == 0) {
			std::cout << "\nSolving, Iteration " << i;
		}
		for (int player = 0; player<mNumPlayers; player++) {
			if (i % mStrategyInterval == 0) {
				Pluribus::subgameUpdate(mCurrentState, player);
			}
			if (i > mPruneThreshold) {
				float prune = (float)rand() / RAND_MAX;
				if (prune < .05) {
					Pluribus::subgameSolve(mCurrentState, player);
				}
				else {
					Pluribus::subgameSolve(mCurrentState, player, true);
				}
			}
			else {
				Pluribus::subgameSolve(mCurrentState, player);
			}
		}
		if (i < mLCFRThreshold && i % mDiscountInterval == 0) {
			float discount = (i / mDiscountInterval) / ((i / mDiscountInterval) + 1.);
			for (auto map : mNodeMap) {
				std::unordered_map<std::string, InfoNode> playerNodes = mNodeMap[map.first];
				for (auto keyValue : playerNodes) {
					//auto validActions = mValidActionsMap[keyValue.first];
					std::string actionSet = change_infoset2actionset(keyValue.first);
					auto validActions = mValidActionsMap[actionSet];
					for (auto action : validActions) {
						keyValue.second.regretSum.at(action) *= discount;
						keyValue.second.strategySum.at(action) *= discount;
					}
				}
			}
		}
	}
};

std::string Pluribus::change_infoset2actionset(std::string str) {
	//vector<string> vStr;
	//boost::split(vStr, infoSet, boost::is_any_of("|"), boost::token_compress_on);
	//return vStr.back();
	vector<string>tokens;
	std::string delimiters = "|";
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	string::size_type pos = str.find_first_of(delimiters, lastPos);
	while (string::npos != pos || string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
	return tokens.back();
}


std::valarray<float> Pluribus::subgameSolve(State state, int player, bool prune) {
	if (state.isTerminal()) {
		std::valarray<float> util = state.payoff();
		return util;
	}

	int currentPlayer = state.mTurn;

	if (currentPlayer == player) {
		std::string infoSet = state.infoSet();
		std::string actionSet = state.actionSet();
		auto frozenAction = mFrozenNodes.find(infoSet);
		if (frozenAction == mFrozenNodes.end()) {
			auto search = mNodeMap[currentPlayer].find(infoSet);
			if (search == mNodeMap[currentPlayer].end()) {
				std::set<std::string> validActions = state.validActions();
				mNodeMap[currentPlayer].insert({ infoSet, InfoNode(validActions) });
				mValidActionsMap[actionSet] = validActions;
			}

			std::set<std::string> validActions = mValidActionsMap[actionSet];
			std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);
			std::unordered_map<std::string, double> utilities;

			std::valarray<float> nodeUtil(state.mNumPlayers);
			std::valarray<float> returned;
			float regret = 0.0;
			if (prune) {
				std::set<std::string> explored;
				for (auto action : validActions) {
					if (mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) > mRegretMinimum) {
						returned = subgameSolve(State(state, action), player, prune);
						utilities[action] = returned[currentPlayer];
						//nodeUtil += returned * strategy.at(action);
						nodeUtil += returned * (float)strategy.at(action);
						explored.insert(action);
					}
				}
				for (auto action : validActions) {
					auto search = explored.find(action);
					if (search != explored.end()) {
						regret = utilities.at(action) - nodeUtil[currentPlayer];
						mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) += regret;
					}
				}
			}
			else {
				for (auto action : validActions) {
					returned = subgameSolve(State(state, action), player, prune);
					utilities[action] = returned[currentPlayer];
					//nodeUtil += returned * strategy.at(action); 
					nodeUtil += returned * (float)strategy.at(action);

				}
				for (auto action : validActions) {
					regret = utilities.at(action) - nodeUtil[currentPlayer];
					mNodeMap[currentPlayer].at(infoSet).regretSum.at(action) += regret;
				}
			}
			return nodeUtil;
		}
		else {
			// may need to check that we are not at a leaf node
			return subgameSolve(State(state, frozenAction->second), player, prune);
		}

	}
	else {
		std::string infoSet = state.infoSet();
		std::string actionSet = state.actionSet();
		auto frozenAction = mFrozenNodes.find(infoSet);
		if (frozenAction == mFrozenNodes.end()) {
			auto search = mNodeMap[currentPlayer].find(infoSet);
			if (search == mNodeMap[currentPlayer].end()) {
				std::set<std::string> validActions = state.validActions();
				mNodeMap[currentPlayer].insert({ infoSet, InfoNode(validActions) });
				mValidActionsMap[actionSet] = validActions;
			}
			std::set<std::string> validActions = mValidActionsMap[actionSet];
			std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);

			std::vector<std::string> actions;
			std::vector<double> probabilities;
			for (auto map : strategy) {
				actions.push_back(map.first);
				probabilities.push_back(map.second);
			}
			std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
			auto action = random_choice(mActionEng);
			return subgameSolve(State(state, actions[action]), player, prune);
		}
		else {
			return subgameSolve(State(state, frozenAction->second), player, prune);
		}
	}
};

void Pluribus::subgameUpdate(State state, int player) {
	if (state.isTerminal()) {
		return;
	}
	int currentPlayer = state.mTurn;

	if (currentPlayer == player) {
		std::string infoSet = state.infoSet();
		std::string actionSet = state.actionSet();
		auto frozenAction = mFrozenNodes.find(infoSet);
		if (frozenAction == mFrozenNodes.end()) {
			auto search = mNodeMap[currentPlayer].find(infoSet);
			if (search == mNodeMap[currentPlayer].end()) {
				std::set<std::string> validActions = state.validActions();
				mNodeMap[currentPlayer].insert({ infoSet, InfoNode(validActions) });
				mValidActionsMap[actionSet] = validActions;
			}

			std::set<std::string> validActions = mValidActionsMap[actionSet];
			std::unordered_map<std::string, double> strategy = mNodeMap[currentPlayer].at(infoSet).getStrategy(validActions);

			std::vector<std::string> actions;
			std::vector<double> probabilities;
			for (auto map : strategy) {
				actions.push_back(map.first);
				probabilities.push_back(map.second);
			}
			std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
			auto action = random_choice(mActionEng);

			mNodeMap[currentPlayer].at(infoSet).strategySum.at(actions[action]) += 1;
			subgameUpdate(State(state, actions[action]), player);
		}
		else {
			subgameUpdate(State(state, frozenAction->second), player);
		}
	}
	else {
		std::string infoSet = state.infoSet();
		std::string actionSet = state.actionSet();
		auto frozenAction = mFrozenNodes.find(infoSet);
		if (frozenAction == mFrozenNodes.end()) {
			auto search = mValidActionsMap.find(actionSet);
			if (search == mValidActionsMap.end()) {
				std::set<std::string> validActions = state.validActions();
				mNodeMap[currentPlayer].insert({ infoSet, InfoNode(validActions) });
				mValidActionsMap[actionSet] = validActions;
			}
			std::set<std::string> validActions = mValidActionsMap[actionSet];
			for (auto action : validActions) {
				subgameUpdate(State(state, action), player);
			}
		}
		else {
			subgameUpdate(State(state, frozenAction->second), player);
		}
	}
};

void Pluribus::opponentTurn(std::string action) {
	std::string infoSet = mCurrentState.infoSet();
	std::string actionSet = mCurrentState.actionSet();
	std::set<std::string> validActions = mValidActionsMap[actionSet];
	if (validActions.find(action) == validActions.end()) {
		// add action to all public states that share infoset
		search(10000);
	}
	mCurrentState = State(mCurrentState, action);
};

void Pluribus::pluribusTurn() {
	std::string infoSet = mCurrentState.infoSet();
	std::string actionSet = mCurrentState.actionSet();
	std::set<std::string> validActions = mValidActionsMap[actionSet];
	int player = mCurrentState.mTurn;
	// do we want average strategy or just strategy?
	std::unordered_map<std::string, double> strategy = mNodeMap[player].at(infoSet).getStrategy(validActions);

	std::vector<std::string> actions;
	std::vector<double> probabilities;
	for (auto map : strategy) {
		actions.push_back(map.first);
		probabilities.push_back(map.second);
	}
	std::discrete_distribution<int> random_choice(probabilities.begin(), probabilities.end());
	auto action = random_choice(mActionEng);

	mFrozenNodes[infoSet] = actions[action];
	mCurrentState = State(mCurrentState, actions[action]);
};

