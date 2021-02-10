import os
import sys
FILE_PATH = os.path.dirname(os.path.abspath(__file__))
ROOT_PATH = os.path.abspath(os.path.join(FILE_PATH, '..'))
sys.path.append(ROOT_PATH)
sys.path.insert(0, os.path.join(ROOT_PATH, 'build/Release' if os.name == 'nt' else 'build'))

from datetime import datetime
import numpy as np
from card import Card, Category, CardGroup, action_space
from utils import to_char, to_value, get_mask_alter, give_cards_without_minor, \
    get_mask, action_space_single, action_space_pair, get_category_idx, normalize

from datetime import datetime
#from tensorpack import *
#from env import Env as CEnv
from mct import CEnv,mcsearch, CCard, CCardGroup, CCategory, mcsearch_maybecards, getActionspaceCount
#from TensorPack.MA_Hierarchical_Q.predictor import Predictor
#from TensorPack.MA_Hierarchical_Q.DQNModel import Model
from tools import make_sqit_cards

weight_path = os.path.join(ROOT_PATH, 'pretrained_model/model-302500')


class Agent:
    def __init__(self, role_id):
        self.role_id = role_id

    def intention(self, env):
        pass


class RandomAgent(Agent):
    def intention(self, env):
        mask = get_mask(env.get_curr_handcards(), action_space, env.get_last_outcards())
        intention = np.random.choice(action_space, 1, p=mask / mask.sum())[0]
        return intention


class RHCPAgent(Agent):
    def intention(self, env):
        intention = to_char(CEnv.step_auto_static(Card.char2color(env.get_curr_handcards()), to_value(env.get_last_outcards())))
        # print('rhcp handcards:',env.get_curr_handcards())
        # print('rhcp give cards：', intention)
        return intention

    def search(self,handcards,last_cards):
        intention = to_char(
            CEnv.step_auto_static(Card.char2color(handcards), to_value(last_cards)))
        # print('rhcp handcards:', handcards)
        # print('rhcp give cards：', intention)
        return intention


class CDQNAgent(Agent):
    def __init__(self, role_id, weight_path):
        def role2agent(role):
            if role == 2:
                return 'agent1'
            elif role == 1:
                return 'agent3'
            else:
                return 'agent2'
        super().__init__(role_id)
        agent_names = ['agent%d' % i for i in range(1, 4)]
        # model = Model(agent_names, (1000, 21, 256 + 256 * 2 + 120), 'Double', (1000, 21), 0.99)
        model = Model(agent_names, (1000, 21, 256 + 256 + 60), 'Double', (1000, 21), 0.99)
        self.predictor = Predictor(OfflinePredictor(PredictConfig(
            model=model,
            session_init=SaverRestore(weight_path),
            input_names=[role2agent(role_id) + '/state', role2agent(role_id) + '_comb_mask', role2agent(role_id) + '/fine_mask'],
            output_names=[role2agent(role_id) + '/Qvalue'])), num_actions=(1000, 21))

    def intention(self, env):
        handcards = env.get_curr_handcards()
        # print('地主手牌：',handcards)
        last_two_cards = env.get_last_two_cards()
        prob_state = env.get_state_prob()
        intention = self.predictor.predict(handcards, last_two_cards, prob_state)

        # print('地主出牌：',intention)
        print('\n')
        return intention


