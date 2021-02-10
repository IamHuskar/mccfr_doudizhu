import os
import sys
'''
FILE_PATH = os.path.dirname(os.path.abspath(__file__))
ROOT_PATH = os.path.abspath(os.path.join(FILE_PATH, '..'))
sys.path.append(ROOT_PATH)
sys.path.insert(0, os.path.join(ROOT_PATH, 'build/Release' if os.name == 'nt' else 'build'))

from datetime import datetime
import numpy as np
from card import Card, Category, CardGroup, action_space
from utils import to_char, to_value, get_mask_alter, give_cards_without_minor, \
    get_mask, action_space_single, action_space_pair, get_category_idx, normalize

#from tensorpack import *
from env import Env as CEnv
from mct import mcsearch, mcsearch_maybecards, CCard, CCardGroup, CCategory, get_action, getActionspaceCount
from TensorPack.MA_Hierarchical_Q.env import Env
from TensorPack.MA_Hierarchical_Q.predictor import Predictor
from TensorPack.MA_Hierarchical_Q.DQNModel import Model

from tools import make_sqit_cards

weight_path = os.path.join(ROOT_PATH, 'pretrained_model/model-302500')


# class MCTEnv(Env):
#     def step(self, intention):
#         player, done = super().step(intention)
#         if player != self.agent_names[0]:
#             return 1, done
#         else:
#             return -1, done
#
#     def step_auto(self):
#         def char2ccardgroup(chars):
#             cg = CardGroup.to_cardgroup(chars)
#             ccg = CCardGroup([CCard(to_value(c) - 3) for c in cg.cards], CCategory(cg.type), cg.value, cg.len)
#             return ccg
#
#         def ccardgroup2char(cg):
#             return [to_char(int(c) + 3) for c in cg.cards]
#
#         handcards_char = self.get_curr_handcards()
#         chandcards = [CCard(to_value(c) - 3) for c in handcards_char]
#         player_idx = self.get_current_idx()
#         unseen_cards = self.player_cards[self.agent_names[(player_idx + 1) % 3]] + self.player_cards[self.agent_names[(player_idx + 2) % 3]]
#         cunseen_cards = [CCard(to_value(c) - 3) for c in unseen_cards]
#
#         next_handcards_cnt = len(self.player_cards[self.agent_names[(player_idx + 1) % 3]])
#
#         last_cg = char2ccardgroup(self.get_last_outcards())
#         caction = mcsearch(chandcards, cunseen_cards, next_handcards_cnt, last_cg,
#                            (self.agent_names.index(self.curr_player) - self.agent_names.index(self.lord)  + 3) % 3,
#                            (self.agent_names.index(self.controller) - self.agent_names.index(self.lord) + 3) % 3, 10, 50, 500)
#         intention = ccardgroup2char(caction)
#         return self.step(intention)

class MCTEnv(Env):
    def step(self, intention):
        player, done = super().step(intention)
        if player != self.agent_names[0]:
            return 1, done
        else:
            return -1, done

    def get_mct_maybe_cards(self):
        def char2ccardgroup(chars):
            cg = CardGroup.to_cardgroup(chars)
            ccg = CCardGroup([CCard(to_value(c) - 3) for c in cg.cards], CCategory(cg.type), cg.value, cg.len)
            return ccg

        def ccardgroup2char(cg):
            return [to_char(int(c) + 3) for c in cg.cards]

        prePlayer_id = (self.get_current_idx() + 1) % 2
        true_hand_cards = self.player_cards[self.agent_names[prePlayer_id]]

        prePlayer_cnts = len(self.player_cards[self.agent_names[prePlayer_id]])

        sqilt = make_sqit_cards(prePlayer_cnts)
        prePlayer_lastcards = self.out_cards
        prePlayer_lastcard = prePlayer_lastcards[1]

        prePlayer_outcards = self.get_last_outcards()

        pre_unseen_cards = self.extra_cards + self.player_cards[self.agent_names[prePlayer_id]]

        pre_unseen_cards = sorted(pre_unseen_cards)

        card_sets = sqilt.guess_cards(pre_unseen_cards)
        maybe_card_sets = []
        for card_one in card_sets:
            pre_hands_one = prePlayer_outcards + card_one

            chandcards = [CCard(to_value(c) - 3) for c in pre_hands_one]
            player_idx = (self.get_current_idx() + 1) % 2
            # unseen_cards = self.player_cards[self.agent_names[(player_idx + 1) % 3]] + self.player_cards[self.agent_names[(player_idx + 2) % 3]]
            unseen_cards = self.player_cards[self.agent_names[(player_idx + 1) % 2]] + \
                           self.extra_cards + \
                           self.player_cards[self.agent_names[(player_idx) % 2]]
            for r_card in card_one:
                unseen_cards.remove(r_card)

            cunseen_cards = [CCard(to_value(c) - 3) for c in unseen_cards]
            next_handcards_cnt = len(self.player_cards[self.agent_names[(player_idx + 1) % 2]])
            last_cg = char2ccardgroup(self.mcts_histories[self.agent_names[(prePlayer_id + 1) % 2]][-1])

            max_d = 50
            maybe_cards = []
            cmaybe_cards = []
            for maybe_card in maybe_cards:
                c_cards = [CCard(to_value(c) - 3) for c in maybe_card]
                cmaybe_cards.append(c_cards)
            caction_maybecards = mcsearch_maybecards(chandcards, cmaybe_cards, cunseen_cards, next_handcards_cnt,
                                                     last_cg,
                                                     (self.agent_names.index(
                                                         self.curr_player) - self.agent_names.index(
                                                         self.lord) + 2) % 2,
                                                     (self.agent_names.index(
                                                         self.controller) - self.agent_names.index(
                                                         self.lord) + 2) % 2, 10, max_d, 500, 2, 1)
            intention_maybecards = ccardgroup2char(caction_maybecards)

            card_one_intention = sorted(intention_maybecards)
            prePlayer_outcards = sorted(prePlayer_outcards)
            if card_one_intention == prePlayer_outcards:
                # print('ok')
                maybe_card_sets.append(card_one)

        # print('maybe_card_sets =', len(maybe_card_sets))

        maybe_card_sets_2 = []
        prePlayer_outcards_2 = self.mcts_histories[self.agent_names[prePlayer_id]][-2]
        # prePlayer_lastcard_2 = self.mcts_histories[self.agent_names[(prePlayer_id + 1) % 2]][-2]
        for card_two in maybe_card_sets:
            pre_hands_two = prePlayer_outcards_2 + prePlayer_outcards + card_two

            chandcards = [CCard(to_value(c) - 3) for c in pre_hands_two]
            player_idx = (self.get_current_idx() + 1) % 2
            # unseen_cards = self.player_cards[self.agent_names[(player_idx + 1) % 3]] + self.player_cards[self.agent_names[(player_idx + 2) % 3]]
            # last_cg_2 = char2ccardgroup(self.mcts_histories[self.agent_names[(prePlayer_id + 1) % 2]][-2])
            unseen_cards = self.player_cards[self.agent_names[(player_idx + 1) % 2]] + \
                           self.extra_cards + \
                           self.player_cards[self.agent_names[(player_idx) % 2]] +\
                           self.mcts_histories[self.agent_names[(prePlayer_id + 1) % 2]][-2]
            for r_card in card_two:
                unseen_cards.remove(r_card)

            cunseen_cards = [CCard(to_value(c) - 3) for c in unseen_cards]
            next_handcards_cnt = len(self.player_cards[self.agent_names[(player_idx + 1) % 2]]) + len(self.mcts_histories[self.agent_names[(prePlayer_id + 1) % 2]][-2])
            last_cg = char2ccardgroup(self.mcts_histories[self.agent_names[(prePlayer_id + 1) % 2]][-2])

            max_d = 50
            maybe_cards = []
            cmaybe_cards = []
            for maybe_card in maybe_cards:
                c_cards = [CCard(to_value(c) - 3) for c in maybe_card]
                cmaybe_cards.append(c_cards)
            caction_maybecards = mcsearch_maybecards(chandcards, cmaybe_cards, cunseen_cards, next_handcards_cnt,
                                                     last_cg,
                                                     (self.agent_names.index(
                                                         self.curr_player) - self.agent_names.index(
                                                         self.lord) + 2) % 2,
                                                     (self.agent_names.index(
                                                         self.controller) - self.agent_names.index(
                                                         self.lord) + 2) % 2, 10, max_d, 500, 2, 1)
            intention_maybecards = ccardgroup2char(caction_maybecards)

            prePlayer_outcards = self.mcts_histories[self.agent_names[(prePlayer_id + 1)%2]][-2]

            card_one_intention = sorted(intention_maybecards)
            prePlayer_outcards_2 = sorted(prePlayer_outcards_2)
            if card_one_intention == prePlayer_outcards_2:
                # print('ok')
                maybe_card_sets_2.append(card_two)

        # print('maybe_card_sets_2 =', len(maybe_card_sets_2))

        return maybe_card_sets_2 if len(maybe_card_sets_2) > 0 else maybe_card_sets





    def get_maybe_cards(self):
        # return []

        prePlayer_id = (self.get_current_idx() + 1) % 2
        true_hand_cards = self.player_cards[self.agent_names[prePlayer_id]]
        # print('true_hand_cards=',true_hand_cards)

        prePlayer_cnts = len(self.player_cards[self.agent_names[prePlayer_id]])
        if prePlayer_cnts < 6 and prePlayer_cnts > 2:
            sqilt = make_sqit_cards(prePlayer_cnts)
            prePlayer_lastcards = self.out_cards
            prePlayer_lastcard = prePlayer_lastcards[1]

            prePlayer_outcards = self.get_last_outcards()

            # prePlayer_handcards = self.player_cards[self.agent_names[prePlayer_id]] + prePlayer_outcards
            # prePlayer_intention = self.rhcpAgent.search(prePlayer_handcards,prePlayer_lastcard)

            pre_unseen_cards = self.extra_cards + self.player_cards[self.agent_names[prePlayer_id]]

            pre_unseen_cards = sorted(pre_unseen_cards)

            card_sets = sqilt.guess_cards(pre_unseen_cards)
            maybe_card_sets = []
            for card_one in card_sets:
                pre_hands_one = prePlayer_outcards + card_one
                card_one_intention = self.rhcpAgent.search(pre_hands_one, prePlayer_lastcard)
                card_one_intention = sorted(card_one_intention)
                prePlayer_outcards = sorted(prePlayer_outcards)
                if card_one_intention == prePlayer_outcards:
                    # print('ok')
                    maybe_card_sets.append(card_one)

            # print('maybe_card_sets =', len(maybe_card_sets))

            mct_his_len = len(self.mcts_histories[self.agent_names[(prePlayer_id + 1) % 2]])
            if mct_his_len < 2:
                return maybe_card_sets
            elif len(maybe_card_sets) == 0:
                return maybe_card_sets

            maybe_card_sets_2 = []
            prePlayer_outcards_2 = self.mcts_histories[self.agent_names[prePlayer_id]][-2]
            prePlayer_lastcard_2 = self.mcts_histories[self.agent_names[(prePlayer_id + 1) % 2]][-2]

            for card_two in maybe_card_sets:
                pre_hands_two = prePlayer_outcards_2 + card_two + prePlayer_outcards
                card_two_intention = self.rhcpAgent.search(pre_hands_two, prePlayer_lastcard_2)
                card_two_intention = sorted(card_two_intention)
                prePlayer_outcards_2 = sorted(prePlayer_outcards_2)
                if card_two_intention == prePlayer_outcards_2:
                    # print('ok')
                    maybe_card_sets_2.append(card_two)
            # print('maybe_card_sets_2 =', len(maybe_card_sets_2))
            if mct_his_len < 3:
                return maybe_card_sets_2 if len(maybe_card_sets_2) > 0 else maybe_card_sets
            elif len(maybe_card_sets_2) == 0:
                return maybe_card_sets

            maybe_card_sets_3 = []
            prePlayer_outcards_3 = self.mcts_histories[self.agent_names[prePlayer_id]][-3]
            prePlayer_lastcard_3 = self.mcts_histories[self.agent_names[(prePlayer_id + 1) % 2]][-3]

            for card_there in maybe_card_sets_2:
                pre_hands_three = prePlayer_outcards_3 + prePlayer_outcards_2 + card_there + prePlayer_outcards
                card_three_intention = self.rhcpAgent.search(pre_hands_three, prePlayer_lastcard_3)
                card_three_intention = sorted(card_three_intention)
                prePlayer_outcards_3 = sorted(prePlayer_outcards_3)
                if card_three_intention == prePlayer_outcards_3:
                    # print('ok')
                    maybe_card_sets_3.append(card_there)
            # print('maybe_card_sets_3 =', len(maybe_card_sets_3))

            if mct_his_len < 4:
                return maybe_card_sets_3 if len(maybe_card_sets_3) > 0 else maybe_card_sets_2
            elif len(maybe_card_sets_3) == 0:
                return maybe_card_sets_2

            maybe_card_sets_4 = []
            prePlayer_outcards_4 = self.mcts_histories[self.agent_names[prePlayer_id]][-4]
            prePlayer_lastcard_4 = self.mcts_histories[self.agent_names[(prePlayer_id + 1) % 2]][-4]

            for card_four in maybe_card_sets_3:
                pre_hands_four = prePlayer_outcards_4 + prePlayer_outcards_3 + prePlayer_outcards_2 + card_four + prePlayer_outcards
                card_four_intention = self.rhcpAgent.search(pre_hands_four, prePlayer_lastcard_4)
                card_four_intention = sorted(card_four_intention)
                prePlayer_outcards_4 = sorted(prePlayer_outcards_4)
                if card_four_intention == prePlayer_outcards_4:
                    # print('ok')
                    maybe_card_sets_4.append(card_four)
            # print('maybe_card_sets_4 =', len(maybe_card_sets_4))
            return maybe_card_sets_4
        elif prePlayer_cnts < 3:
            return self.get_mct_maybe_cards()
        return []

    def manual_step_auto(self):
        def char2ccardgroup(chars):
            cg = CardGroup.to_cardgroup(chars)
            ccg = CCardGroup([CCard(to_value(c) - 3) for c in cg.cards], CCategory(cg.type), cg.value, cg.len)
            return ccg

        def ccardgroup2char(cg):
            return [to_char(int(c) + 3) for c in cg.cards]

        handcards_char = self.get_curr_handcards()
        print('农民手牌：', handcards_char)
        manual_input = input("农民出牌:")
        intention_maybecards = list(manual_input)
        return self.step(intention_maybecards)

    def step_auto(self):
        def char2ccardgroup(chars):
            cg = CardGroup.to_cardgroup(chars)
            ccg = CCardGroup([CCard(to_value(c) - 3) for c in cg.cards], CCategory(cg.type), cg.value, cg.len)
            return ccg

        def ccardgroup2char(cg):
            return [to_char(int(c) + 3) for c in cg.cards]



        handcards_char = self.get_curr_handcards()
        print('农民手牌：',handcards_char)
        chandcards = [CCard(to_value(c) - 3) for c in handcards_char]
        player_idx = self.get_current_idx()
        # unseen_cards = self.player_cards[self.agent_names[(player_idx + 1) % 3]] + self.player_cards[self.agent_names[(player_idx + 2) % 3]]


        next_handcards_cnt = len(self.player_cards[self.agent_names[(player_idx + 1) % 2]])

        last_cg = char2ccardgroup(self.get_last_outcards())
        # print('mct current_idx =',(self.agent_names.index(self.curr_player) - self.agent_names.index(self.lord) + 2) % 2)
        # print('mct current_controller =', (self.agent_names.index(self.controller) - self.agent_names.index(self.lord) + 2) % 2)

        # caction = mcsearch(chandcards, cunseen_cards, next_handcards_cnt, last_cg,
        #                    (self.agent_names.index(self.curr_player) - self.agent_names.index(self.lord) + 2) % 2,
        #                    (self.agent_names.index(self.controller) - self.agent_names.index(self.lord) + 2) % 2, 10, 200, 500, 2, 1)
        # intention = ccardgroup2char(caction)

        action_count = getActionspaceCount(chandcards,last_cg,
                             (self.agent_names.index(self.curr_player) - self.agent_names.index(self.lord) + 2) % 2,
                             (self.agent_names.index(self.controller) - self.agent_names.index(self.lord) + 2) % 2)
        if action_count > 1:
            maybe_cards = self.get_maybe_cards()
        else:
            maybe_cards = []

        cmaybe_cards = []
        for maybe_card in maybe_cards:
            c_cards = [CCard(to_value(c) - 3) for c in maybe_card]
            cmaybe_cards.append(c_cards)

        max_d = len(maybe_cards) if len(maybe_cards) > 0 else 100
        unseen_cards = self.player_cards[self.agent_names[(player_idx + 1) % 2]] + self.extra_cards
        if max_d == 100 and next_handcards_cnt < 3 and action_count > 1:
            sqilt = make_sqit_cards(next_handcards_cnt)
            unseen_cards = sqilt.remove_card(unseen_cards, '7')
        cunseen_cards = [CCard(to_value(c) - 3) for c in unseen_cards]


        caction_maybecards = mcsearch_maybecards(chandcards, cmaybe_cards, cunseen_cards, next_handcards_cnt, last_cg,
                           (self.agent_names.index(self.curr_player) - self.agent_names.index(self.lord) + 2) % 2,
                           (self.agent_names.index(self.controller) - self.agent_names.index(self.lord) + 2) % 2, 15, 100, 1000, 2, 1)
        intention_maybecards = ccardgroup2char(caction_maybecards)
        # print('mct give cards：', intention)
        print('农民出牌：', intention_maybecards)
        print('\n')
        return self.step(intention_maybecards)


class RandomEnv(Env):
    def step(self, intention):
        # print(self.get_curr_handcards())
        # print(intention)
        player, done = super().step(intention)
        if player != self.agent_names[0]:
            return 1, done
        else:
            return -1, done

    def step_auto(self):
        mask = get_mask(self.get_curr_handcards(), action_space, self.get_last_outcards())
        intention = np.random.choice(action_space, 1, p=mask / mask.sum())[0]
        return self.step(intention)


class CDQNEnv(Env):
    def __init__(self, weight_path):
        super().__init__()
        agent_names = ['agent%d' % i for i in range(1, 4)]
        # model = Model(agent_names, (1000, 21, 256 + 256 * 2 + 120), 'Double', (1000, 21), 0.99)
        model = Model(agent_names, (1000, 21, 256 + 256  + 60), 'Double', (1000, 21), 0.99)
        self.predictors = {n: Predictor(OfflinePredictor(PredictConfig(
            model=model,
            session_init=SaverRestore(weight_path),
            input_names=[n + '/state', n + '_comb_mask', n + '/fine_mask'],
            output_names=[n + '/Qvalue'])), num_actions=(1000, 21)) for n in self.get_all_agent_names()}

    def step(self, intention):
        # print(intention)
        player, done = super().step(intention)
        if player != self.agent_names[0]:
            return 1, done
        else:
            return -1, done

    def step_auto(self):
        handcards = self.get_curr_handcards()
        last_two_cards = self.get_last_two_cards()
        prob_state = self.get_state_prob()
        intention = self.predictors[self.get_curr_agent_name()].predict(handcards, last_two_cards, prob_state)
        return self.step(intention)

class RHCPEnv(CEnv):
    def __init__(self):
        super().__init__()
        self.agent_names = ['agent1', 'agent2','agent3']

    def prepare(self):
        super().prepare()
        self.lord = self.agent_names[self.get_current_idx()]
        self.controller = self.lord
        # print('lord is ', self.lord, self.get_role_ID())

    @property
    def curr_player(self):
        return self.agent_names[self.get_current_idx()]

    @property
    def player_cards(self):
        other_two = self.get_last_two_handcards()
        curr_idx = self.get_current_idx()
        return {
            self.agent_names[(curr_idx + 2) % 3]: to_char(other_two[1]),
            self.agent_names[(curr_idx + 1) % 3]: to_char(other_two[0]),
            self.agent_names[curr_idx]: self.get_curr_handcards()
        }

    def get_current_idx(self):
        return super().get_curr_ID()

    def get_last_outcards(self):
        return to_char(super().get_last_outcards())

    def get_last_two_cards(self):
        last_two_cards = super().get_last_two_cards()
        last_two_cards = [to_char(c) for c in last_two_cards]
        return last_two_cards

    def get_curr_handcards(self):
        return to_char(super().get_curr_handcards())

    def step(self, intention):
        # print(intention)
        idx = self.get_current_idx()
        print('地主：', self.player_cards[self.agent_names[idx]])
        print('地主:', 'gives', intention, self.controller)
        print('\n')

        r, done = self.step2(to_value(intention))

        return r, done

    def step_auto(self):
        idx = self.get_current_idx()
        # print(idx)
        id = '上家：'
        if idx ==1:
            id= '下家：'
        print(id,self.player_cards[self.agent_names[idx]])
        intention, r = super().step2_auto()
        intention = to_char(intention)
        print(id, 'gives', intention, self.controller)
        print('\n')

        if len(intention) > 0:
            self.controller = self.agent_names[idx]
        assert np.all(self.get_state_prob() >= 0) and np.all(self.get_state_prob() <= 1)
        # print(intention)
        return r, r != 0

def make_env(which):
    if which == 'RHCP':
        return RHCPEnv()
    elif which == 'RANDOM':
        return RandomEnv()
    elif which == 'CDQN':
        return CDQNEnv(weight_path)
    elif which == 'MCT':
        return MCTEnv()
    else:
        raise Exception('env type not supported')
'''