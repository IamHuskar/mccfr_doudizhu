#include "mctree.h"
#include <math.h>
#include <assert.h>
#include <unordered_set>
#include <algorithm>
#include "env.h"
#include "Pluribus.hpp"


vector<mt19937> generators;

MCState::MCState() {

}

MCState::MCState(const MCState &s) {
	_last_group = s._last_group;
	_current_idx = s._current_idx;
	_current_controller = s._current_controller;
	_target_idx = s._target_idx;
	_winner = s._winner;
	_id = s._id;
	for (size_t i = 0; i < s._players.size(); i++)
	{
		_players.push_back(new Player(*s._players[i]));
	}
}

MCState::MCState(const CEnv &env) {
	_last_group = env._last_group;
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
				edge->r = 1.f * boom_cnt * 2;
			}
			else {
				if (sprime->_target_idx + sprime->_winner == 3)
				{
					edge->r = 1.f * boom_cnt * 2;
				}
				else {
					edge->r = NEG_REWARD * NEG_REWARD_WEIGHT * boom_cnt * 2;
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
	while (node->src) {
		auto edge = node->src;
		{
			std::lock_guard<std::mutex> lock(edge->src->mu);
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

		auto cg = *action;
		if (cg._category == Category::BIGBANG) {
			boom_cnt += 1;

		}
		else if (cg._category == Category::QUADRIC) {
			boom_cnt += 1;
		}

	}
	float r = 0;
	// cout << st->winner << endl;
	if (s._winner == s._target_idx)
	{
		r = 1.f * boom_cnt * 2;
	}
	else {
		if (s._target_idx + s._winner == 3)
		{
			r = 1.f * boom_cnt * 2;
		}
		else {
			r = NEG_REWARD * NEG_REWARD_WEIGHT * boom_cnt * 2;
		}
	}
	return r;
}

vector<float> MCTree::eval(float c) {
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

	auto next_idx = (s._current_idx + 1) % s._players.size();
	if (cg._category != Category::EMPTY)
	{


		s._current_controller = s._current_idx;
		s._last_group = cg;


		bool islord = false;
		if (s._current_idx == 0) {
			islord = true;
		}
		if (s._players[s._current_idx]->over(islord, 2)) {
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
		if (sprime->_players[sprime->_current_idx]->over(islord, 2))
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
	
	return s->get_action_space_1();
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

	if (action_space.size() > 1)
	{
		int maybe_cards_size = maybe_cards.size();
		for (size_t d = 0; d < max_d; d++)
		{
			MCState *ss = new MCState(*s);
			if (d < maybe_cards_size)
			{
				vector<Card> maybe_card = maybe_cards[d];
				ss->_players[idx1]->_handcards = maybe_card;
			}
			else {
				shuffle(unseen_cards.begin(), unseen_cards.end(), generator);
				ss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
			}


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

	auto cand = *action_space[max_element(total_cnts.begin(), total_cnts.end()) - total_cnts.begin()];
	delete s;
	return cand;
}

/*
CardGroup mcsearch(vector<Card> self_cards, vector<Card> unseen_cards,
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

	cout << "weight=" << weight << endl;
	cout << "max_d=" << max_d << endl;
	cout << "max_iter=" << max_iter << endl;
	cout << "next_handcards_cnt=" << next_handcards_cnt << endl;
	cout << "unseen_cards=";
	sort(unseen_cards.begin(), unseen_cards.end());
	for (auto card : unseen_cards)
	{
		cout << card << ",";
	}
	cout << endl;

	if (action_space.size() > 1)
	{
		for (size_t d = 0; d < max_d; d++)
		{
			MCState *ss = new MCState(*s);
			shuffle(unseen_cards.begin(), unseen_cards.end(), generator);

			ss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
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
*/
CardGroup mcsearch(vector<Card> self_cards, vector<Card> unseen_cards,
	int next_handcards_cnt,
	const CardGroup &last_cardgroup, int current_idx, int current_controller,
	int n_threads, int max_d, int max_iter, int weight, int n_r_weight) {

	NEG_REWARD_WEIGHT = n_r_weight;

	auto seed = random_device{}();
	auto generator = mt19937(seed);
	Pluribus train(2);

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
			if (d < maybe_cards_size)
			{
				vector<Card> maybe_card = maybe_cards[d];
				ss->_players[idx1]->_handcards = maybe_card;
			}
			else {
				shuffle(unseen_cards.begin(), unseen_cards.end(), generator);
				ss->_players[idx1]->_handcards = vector<Card>(unseen_cards.begin(), unseen_cards.begin() + next_handcards_cnt);
			}


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
	for (int i = 0; i != index.size(); i++) {
		if (i < k) {
			actions.push_back(action_space[index[i]]);
		}
	}
	return actions;

}
