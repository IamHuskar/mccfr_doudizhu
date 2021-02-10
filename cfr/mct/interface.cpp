#include <iostream>
#include <memory>
#include <algorithm>
#include <random>
#include "env.h"
#include "card.h"
#include "mctree.h"
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#pragma warning(disable:4996)
namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(mct, m) {
    m.def("mcsearch", &mcsearch, py::arg("self_cards"), py::arg("unseen_cards"), py::arg("next_handcards_cnt"), py::arg("_pre_group"),py::arg("last_cardgroup"),
        py::arg("current_idx"), py::arg("current_controller"), py::arg("n_threads") = 10, py::arg("max_d") = 50, py::arg("max_iter") = 250, py::arg("weight") = 2, py::arg("NEG_REWARD_WEIGHT") = 1);

    m.def("get_topk_actions", &get_topk_actions, py::arg("self_cards"), py::arg("maybe_cards"),py::arg("unseen_cards"), py::arg("next_handcards_cnt"), py::arg("last_cardgroup"), py::arg("current_idx"), py::arg("current_controller"), py::arg("n_threads") = 10, py::arg("max_d") = 50, py::arg("max_iter") = 250, py::arg("weight") = 2, py::arg("NEG_REWARD_WEIGHT") = 1,py::arg("k") = 4);
	m.def("mcsearch_maybecards", &mcsearch_maybecards, py::arg("self_cards"), py::arg("maybe_cards"), py::arg("unseen_cards"), py::arg("next_handcards_cnt"), py::arg("last_cardgroup"), py::arg("current_idx"), py::arg("current_controller"), py::arg("n_threads") = 10, py::arg("max_d") = 50, py::arg("max_iter") = 250, py::arg("weight") = 2, py::arg("NEG_REWARD_WEIGHT") = 1);
	m.def("get_action", &get_action, py::arg("self_cards"), py::arg("next_player_cards"), py::arg("last_cardgroup"),
		py::arg("current_idx"), py::arg("current_controller"), py::arg("n_threads") = 10, py::arg("max_iter") = 250, py::arg("weight") = 2, py::arg("NEG_REWARD_WEIGHT") = 1);

	m.def("mcsearch_god", &mcsearch_god, py::arg("self_cards"), py::arg("oppcards"), py::arg("last_cardgroup"), py::arg("current_idx"), py::arg("current_controller"), py::arg("n_threads") = 10, py::arg("max_d") = 50, py::arg("max_iter") = 250, py::arg("weight") = 2, py::arg("NEG_REWARD_WEIGHT") = 1);


	m.def("getActionspaceCount", &getActionspaceCount, py::arg("self_cards"), py::arg("last_cardgroup"),py::arg("current_idx"), py::arg("current_controller"));
	m.def("getActionspace", &getActionspace, py::arg("self_cards"), py::arg("last_cardgroup"),py::arg("current_idx"), py::arg("current_controller"));
	m.def("mcsearch_maybecards_twohandsCards", &mcsearch_maybecards_twohandsCards, py::arg("self_cards"),
		py::arg("maybe_cards"), py::arg("unseen_cards"), py::arg("next_handcards_cnt"),
		py::arg("pre_pre_cardgroup"), py::arg("pre_cardgroup"), py::arg("last_cardgroup"),
		py::arg("current_idx"), py::arg("current_controller"), py::arg("n_threads") = 10, py::arg("max_d") = 50,
		py::arg("max_iter") = 250, py::arg("weight") = 2, py::arg("NEG_REWARD_WEIGHT") = 1);
	m.def("useable_cache", &useable_cache, py::arg("self_cards"), py::arg("unseen_cards"), py::arg("next_handcards_cnt"), py::arg("_pre_group"), py::arg("last_cardgroup"),
		py::arg("current_idx"), py::arg("current_controller"));
	m.def("search_cache", &search_cache, py::arg("self_cards"), py::arg("unseen_cards"), py::arg("next_handcards_cnt"), py::arg("_pre_group"), py::arg("last_cardgroup"),
		py::arg("current_idx"), py::arg("current_controller"));
    py::class_<CardGroup>(m, "CCardGroup")
        .def(py::init<>())
        .def(py::init<const vector<Card>&, Category, int, int>(), py::arg("Card"), py::arg("Category"), py::arg("rank"), py::arg("len") = 1)
        .def_readwrite("cards", &CardGroup::_cards);
        
    py::class_<CEnv>(m, "CEnv")
        .def(py::init<>())
        .def("reset", &CEnv::reset)
        .def("step_auto", &CEnv::step_auto);

    py::enum_<Category>(m, "CCategory")
        .value("EMPTY", Category::EMPTY)
        .value("SINGLE", Category::SINGLE)
        .value("DOUBLE", Category::DOUBLE)
        .value("TRIPLE", Category::TRIPLE)
        .value("QUADRIC", Category::QUADRIC)
        .value("THREE_ONE", Category::THREE_ONE)
        .value("THREE_TWO", Category::THREE_TWO)
        .value("SINGLE_LINE", Category::SINGLE_LINE)
        .value("DOUBLE_LINE", Category::DOUBLE_LINE)
        .value("TRIPLE_LINE", Category::TRIPLE_LINE)
        .value("THREE_ONE_LINE", Category::THREE_ONE_LINE)
        .value("THREE_TWO_LINE", Category::THREE_TWO_LINE)
        .value("BIGBANG", Category::BIGBANG)
        .value("FOUR_TAKE_ONE", Category::FOUR_TAKE_ONE)
        .value("FOUR_TAKE_TWO", Category::FOUR_TAKE_ONE)
        .export_values();

    py::enum_<Card>(m, "CCard")
        .value("THREE", Card::THREE)
        .value("FOUR", Card::FOUR)
        .value("FIVE", Card::FIVE)
        .value("SIX", Card::SIX)
        .value("SEVEN", Card::SEVEN)
        .value("EIGHT", Card::EIGHT)
        .value("NINE", Card::NINE)
        .value("TEN", Card::TEN)
        .value("JACK", Card::JACK)
        .value("QUEEN", Card::QUEEN)
        .value("KING", Card::KING)
        .value("ACE", Card::ACE)
        .value("TWO", Card::TWO)
        .value("BLACK_JOKER", Card::BLACK_JOKER)
        .value("RED_JOKER", Card::RED_JOKER)
        .export_values();

}