class MCTAgent(Agent):
    def get_maybe_cards(self,env):
        # return []

        prePlayer_id = (env.get_current_idx() + 1) % 2
        true_hand_cards = env.player_cards[env.agent_names[prePlayer_id]]
        # print('true_hand_cards=',true_hand_cards)

        prePlayer_cnts = len(env.player_cards[env.agent_names[prePlayer_id]])
        if prePlayer_cnts < 6:
            sqilt = make_sqit_cards(prePlayer_cnts)
            prePlayer_lastcards = env.out_cards
            prePlayer_lastcard = prePlayer_lastcards[0]

            prePlayer_outcards = env.get_last_outcards()

            # prePlayer_handcards = self.player_cards[self.agent_names[prePlayer_id]] + prePlayer_outcards
            # prePlayer_intention = self.rhcpAgent.search(prePlayer_handcards,prePlayer_lastcard)

            pre_unseen_cards = env.extra_cards + env.player_cards[env.agent_names[prePlayer_id]]

            pre_unseen_cards = sorted(pre_unseen_cards)

            card_sets = sqilt.guess_cards(pre_unseen_cards)
            maybe_card_sets = []
            for card_one in card_sets:
                pre_hands_one = prePlayer_outcards + card_one
                card_one_intention = env.rhcpAgent.search(pre_hands_one, prePlayer_lastcard)
                card_one_intention = sorted(card_one_intention)
                prePlayer_outcards = sorted(prePlayer_outcards)
                if sorted(card_one) == sorted(true_hand_cards):
                    pass
                    # print('==')
                if card_one_intention == prePlayer_outcards:
                    # print('==')
                    maybe_card_sets.append(card_one)

            # print('agent_maybe_card_sets =', len(maybe_card_sets))

            mct_his_len = len(env.mcts_histories[env.agent_names[(prePlayer_id + 1) % 2]])
            if mct_his_len < 2:
                return maybe_card_sets
            elif len(maybe_card_sets) == 0:
                return maybe_card_sets

            maybe_card_sets_2 = []
            prePlayer_outcards_2 = env.mcts_histories[env.agent_names[prePlayer_id]][-2]
            prePlayer_lastcard_2 = env.mcts_histories[env.agent_names[(prePlayer_id + 1) % 2]][-2]

            for card_two in maybe_card_sets:
                pre_hands_two = prePlayer_outcards_2 + card_two + prePlayer_outcards
                card_two_intention = env.rhcpAgent.search(pre_hands_two, prePlayer_lastcard_2)
                card_two_intention = sorted(card_two_intention)
                prePlayer_outcards_2 = sorted(prePlayer_outcards_2)
                if sorted(card_two) == sorted(true_hand_cards):
                    pass
                    # print('==')
                if card_two_intention == prePlayer_outcards_2:
                    # print('==')
                    maybe_card_sets_2.append(card_two)
            # print('agent_maybe_card_sets_2 =', len(maybe_card_sets_2))
            if mct_his_len < 3:
                return maybe_card_sets_2 if len(maybe_card_sets_2) >0 else maybe_card_sets
            elif len(maybe_card_sets_2) == 0:
                return maybe_card_sets

            maybe_card_sets_3 = []
            prePlayer_outcards_3 = env.mcts_histories[env.agent_names[prePlayer_id]][-3]
            prePlayer_lastcard_3 = env.mcts_histories[env.agent_names[(prePlayer_id + 1) % 2]][-3]

            for card_there in maybe_card_sets_2:
                pre_hands_three = prePlayer_outcards_3 + prePlayer_outcards_2 + card_there + prePlayer_outcards
                card_three_intention = env.rhcpAgent.search(pre_hands_three, prePlayer_lastcard_3)
                card_three_intention = sorted(card_three_intention)
                prePlayer_outcards_3 = sorted(prePlayer_outcards_3)
                if sorted(card_there) == sorted(true_hand_cards):
                    pass
                    # print('==')
                if card_three_intention == prePlayer_outcards_3:
                    # print('==')
                    maybe_card_sets_3.append(card_there)
            # print('agent_maybe_card_sets_3 =', len(maybe_card_sets_3))

            if mct_his_len < 4:
                return maybe_card_sets_3 if len(maybe_card_sets_3) > 0 else maybe_card_sets_2
            elif len(maybe_card_sets_3) == 0:
                return maybe_card_sets_2

            maybe_card_sets_4 = []
            prePlayer_outcards_4 = env.mcts_histories[env.agent_names[prePlayer_id]][-4]
            prePlayer_lastcard_4 = env.mcts_histories[env.agent_names[(prePlayer_id + 1) % 2]][-4]

            for card_four in maybe_card_sets_3:
                pre_hands_four = prePlayer_outcards_4 + prePlayer_outcards_3 + prePlayer_outcards_2 + card_four + prePlayer_outcards
                card_four_intention = env.rhcpAgent.search(pre_hands_four, prePlayer_lastcard_4)
                card_four_intention = sorted(card_four_intention)
                prePlayer_outcards_4 = sorted(prePlayer_outcards_4)
                if sorted(card_four) == sorted(true_hand_cards):
                    pass
                    # print('==')
                if card_four_intention == prePlayer_outcards_4:
                    # print('==')
                    maybe_card_sets_4.append(card_four)
            # print('agent_maybe_card_sets_4 =', len(maybe_card_sets_4))
            return maybe_card_sets_4 if len(maybe_card_sets_4) > 0 else maybe_card_sets_3
        return []

    def manual_intention(self,env):
        handcards_char = env.get_curr_handcards()
        print('地主手牌：', handcards_char)
        manual_input = input("地主出牌:").replace('10','t')
        manual_input = list(manual_input)
        manual_list = []
        for obj in manual_input:
            if obj == 't':
                manual_list.append('10')
            else:
                manual_list.append(obj)
        return manual_list

    def intention(self, env):
        def char2ccardgroup(chars):
            cg = CardGroup.to_cardgroup(chars)
            ccg = CCardGroup([CCard(to_value(c) - 3) for c in cg.cards], CCategory(cg.type), cg.value, cg.len)
            return ccg

        def ccardgroup2char(cg):
            return [to_char(int(c) + 3) for c in cg.cards]


        handcards_char = env.get_curr_handcards()
        print('地主手牌：', handcards_char)
        chandcards = [CCard(to_value(c) - 3) for c in handcards_char]
        player_idx = env.get_current_idx()
        # unseen_cards = self.player_cards[self.agent_names[(player_idx + 1) % 3]] + self.player_cards[self.agent_names[(player_idx + 2) % 3]]


        next_handcards_cnt = len(env.player_cards[env.agent_names[(player_idx + 1) % 2]])

        last_cg = char2ccardgroup(env.get_last_outcards())

        action_count = getActionspaceCount(chandcards, last_cg,
                                           (env.agent_names.index(env.curr_player) - env.agent_names.index(
                                               env.lord) + 2) % 2,
                                           (env.agent_names.index(env.controller) - env.agent_names.index(
                                               env.lord) + 2) % 2)
        if action_count > 1:
            maybe_cards = self.get_maybe_cards(env)
        else:
            maybe_cards = []

        unseen_cards = env.player_cards[env.agent_names[(player_idx + 1) % 2]] + env.extra_cards

        max_d = len(maybe_cards) if len(maybe_cards) > 0 else 100
        if max_d == 100 and next_handcards_cnt < 3 and action_count > 1:
            sqilt = make_sqit_cards(next_handcards_cnt)
            unseen_cards = sqilt.remove_card(unseen_cards,'7')

        cunseen_cards = [CCard(to_value(c) - 3) for c in unseen_cards]


        cmaybe_cards = []
        for maybe_card in maybe_cards:
            c_cards = [CCard(to_value(c) - 3) for c in maybe_card]
            cmaybe_cards.append(c_cards)


        caction_maybecards = mcsearch_maybecards(chandcards, cmaybe_cards, cunseen_cards, next_handcards_cnt, last_cg,
                                                 (env.agent_names.index(env.curr_player) - env.agent_names.index(
                                                     env.lord) + 2) % 2,
                                                 (env.agent_names.index(env.controller) - env.agent_names.index(
                                                     env.lord) + 2) % 2, 15, 100, 1000, 2, 1)
        intention_maybecards = ccardgroup2char(caction_maybecards)

        # print('mct give cards：', intention)
        print('地主出牌：', intention_maybecards)
        print('\n')

        return intention_maybecards


def make_agent(which, role_id):
    if which == 'RHCP':
        return RHCPAgent(role_id)
    elif which == 'RANDOM':
        return RandomAgent(role_id)
    elif which == 'CDQN':
        return CDQNAgent(role_id, weight_path)
    elif which == 'MCT':
        return MCTAgent(role_id)
    else:
        raise Exception('env type not supported')

