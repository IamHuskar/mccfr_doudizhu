#include "mctree.h"
#include <math.h>
#include <assert.h>
#include <unordered_set>
#include <algorithm>
#include "env.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/filesystem.hpp>
#include "Pluribus.hpp"


vector<mt19937> generators;

MCState::MCState() {
	init_str_card_map();
}

MCState::MCState(const MCState &s) {
	_str_card_map = s._str_card_map;
	_card_str_map = s._card_str_map;

	_last_group = s._last_group;
	_publiccards = s._publiccards;
	_current_idx = s._current_idx;
	_current_controller = s._current_controller;
	_target_idx = s._target_idx;
	_winner = s._winner;
	_id = s._id;
	for (size_t i = 0; i < s._players.size(); i++)
	{
		_players.push_back(new Player(*s._players[i]));
	}
	_publiccards = s._publiccards;
}

MCState::MCState(const CEnv &env) {
	init_str_card_map();
	_last_group = env._last_group;
	_pre_group = env._pre_group;
	_current_idx = env._current_idx;
	_current_controller = env._current_controller;
	_winner = -1;
	_id = StateId::NORMAL;
	_target_idx = _current_idx;
	for (size_t i = 0; i < env._players.size(); i++)
	{
		_players.push_back(new Player(*env._players[i]));
	}
}
std::string MCState::infoSet() {
	int player = _current_idx;
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
void MCState::init_str_card_map() {
	vector<string> initcards({ "3","4","5","6","7","8","9","X","J","Q","K","A","2","*","$" });
	for (int i = 0; i < 15; i++) {
		_str_card_map.insert({ initcards[i],i });
		_card_str_map.insert({ i,initcards[i] });
	}
}

CardGroup MCState::str2group(std::string s) {
	int e_size = s.size();
	vector<Card> cards;
	for (int i = 0; i < e_size; i++) {
		auto search = _str_card_map.find(string(1,s[i]));
		if (search == _str_card_map.end()) {
		}
		else{
			cards.push_back(Card(_str_card_map[string(1, s[i])]));
		}
		
	}
	if (cards.size() == 0) {
		return CardGroup(cards, Category::EMPTY, 0);
	}
	sort(cards.begin(), cards.end());
	return _players[_current_idx]->find_card_group(cards);
}

MCState::~MCState() {
	for (auto player : _players)
	{
		delete player;
	}
}

vector<vector<CardGroup>::iterator> MCState::get_action_space() const {
	return _players[_current_idx]->candidate(_last_group);
}
vector<CardGroup> MCState::get_action_space_1() const {
	return _players[_current_idx]->candidate_1(_last_group);
}


/*
Node::Node(Edge *src, State *st, vector<float> priors) {
this->st = st;
this->actions = st->get_action_space();
//if (this->actions.size() > 300)
//{
//	cout << "large branching factor" << endl;
//}
if (priors.empty() && !this->actions.empty()) {
priors = vector<float>(this->actions.size(), 1.f / this->actions.size());
}
this->src = src;
for (size_t i = 0; i < this->actions.size(); i++) {
this->edges.push_back(new Edge(this, this->actions[i], priors[i]));
}
}
*/

MCNode::MCNode(Edge *src, MCState *st, vector<float> total_cnts, vector<float> priors, bool root) {

	this->st = st;
	this->actions = st->get_action_space();
	if (priors.empty() && !this->actions.empty()) {
		if (root && !total_cnts.empty()){
			float sum = 0.f;
			size_t e_size =total_cnts.size();
			for (size_t i = 0; i <total_cnts.size(); i++) {
				sum +=total_cnts[i];
			}
			priors = vector<float>(this->actions.size(), 1.f / this->actions.size());
			for (size_t i = 0; i <total_cnts.size(); i++) {
				priors[i] = total_cnts[i]/sum;
			}

		}else{
			priors = vector<float>(this->actions.size(), 1.f / this->actions.size());
		}

	}
	
	this->src = src;
	for (size_t i = 0; i < this->actions.size(); i++) {
		this->edges.push_back(new Edge(this, this->actions[i], priors[i]));
	}

	/*
	this->st = st;
	this->actions = st->get_action_space();
	if (priors.empty() && !this->actions.empty()) {
		priors = vector<float>(this->actions.size(), 1.f / this->actions.size());
	}
	this->src = src;
	for (size_t i = 0; i < this->actions.size(); i++) {
		this->edges.push_back(new Edge(this, this->actions[i], priors[i]));
	}
	*/

}


MCNode::~MCNode() {
	if (this->st) {
		delete this->st;
	}
	for (auto e : this->edges) {
		if (e) {
			delete e;
		}
	}
}

Edge::Edge(MCNode *src, const vector<CardGroup>::iterator &action, float prior) {
	this->action = action;
	this->p = prior;
	this->src = src;
}


Edge::~Edge() {
	if (this->dest) {
		delete this->dest;
	}
}


Edge* MCNode::choose(float c) {
	float sum = 0.f;
	size_t e_size = edges.size();
	for (size_t i = 0; i < edges.size(); i++) {
		sum += edges[i]->n;
	}

	float nsum_sqrt = sqrtf(sum);
	int best_idx = -1;
	float best = -100.f;
	for (size_t i = 0; i < e_size; i++) {
		float cand = edges[i]->q + (edges[i]->n == 0 ? 1000.f : c * nsum_sqrt / (edges[i]->n));
		if (cand > best) {
			best_idx = i;
			best = cand;
		}
	}
	return edges[best_idx];
}


MCTree::MCTree(MCState* st, vector<float> total_cnts, float c) {
	this->root = new MCNode(nullptr, st, total_cnts, vector<float>(), true);
	//cout << root->actions.size() << endl;
	this->counter = 0;
	this->c = c;
	this->total_cnts = total_cnts;
}


MCTree::~MCTree() {
	if (root) {
		delete root;
	}
}

void MCTree::search(int n_threads, int n) {
	if (generators.empty()) {
		for (int i = 0; i < n_threads; i++) {
			generators.push_back(mt19937(random_device{}()));
		}
	}
	counter = n;
	vector<thread> threads;
	for (int i = 0; i < n_threads; i++) {

		threads.push_back(std::move(std::thread(&MCTree::search_thread, this, &generators[i])));
	}
	for (auto& t : threads) {
		t.join();
	}
}

void MCTree::search_thread(mt19937 *generator) {
	while (true) {
		{
			std::lock_guard<std::mutex> lock(counter_mu);
			if (counter == 0) {
				break;
			}
			else {
				counter--;
			}
		}
		float val = 0.f;
		// cout << "explore" << endl;

		MCNode* leaf = explore(root, val, *generator, 1);
		// cout << val << endl;
		backup(leaf, val);
	}
}


// TODO: change per node lock to per edge
MCNode* MCTree::explore(MCNode* node, float& val, mt19937 &generator, int boom_cnt) {
	std::unique_lock<std::mutex> lock(node->mu);
	auto edge = node->choose(this->c);
	if (edge->dest) {
		if (edge->terminiated) {
			val = edge->r;
			lock.unlock();
			return edge->dest;
		}
		else {
			lock.unlock();
			return explore(edge->dest, val, generator, boom_cnt);
		}
	}
	else {
		// cout << node->st->idx << ": " << static_cast<int>(node->st->id) << ", ";
		// cout << node->st->get_action_space().size() << endl;
		MCState* sprime = step(*node->st, edge->action);

		auto cg = *edge->action;
		if (cg._category == Category::BIGBANG) {
			boom_cnt += 1;

		}
		else if (cg._category == Category::QUADRIC) {
			boom_cnt += 1;
		}
		edge->dest = new MCNode(edge, sprime, this->total_cnts);

		if (sprime->_id == StateId::FINISHED) {
			if (sprime->_current_idx == 0) {
				if(sprime->_players[1]->_handcards.size() == 17){
					boom_cnt += 1;
				}
			}else{
				if(sprime->_players[0]->_handcards.size() == 20){
					boom_cnt += 1;
				}
			}

			if (sprime->_winner == sprime->_target_idx)
			{
				edge->r = 1.f * boom_cnt;
			}
			else {
				if (sprime->_target_idx + sprime->_winner == 3)
				{
					edge->r = 1.f * boom_cnt;
				}
				else {
					edge->r = NEG_REWARD * NEG_REWARD_WEIGHT * boom_cnt;
				}
			}
			edge->terminiated = true;
			val = edge->r;
			lock.unlock();
			return edge->dest;
		}
		lock.unlock();
		// cout << "rollout ";
		val = rollout(edge->dest, generator, boom_cnt);
		// cout << val << endl;
		// if (val != 0) {
		//     cout << val << ", ";
		// }


		return edge->dest;
	}
}

void MCTree::backup(MCNode* node, float val) {
	int cur_idx = node->st->_current_idx;
	while (node->src) {
		auto edge = node->src;
		{
			std::lock_guard<std::mutex> lock(edge->src->mu);
			int idx = node->st->_current_idx;
			const vector<CardGroup>::iterator &a = edge->action;
			auto cg = *a;
			if (cg._category == Category::BIGBANG) {
				val *= 1;

			}
			else if (cg._category == Category::QUADRIC) {
				val *= 1;
			}
			
			edge->n++;
			edge->w += val;
			edge->q = edge->w / edge->n;
							
		}
		node = edge->src;
	}
}
float MCTree::rollout(MCNode* node, mt19937 &generator, int boom_cnt) {
	auto st = node->st;
	auto s = MCState(*st);
	while (s._id != StateId::FINISHED) {
		auto actions = s.get_action_space();
		auto action = actions[generator() % actions.size()];
		step_ref(s, action);
		/*
		auto cg = *action;
		if (cg._category == Category::BIGBANG) {
			boom_cnt += 1;

		}
		else if (cg._category == Category::QUADRIC) {
			boom_cnt += 1;
		}
		*/
	}
	float r = 0;
	// cout << st->winner << endl;
	if (s._winner == s._target_idx)
	{
		r = 1.f * boom_cnt;
	}
	else {
		if (s._target_idx + s._winner == 3)
		{
			r = 1.f * boom_cnt;
		}
		else {
			r = NEG_REWARD * NEG_REWARD_WEIGHT * boom_cnt;		}
	}
	return r;
}

void MCTree::subgame(MCNode* node) {
	idx = this->root->st->_current_idx;
	//cout << "idx=" << idx << endl;
	if (node) {
		auto edges = node->edges;
		size_t e_size = edges.size();
		for (size_t i = 0; i < e_size; i++) {
			Edge *ed = edges[i];
			if (node->st->_current_idx == idx) {
			//if (1) {
				std::string info = node->st->infoSet();
				//cout << "info=" << info << endl;
				auto cg = *ed->action;
				std::string action = cg.to_str();
				//cout << "action=" << action << endl;
				float q = ed->q;
				//cout << "q=" << q << endl;
				auto search = mNodeMap.find(info);
				if (search == mNodeMap.end()) {
					unordered_map<std::string, float> v;
					v[action] = q;
					mNodeMap.insert({ info, v });

				}
				else {
					std::unordered_map<std::string, float> submNodeMap = mNodeMap.at(info);
					auto subsearch = submNodeMap.find(action);
					if (subsearch == submNodeMap.end()) {
						submNodeMap[action] = q;
						mNodeMap[info] = submNodeMap;
						//mNodeMap.insert({ info, submNodeMap });
					}
					else {
						mNodeMap.at(info).at(action) += q;
					}
				}
			}
			subgame(ed->dest);
		}
	}
	else {
		return;
		cout << "bottom" << endl;
	}
}


vector<float> MCTree::eval(float c,bool save) {
	vector<float> cnts;
	float sum = 0.f;
	size_t e_size = root->edges.size();
	for (size_t i = 0; i < root->edges.size(); i++) {
		sum += root->edges[i]->n;
	}

	float nsum_sqrt = sqrtf(sum);
	int best_idx = -1;
	for (size_t i = 0; i < e_size; i++) {
		float cand = root->edges[i]->q + c * nsum_sqrt / (root->edges[i]->n);
		//cout << root->edges[i]->q << endl;
		//cnts.push_back(cand);
		cnts.push_back(root->edges[i]->q);
	}

	subgame(root);
	return cnts;

}

vector<int> MCTree::predict() {
	vector<int> cnts;
	for (size_t i = 0; i < root->edges.size(); i++) {
		// cout << root->edges[i]->q << ", ";
		cnts.push_back(root->edges[i]->n);
		//cnts.push_back(root->edges[i]->q);
	}
	// cout << endl;
	return cnts;
}

void step_ref(MCState &s, const vector<CardGroup>::iterator &a) {
	auto cg = *a;
	s._players[s._current_idx]->remove_cards(a->_cards);
	s._pre_group = s._last_group;
	auto next_idx = (s._current_idx + 1) % s._players.size();
	if (cg._category != Category::EMPTY)
	{


		s._current_controller = s._current_idx;
		s._last_group = cg;


		bool islord = false;
		if (s._current_idx == 0) {
			islord = true;
		}
		if (s._players[s._current_idx]->over(islord, NEG_REWARD_WEIGHT)) {
			s._winner = s._current_idx;
			s._id = StateId::FINISHED;
			return;
		}

		/*
		if (s._players[s._current_idx]->over())
		{
		s._winner = s._current_idx;
		s._id = StateId::FINISHED;
		return;
		}
		*/

	}
	if (next_idx == s._current_controller)
	{
		s._last_group = CardGroup({}, Category::EMPTY, 0);
	}
	s._current_idx = next_idx;
	return;
}

MCState* step(const MCState& s, const vector<CardGroup>::iterator &a) {
	auto cg = *a;
	MCState *sprime = new MCState(s);
	sprime->_players[sprime->_current_idx]->remove_cards(a->_cards);
	sprime->_pre_group = s._last_group;
	auto next_idx = (sprime->_current_idx + 1) % s._players.size();
	if (cg._category != Category::EMPTY)
	{

		sprime->_current_controller = sprime->_current_idx;
		sprime->_last_group = cg;
		/*
		if (sprime->_players[sprime->_current_idx]->over())
		{
		sprime->_winner = sprime->_current_idx;
		sprime->_id = StateId::FINISHED;
		return sprime;
		}
		*/

		bool islord = false;
		if (sprime->_current_idx == 0) {
			islord = true;
		}
		if (sprime->_players[sprime->_current_idx]->over(islord, NEG_REWARD_WEIGHT))
		{
			sprime->_winner = sprime->_current_idx;
			sprime->_id = StateId::FINISHED;
			return sprime;
		}

	}
	if (next_idx == sprime->_current_controller)
	{
		sprime->_last_group = CardGroup({}, Category::EMPTY, 0);
	}
	sprime->_current_idx = next_idx;
	//cout << "stepped" << endl;
	return sprime;
}



void savePluribus(std::unordered_map<std::string, std::unordered_map<std::string, float>> mNodeMap) {
	//std::ofstream ofs("mcts");
	//boost::archive::text_oarchive oa(ofs);
	//oa << mNodeMap;
	cout << "save" << endl;
}

int getActionspaceCount(vector<Card> self_cards, const CardGroup &last_cardgroup, int current_idx, int current_controller) {
	// set attributes
	MCState *s = new MCState();
	s->_current_idx = current_idx;
	s->_current_controller = current_controller;
	s->_winner = -1;
	s->_id = StateId::NORMAL;
	s->_target_idx = current_idx;
	s->_last_group = last_cardgroup;
	for (size_t i = 0; i < 2; i++)
	{
		s->_players.push_back(new Player());
	}
	s->_players[current_idx]->_handcards = self_cards;
	sort(s->_players[current_idx]->_handcards.begin(), s->_players[current_idx]->_handcards.end());
	s->_players[current_idx]->calc_avail_actions();

	auto action_space = s->get_action_space();
	delete s;
	return action_space.size();
}
vector<CardGroup> getActionspace(vector<Card> self_cards, const CardGroup &last_cardgroup, int current_idx, int current_controller) {
	// set attributes
	MCState *s = new MCState();
	s->_current_idx = current_idx;
	s->_current_controller = current_controller;
	s->_winner = -1;
	s->_id = StateId::NORMAL;
	s->_target_idx = current_idx;
	s->_last_group = last_cardgroup;
	for (size_t i = 0; i < 2; i++)
	{
		s->_players.push_back(new Player());
	}
	s->_players[current_idx]->_handcards = self_cards;
	sort(s->_players[current_idx]->_handcards.begin(), s->_players[current_idx]->_handcards.end());
	s->_players[current_idx]->calc_avail_actions();
	vector<CardGroup> spaces = s->get_action_space_1();
	delete s;
	return spaces;
}
CardGroup mcsearch_god(vector<Card> self_cards, vector<Card> oppcards,
	const CardGroup &last_cardgroup, int current_idx, int current_controller,
	int n_threads, int max_d, int max_iter, int weight, int n_r_weight) {

	NEG_REWARD_WEIGHT = n_r_weight;

	auto seed = random_device{}();
	auto generator = mt19937(seed);

	// set attributes
	MCState *s = new MCState();
	s->_current_idx = current_idx;
	s->_current_controller = current_controller;
	s->_winner = -1;
	s->_id = StateId::NORMAL;
	s->_target_idx = current_idx;
	s->_last_group = last_cardgroup;
	for (size_t i = 0; i < 2; i++)
	{
		s->_players.push_back(new Player());
	}
	s->_players[current_idx]->_handcards = self_cards;
	sort(s->_players[current_idx]->_handcards.begin(), s->_players[current_idx]->_handcards.end());
	s->_players[current_idx]->calc_avail_actions();

	int idx1 = (s->_current_idx + 1) % 2;
	sort(oppcards.begin(), oppcards.end());
	s->_players[idx1]->_handcards = oppcards;
	s->_players[idx1]->calc_avail_actions();

	auto action_space = s->get_action_space();
	vector<float> total_cnts(action_space.size(), 0);

	

	vector<thread> threads;

	if (action_space.size() > 1)
	{
		for (size_t d = 0; d < max_d; d++)
		{
			MCState *ss = new MCState(*s);
			// sequential determinization (parallel MCT)
			MCTree tree(ss, total_cnts, sqrtf(weight));
			tree.search(n_threads, max_iter);
			//vector<int> cnts = tree.predict();
			vector<float> cnts = tree.eval(false);
			for (size_t i = 0; i < action_space.size(); i++)
			{
				total_cnts[i] += cnts[i];
			}
		}
	}
	auto cand = *action_space[max_element(total_cnts.begin(), total_cnts.end()) - total_cnts.begin()];
	delete s;
	return cand;
}

CardGroup mcsearch_maybecards(vector<Card> self_cards, vector<vector<Card>> maybe_cards, vector<Card> unseen_cards,
	int next_handcards_cnt,
	const CardGroup &last_cardgroup, int current_idx, int current_controller,
	int n_threads, int max_d, int max_iter, int weight, int n_r_weight) {

	NEG_REWARD_WEIGHT = n_r_weight;

	auto seed = random_device{}();
	auto generator = mt19937(seed);

	// set attributes
	MCState *s = new MCState();
	s->_current_idx = current_idx;
	s->_current_controller = current_controller;
	s->_winner = -1;
	s->_id = StateId::NORMAL;
	s->_target_idx = current_idx;
	s->_last_group = last_cardgroup;
	for (size_t i = 0; i < 2; i++)
	{
		s->_players.push_back(new Player());
	}
	s->_players[current_idx]->_handcards = self_cards;
	sort(s->_players[current_idx]->_handcards.begin(), s->_players[current_idx]->_handcards.end());
	s->_players[current_idx]->calc_avail_actions();

	auto action_space = s->get_action_space();
	vector<float> total_cnts(action_space.size(), 0);

	int idx1 = (s->_current_idx + 1) % 2;

	vector<thread> threads;

	sort(unseen_cards.begin(), unseen_cards.end());

	std::unordered_map<std::string, std::unordered_map<std::string, float>> mNodeMap;

	if (action_space.size() > 1)
	{
		int maybe_cards_size = maybe_cards.size();
		for (size_t d = 0; d < max_d; d++)
		{
			MCState *ss = new MCState(*s);
			/*
			if (d < maybe_cards_size)
			{
				vector<Card> maybe_card = maybe_cards[d];
				ss->_players[idx1]->_handcards = maybe_card;
			}
			else {
				shuffle(unseen_cards.begin(), unseen_cards.end(), generator);
				ss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
			}
			*/
			shuffle(unseen_cards.begin(), unseen_cards.end(), generator);
			ss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
			ss->_publiccards = vector<Card>(unseen_cards.begin() + next_handcards_cnt, unseen_cards.end());

			sort(ss->_players[idx1]->_handcards.begin(), ss->_players[idx1]->_handcards.end());
			ss->_players[idx1]->calc_avail_actions();

			// sequential determinization (parallel MCT)
			MCTree tree(ss, total_cnts, sqrtf(weight));
			tree.search(n_threads, max_iter);
			//vector<int> cnts = tree.predict();
			vector<float> cnts = tree.eval();
			for (size_t i = 0; i < action_space.size(); i++)
			{
				total_cnts[i] += cnts[i];
			}

			for (auto map : tree.mNodeMap) {
			std:string info = map.first;
				auto search = mNodeMap.find(info);
				auto second = map.second;
				if (search == mNodeMap.end()) {
					mNodeMap.insert(map);
					//cout << "info=" << info << endl;

				}
				else {
					std::unordered_map<std::string, float> submNodeMap = tree.mNodeMap.at(info);
					for (auto map : submNodeMap) {
						string action = map.first;
						auto q = map.second;
						mNodeMap.at(info).at(action) += q;
					}
				}
			}
		}
	}
	savePluribus(mNodeMap);
	auto cand = *action_space[max_element(total_cnts.begin(), total_cnts.end()) - total_cnts.begin()];
	delete s;
	return cand;
}



CardGroup mcsearch(vector<Card> self_cards, vector<Card> unseen_cards,
	int next_handcards_cnt, const CardGroup &_pre_group,
	const CardGroup &last_cardgroup, int current_idx, int current_controller,
	int n_threads_111, int max_d, int max_iter, int weight, int n_r_weight) 
{

	NEG_REWARD_WEIGHT = n_r_weight;

	auto seed = random_device{}();
	auto generator = mt19937(seed);
	Pluribus train(2, n_r_weight);

	State *s = new State();
	s->mTurn = current_idx;
	int idx1 = (s->mTurn + 1) % 2;
	s->_last_group = last_cardgroup;
	s->_pre_group = _pre_group;
	for (size_t i = 0; i < 2; i++)
	{
		s->_players.push_back(new Player());
	}

	s->_players[current_idx]->_handcards = self_cards;
	sort(s->_players[current_idx]->_handcards.begin(), s->_players[current_idx]->_handcards.end());
	s->_players[current_idx]->calc_avail_actions();

	std::unordered_map<std::string, std::unordered_map<std::string, float>> mNodeMap;
	if (boost::filesystem::exists("mcts")) {
		std::ifstream ifs("mcts");
		boost::archive::text_iarchive ia(ifs);
		ia >> mNodeMap;
		std::cout << "loaded mcts";
	}
	set<string> s1;
	for (auto map : mNodeMap) {
		string info = map.first;

		vector<string>tokens;
		std::string delimiters = "|";
		string::size_type lastPos = info.find_first_not_of(delimiters, 0);
		// Find first "non-delimiter".
		string::size_type pos = info.find_first_of(delimiters, lastPos);
		while (string::npos != pos || string::npos != lastPos)
		{
			// Found a token, add it to the vector.
			tokens.push_back(info.substr(lastPos, pos - lastPos));
			// Skip delimiters.  Note the "not_of"
			lastPos = info.find_first_not_of(delimiters, pos);
			// Find next "non-delimiter"
			pos = info.find_first_of(delimiters, lastPos);
		}
		string id = tokens[0];
		string cur_hands = tokens[1];
		string pre = tokens[3];
		string ls = s->_last_group.to_str();
		if (ls.empty()) {
			ls = "EMPTY";
		}
		if (id == to_string(idx1) && ls == pre) {
			s1.insert(cur_hands);
		}

	}

	for (size_t d = 0; d < max_d; d++)
	{

		State *ss = new State(*s);

		shuffle(unseen_cards.begin(), unseen_cards.end(), generator);
		ss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
		sort(ss->_players[idx1]->_handcards.begin(), ss->_players[idx1]->_handcards.end());


		vector<Card> cards = ss->_players[idx1]->_handcards;
		CardGroup g = CardGroup(cards, Category::SINGLE, 1);
		if (cards.size() == 0) {
			g = CardGroup(cards, Category::EMPTY, 1);
		}
		if (s1.size() > 0) {
			if (s1.find(g.to_str()) == s1.end()) {
				delete ss;
				continue;
			}
		}
		ss->_players[idx1]->calc_avail_actions();
		ss->_publiccards = vector<Card>(unseen_cards.begin() + next_handcards_cnt, unseen_cards.end());
		sort(ss->_publiccards.begin(), ss->_publiccards.end());
		train.mCurrentState = *ss;
		auto t1 = std::chrono::high_resolution_clock::now();
		std::cout << max_iter;
		train.train(max_iter);

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
	return cand;
	
}




CardGroup get_action(vector<Card> self_cards, vector<Card> next_player_cards,
	const CardGroup &last_cardgroup, int current_idx, int current_controller,
	int n_threads, int max_iter, int weight, int n_r_weight) {

	NEG_REWARD_WEIGHT = n_r_weight;

	auto seed = random_device{}();
	auto generator = mt19937(seed);

	// set attributes
	MCState *s = new MCState();
	s->_current_idx = current_idx;
	s->_current_controller = current_controller;
	s->_winner = -1;
	s->_id = StateId::NORMAL;
	s->_target_idx = current_idx;
	s->_last_group = last_cardgroup;
	for (size_t i = 0; i < 2; i++)
	{
		s->_players.push_back(new Player());
	}
	s->_players[current_idx]->_handcards = self_cards;
	sort(s->_players[current_idx]->_handcards.begin(), s->_players[current_idx]->_handcards.end());
	s->_players[current_idx]->calc_avail_actions();

	auto action_space = s->get_action_space();
	vector<float> total_cnts(action_space.size(), 0);

	int idx1 = (s->_current_idx + 1) % 2;

	vector<thread> threads;


	if (action_space.size() > 1)
	{
		for (size_t d = 0; d < 1; d++)
		{
			MCState *ss = new MCState(*s);

			ss->_players[idx1]->_handcards = next_player_cards;
			// if (idx1 == 0) {
			// 	ss->_players[idx1]->_handcards.insert(ss->_players[idx1]->_handcards.end(), _env->_extra_cards.begin(), _env->_extra_cards.end());
			// }
			sort(ss->_players[idx1]->_handcards.begin(), ss->_players[idx1]->_handcards.end());
			ss->_players[idx1]->calc_avail_actions();

			// sequential determinization (parallel MCT)
			MCTree tree(ss, total_cnts, sqrtf(weight));
			tree.search(n_threads, max_iter);
			vector<float> cnts = tree.eval();
			for (size_t i = 0; i < action_space.size(); i++)
			{
				total_cnts[i] += cnts[i];
			}
		}
	}

	auto cand = *action_space[max_element(total_cnts.begin(), total_cnts.end()) - total_cnts.begin()];
	delete s;
	return cand;
}

vector<CardGroup> get_topk_actions(vector<Card> self_cards, vector<vector<Card>> maybe_cards, vector<Card> unseen_cards,
	int next_handcards_cnt,
	const CardGroup &last_cardgroup, int current_idx, int current_controller,
	int n_threads, int max_d, int max_iter, int weight, int n_r_weight, int k) {

	NEG_REWARD_WEIGHT = n_r_weight;

	auto seed = random_device{}();
	auto generator = mt19937(seed);

	// set attributes
	MCState *s = new MCState();
	s->_current_idx = current_idx;
	s->_current_controller = current_controller;
	s->_winner = -1;
	s->_id = StateId::NORMAL;
	s->_target_idx = current_idx;
	s->_last_group = last_cardgroup;
	for (size_t i = 0; i < 2; i++)
	{
		s->_players.push_back(new Player());
	}
	s->_players[current_idx]->_handcards = self_cards;
	sort(s->_players[current_idx]->_handcards.begin(), s->_players[current_idx]->_handcards.end());
	s->_players[current_idx]->calc_avail_actions();

	vector<CardGroup> action_space = s->get_action_space_1();
	vector<float> total_cnts(action_space.size(), 0);

	int idx1 = (s->_current_idx + 1) % 2;

	vector<thread> threads;

	sort(unseen_cards.begin(), unseen_cards.end());

	if (action_space.size() > 1)
	{
		int maybe_cards_size = maybe_cards.size();
		for (size_t d = 0; d < max_d; d++)
		{
			MCState *ss = new MCState(*s);
			/*
			if (d < maybe_cards_size)
			{
				vector<Card> maybe_card = maybe_cards[d];
				ss->_players[idx1]->_handcards = maybe_card;
			}
			else {
				shuffle(unseen_cards.begin(), unseen_cards.end(), generator);
				ss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
			}
			*/

			shuffle(unseen_cards.begin(), unseen_cards.end(), generator);
			ss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
			ss->_publiccards = vector<Card>(unseen_cards.begin() + next_handcards_cnt, unseen_cards.end());
			sort(ss->_players[idx1]->_handcards.begin(), ss->_players[idx1]->_handcards.end());
			ss->_players[idx1]->calc_avail_actions();

			// sequential determinization (parallel MCT)
			MCTree tree(ss, total_cnts, sqrtf(weight));
			tree.search(n_threads, max_iter);
			//vector<int> cnts = tree.predict();
			vector<float> cnts = tree.eval();
			for (size_t i = 0; i < action_space.size(); i++)
			{
				total_cnts[i] += cnts[i];
			}
		}
	}


	vector<int> index(total_cnts.size(), 0);
	for (int i = 0; i != index.size(); i++) {
		index[i] = i;
	}
	sort(index.begin(), index.end(),
		[&](const int& a, const int& b) {
		return (total_cnts[a] > total_cnts[b]);
	}
	);
	vector<CardGroup> actions;
	for (int i = 0; i < index.size(); i++) {
		if (i < k) {
			actions.push_back(action_space[index[i]]);
		}
	}
	delete s;
	return actions;

}


CardGroup mcsearch_maybecards_twohandsCards(vector<Card> self_cards, vector<vector<Card>> maybe_cards, vector<Card> unseen_cards,
	int next_handcards_cnt, const CardGroup &pre_pre_cardgroup, const CardGroup &pre_cardgroup,
	const CardGroup &last_cardgroup, int current_idx, int current_controller,
	int n_threads, int max_d, int max_iter, int weight, int n_r_weight) {

	NEG_REWARD_WEIGHT = n_r_weight;

	auto seed = random_device{}();
	auto generator = mt19937(seed);

	std::unordered_map<std::string, std::unordered_map<std::string, float>> mNodeMap;

	// set attributes
	MCState *s = new MCState();
	s->_current_idx = current_idx;
	s->_current_controller = current_controller;
	s->_winner = -1;
	s->_id = StateId::NORMAL;
	s->_target_idx = current_idx;
	s->_last_group = pre_pre_cardgroup;
	for (size_t i = 0; i < 2; i++)
	{
		s->_players.push_back(new Player());
	}
	s->_players[current_idx]->_handcards = self_cards;
	for (auto card : pre_cardgroup._cards) {
		s->_players[current_idx]->add_card(card);
	}
	sort(s->_players[current_idx]->_handcards.begin(), s->_players[current_idx]->_handcards.end());
	s->_players[current_idx]->calc_avail_actions();

	int idx1 = (s->_current_idx + 1) % 2;

	vector<thread> threads;

	sort(unseen_cards.begin(), unseen_cards.end());
	for (size_t d = 0; d < max_d; d++)
	{
		MCState *ss = new MCState(*s);
		shuffle(unseen_cards.begin(), unseen_cards.end(), generator);
		ss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
		ss->_publiccards = vector<Card>(unseen_cards.begin() + next_handcards_cnt, unseen_cards.end());
		for (auto card : last_cardgroup._cards) {
			ss->_players[idx1]->add_card(card);
		}

		sort(ss->_players[idx1]->_handcards.begin(), ss->_players[idx1]->_handcards.end());
		ss->_players[idx1]->calc_avail_actions();

		//cout << "1: " << ss->_players[current_idx]->to_str() << endl;
		//cout << "0: " << ss->_players[idx1]->to_str() << endl;
		//cout << ss->infoSet()<<endl;
		// sequential determinization (parallel MCT)
		vector<float> total_cnts(0, 0);
		auto action_space = ss->get_action_space();

		MCTree tree(ss, total_cnts, sqrtf(weight));
		tree.search(n_threads, max_iter);
		//vector<int> cnts = tree.predict();
		vector<float> cnts = tree.eval();

		CardGroup &cand = *action_space[max_element(cnts.begin(), cnts.end()) - cnts.begin()];
		ss->_pre_group = pre_cardgroup;
		if (cand.to_str() == ss->_pre_group.to_str()) {
			//cout << "cand:" << cand << endl;
			//cout << "lg:" << pre_cardgroup << endl;
		}
		else {
			continue;
		}

		for (auto map : tree.mNodeMap) {
		std:string info = map.first;
			auto search = mNodeMap.find(info);
			auto second = map.second;
			if (search == mNodeMap.end()) {
				mNodeMap.insert(map);
				//cout << "info=" << info << endl;

			}
			else {
				std::unordered_map<std::string, float> submNodeMap = tree.mNodeMap.at(info);
				for (auto map : submNodeMap) {
					string action = map.first;
					auto q = map.second;
					mNodeMap.at(info).at(action) += q;
				}
			}
		}
	}

	savePluribus(mNodeMap);
	//--------------------------
	MCState *sss = new MCState();
	sss->_current_idx = current_idx;
	sss->_current_controller = current_controller;
	sss->_winner = -1;
	sss->_id = StateId::NORMAL;
	sss->_target_idx = current_idx;
	sss->_last_group = last_cardgroup;
	sss->_pre_group = pre_cardgroup;
	for (size_t i = 0; i < 2; i++)
	{
		sss->_players.push_back(new Player());
	}
	sss->_players[current_idx]->_handcards = self_cards;
	sss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
	sort(sss->_players[current_idx]->_handcards.begin(), sss->_players[current_idx]->_handcards.end());
	sss->_publiccards = vector<Card>(unseen_cards.begin() + next_handcards_cnt, unseen_cards.end());

	string inf = sss->infoSet();
	cout << inf << endl;
	float maxq = -100000;
	string taction;
	int q0 = 0;
	auto search = mNodeMap.find(inf);
	if (search == mNodeMap.end()) {
		cout << "not found" << endl;
	}
	else {
		for (auto map : mNodeMap.at(inf)) {
			string subaction = map.first;
			float q = map.second;
			if (q > maxq) {
				maxq = q;
				taction = subaction;
			}
		}
	}
	CardGroup cand = s->str2group(taction);
	delete s;
	delete sss;
	return cand;
}
bool useable_cache(vector<Card> self_cards, vector<Card> unseen_cards,int next_handcards_cnt, const CardGroup &_pre_group,const CardGroup &last_cardgroup, int current_idx, int current_controller)
{
	if (boost::filesystem::exists("mcts")) {

		int idx1 = (current_idx + 1) % 2;
		MCState *sss = new MCState();
		sss->_current_idx = current_idx;
		sss->_current_controller = current_controller;
		sss->_winner = -1;
		sss->_id = StateId::NORMAL;
		sss->_target_idx = current_idx;
		sss->_last_group = last_cardgroup;
		sss->_pre_group = _pre_group;
		for (size_t i = 0; i < 2; i++)
		{
			sss->_players.push_back(new Player());
		}
		sss->_players[current_idx]->_handcards = self_cards;
		sss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
		sort(sss->_players[current_idx]->_handcards.begin(), sss->_players[current_idx]->_handcards.end());
		sss->_publiccards = vector<Card>(unseen_cards.begin() + next_handcards_cnt, unseen_cards.end());

		string inf = sss->infoSet();

		std::unordered_map<std::string, std::unordered_map<std::string, float>> mNodeMap;
		
		std::ifstream ifs("mcts");
		boost::archive::text_iarchive ia(ifs);
		ia >> mNodeMap;
		std::cout << "loaded mcts";

		auto search = mNodeMap.find(inf);
		if (search == mNodeMap.end()) {
			delete sss;
			return false;
		}
		delete sss;
		return true;
	}
	return false;
}
CardGroup search_cache(vector<Card> self_cards, vector<Card> unseen_cards,
	int next_handcards_cnt, const CardGroup &_pre_group,
	const CardGroup &last_cardgroup, int current_idx, int current_controller) {
	MCState *sss = new MCState();
	string taction;
	if (boost::filesystem::exists("mcts")) {

		int idx1 = (current_idx + 1) % 2;
		
		sss->_current_idx = current_idx;
		sss->_current_controller = current_controller;
		sss->_winner = -1;
		sss->_id = StateId::NORMAL;
		sss->_target_idx = current_idx;
		sss->_last_group = last_cardgroup;
		sss->_pre_group = _pre_group;
		for (size_t i = 0; i < 2; i++)
		{
			sss->_players.push_back(new Player());
		}
		sss->_players[current_idx]->_handcards = self_cards;
		sss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
		sort(sss->_players[current_idx]->_handcards.begin(), sss->_players[current_idx]->_handcards.end());
		sss->_publiccards = vector<Card>(unseen_cards.begin() + next_handcards_cnt, unseen_cards.end());
		sss->_players[current_idx]->calc_avail_actions();

		string inf = sss->infoSet();
		cout << "inf =" << inf << endl;

		std::unordered_map<std::string, std::unordered_map<std::string, float>> mNodeMap;

		std::ifstream ifs("mcts");
		boost::archive::text_iarchive ia(ifs);
		ia >> mNodeMap;
		std::cout << "loaded mcts";

		float maxq = -100000;
		
		int q0 = 0;
		for (auto map : mNodeMap.at(inf)) {
			string subaction = map.first;
			float q = map.second;
			if (q > maxq) {
				maxq = q;
				taction = subaction;
			}
		}
	}
	cout << "taction =" << taction << endl;
	CardGroup cand = sss->str2group(taction);
	delete sss;
	return cand;

}
