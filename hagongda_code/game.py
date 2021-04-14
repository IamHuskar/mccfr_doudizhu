import random
from doudizhu import DouDiZhu
import numpy as np

doudizhu = DouDiZhu(type='all')
ACTIONS = doudizhu.get_all_hand(sorted(list(range(5, 15)) * 4 + [16] * 4 + [18, 19]))

class GameState:
    def __init__(self, own_pokers, not_appeared, enemy_hand, player,
                 n_left1, n_left2, public1=[], public2=[]):
        self.own_pokers = own_pokers
        self.not_appeared = not_appeared
        self.enemy_hand = enemy_hand
        self.player = player
        self.n_left1 = n_left1
        self.n_left2 = n_left2
        self.public1 = public1
        self.public2 = public2

    def is_only_one_state(self):
        return False

    def inf_set(self):
        # 自己的牌 + 还没出现过的牌 + 对手最近一次的出牌
        s1 = '.'.join([str(i) for i in sorted(self.own_pokers)])
        s2 = '.'.join([str(i) for i in sorted(self.not_appeared)])
        s3 = self.enemy_hand
        s4 = str(self.n_left1)
        s5 = str(self.n_left2)
        s6 = '.'.join([str(i) for i in self.public1])
        s7 = '.'.join([str(i) for i in self.public2])
        return s1 + '#' + s2 + '#' + s3 + '#' + s4 + '#' + s5 + '#' + s6 + '#' + s7


def poker_encode(pokers):
    if len(pokers) and type(pokers[0]) == str:
        pokers_int = [int(p[:-1]) for p in pokers]
    else:
        pokers_int = pokers
    arr_list = []
    for p in [5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16]:
        arr = np.zeros(4)
        c = pokers_int.count(p)
        if c > 0:
            arr[c - 1] = 1
        arr_list.append(arr)
    arr = np.array([1 if 18 in pokers_int else 0, 1 if 19 in pokers_int else 0])
    arr_list.append(arr)
    return np.hstack(arr_list)


class ChanceState(GameState):
    def __init__(self, own_pokers, not_appeared, enemy_hand,
                 n_left1, n_left2, public1=[], public2=[]):
        super().__init__(own_pokers, not_appeared, enemy_hand, 0,
                         n_left1, n_left2, public1, public2)
        self.enemy_hand = doudizhu.PokerlistToString([int(p[:-1]) for p in enemy_hand])

    def play(self):
        n = len(self.not_appeared) - 9 - len(self.public2)
        enemy_pokers = sorted(random.sample(self.not_appeared, n) + self.public2)
        child = PlayerGameState(self.own_pokers, enemy_pokers, self.not_appeared, 0, self.enemy_hand,
                                self.n_left1, self.n_left2, self.public1, self.public2)
        return child

    def is_terminal(self):
        return False

    def is_chance_node(self):
        return True


class PlayerGameState(GameState):
    def __init__(self, own_pokers, enemy_pokers, not_appeared,
                 player, enemy_hand, n_left1, n_left2, public1=[], public2=[]):
        super().__init__(own_pokers, not_appeared, enemy_hand, player, n_left1, n_left2, public1, public2)
        self.enemy_pokers = enemy_pokers
        own_pokers_int = [int(p[:-1]) for p in own_pokers]
        self.actions = doudizhu.get_feasible_hand(own_pokers_int, enemy_hand)
        self.only_one_action=False #if( len(self.actions) <=1 ) else False
    
    def is_only_one_state(self):
        return self.only_one_action
        
    def current_player(self):
        return self.player

    def legal_actions(self):
        return self.actions

    @property
    def legal_actions_mask(self):
        mask = np.array(
            [1 if a in self.actions else 0 for a in ACTIONS]
        )
        return mask

    def play(self, action):

        if len(action.split(':')[-1]):
            action_pokers = [int(p) for p in action.split(':')[-1].split('.')]
        else:
            action_pokers = []

        new_pokers = self.own_pokers.copy()
        new_pokers_int = [int(p[:-1]) for p in self.own_pokers]
        for p in action_pokers:
            idx = new_pokers_int.index(p)
            new_pokers_int.pop(idx)
            new_pokers.pop(idx)
        new_public = [p for p in self.public1 if p in new_pokers]

        new_not_appeared = self.not_appeared.copy()
        for p in new_pokers:
            if p not in new_public:
                new_not_appeared.append(p)
        for p in self.enemy_pokers:
            if p not in self.public2:
                new_not_appeared.remove(p)
        child = PlayerGameState(self.enemy_pokers, new_pokers,
                                new_not_appeared, (self.player + 1) % 2, action,
                                self.n_left2, self.n_left1,
                                public1=self.public2, public2=new_public)
        return child

    def is_terminal(self):
        return len(self.own_pokers) <= self.n_left1

        #num_poker = [len(a.split(':')[-1].split('.')) if len(a.split(':')[-1]) else 0
        #             for a in self.actions]
        #return len(self.own_pokers) - max(num_poker) <= self.n_left1

    def is_chance_node(self):
        return False


if __name__ == '__main__':

    OwnPokers = ['5D', '6H', '7D', '8C', '9H', '9S', '10C', '10S', '11C', '11H', '12S', '12H', '13H', '14C']
    NotAppeared = ['5C', '5H', '5S', '6S', '6D', '9C', '10H', '10D', '11D', '11S', '12D', '13D', '14D', '16H', '16D',
                   '18J']
    enemy_hand = []
    public = ['12S', '12H', '13H']

    root = ChanceState(OwnPokers, NotAppeared, enemy_hand, 0, 1, public, [])

    child = root.play()
    while (not child.is_terminal()):
        print('own_pokers')
        print(child.own_pokers)
        print(child.public1)
        print('enemy_pokers')
        print(child.enemy_pokers)
        print(child.public2)
        print('Not appear')
        print(sorted(child.not_appeared))
        a = random.choice(child.legal_actions())
        print(a)
        print('---------------------------------------------------------------------------')
        child = child.play(a)
