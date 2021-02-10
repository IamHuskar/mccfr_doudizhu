#pragma once

#include <vector>

#include <iostream>
#include <fstream>

#include "player.h"
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <random>

using namespace std;
static int NEG_REWARD_WEIGHT = 1;
#define NEG_REWARD -1.f

enum class StateId {
	NORMAL = 0,
	FINISHED = 1
};

class MCState {
public:
	CardGroup _last_group;
	StateId _id;
	vector<Player*> _players;
	int _current_idx, _current_controller, _winner, _target_idx;

	MCState();
	MCState(const MCState &s);
	MCState(const CEnv &env);
	~MCState();

	vector<vector<CardGroup>::iterator> get_action_space() const;
	vector<CardGroup> get_action_space_1() const;
};


class Edge;

class MCNode {
public:
	MCState *st = nullptr;
	vector<vector<CardGroup>::iterator> actions;
	Edge *src = nullptr;
	vector<Edge*> edges;
	std::mutex mu;

	MCNode(Edge *src, MCState*st, vector<float> total_cnts = vector<float>(), vector<float> priors = vector<float>(), bool root = false);

	~MCNode();

	Edge *choose(float c = sqrtf(2.f));
};


class Edge {
public:
	vector<CardGroup>::iterator action;
	int n = 0;
	float w = 0.f;
	float q = 0.f;
	bool terminiated = false;
	std::shared_timed_mutex mu;
	float r = 0.f;
	float p = 0.f;
	MCNode *src = nullptr;
	MCNode *dest = nullptr;

	Edge(MCNode *src, const vector<CardGroup>::iterator &action, float prior);

	~Edge();
};


class MCTree {
public:
	MCNode *root = nullptr;
	int idx = -1;
	int counter = 0;
	float c = 0;
	std::mutex counter_mu;
	vector<float> total_cnts;

	MCTree(MCState*, vector<float> total_cnts = vector<float>(), float c = sqrtf(2.f));

	~MCTree();

	void search(int n_threads, int n);
	void search_thread(mt19937 *generator);
	MCNode *explore(MCNode *node, float &val, mt19937 &generator, int boom_cnt);
	void backup(MCNode *node, float val);
	float rollout(MCNode *node, mt19937 &generator, int boom_cnt);
	vector<int> predict();
	vector<float> eval(float c = sqrtf(2.f));

};

vector<CardGroup> getActionspace(vector<Card> self_cards, const CardGroup &last_cardgroup, int current_idx, int current_controller);
int getActionspaceCount(vector<Card> self_cards, const CardGroup &last_cardgroup, int current_idx, int current_controller);

CardGroup mcsearch_maybecards(vector<Card> self_cards, vector<vector<Card>> maybe_cards, vector<Card> unseen_cards,
	int next_handcards_cnt,
	const CardGroup &last_cardgroup, int current_idx, int current_controller,
	int n_threads, int max_d, int max_iter, int weight, int n_r_weight);


CardGroup mcsearch(vector<Card> self_cards, vector<Card> unseen_cards,
    int next_handcards_cnt,
    const CardGroup &last_cardgroup, int current_idx, int current_controller,
    int n_threads, int max_d, int max_iter, int weight, int n_r_weight);

CardGroup get_action(vector<Card> self_cards, vector<Card> next_player_cards,
	const CardGroup &last_cardgroup, int current_idx, int current_controller,
	int n_threads, int max_iter, int weight, int n_r_weight);

vector<CardGroup> get_topk_actions(vector<Card> self_cards, vector<vector<Card>> maybe_cards, vector<Card> unseen_cards,
	int next_handcards_cnt,
	const CardGroup &last_cardgroup, int current_idx, int current_controller,
	int n_threads, int max_d, int max_iter, int weight, int n_r_weight, int k);

void step_ref(MCState &s, const vector<CardGroup>::iterator &a);
MCState* step(const MCState& s, const vector<CardGroup>::iterator &a);