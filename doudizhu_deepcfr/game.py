import random
from doudizhu import DouDiZhu
import numpy as np

doudizhu = DouDiZhu(type='action')
doudizhu.PokerlistToString([])
ACTIONS = doudizhu.get_all_hand(sorted(list(range(5, 15)) * 4 + [16] * 4 + [18, 19]))


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


class ChanceState(object):
    def __init__(self, n_left1, n_left2, player=0, pokers1=[], pokers2=[], public1=[], public2=[]):
        # 以第一个玩家为地主,第二个玩家为农民
        self.n_left1 = n_left1
        self.n_left2 = n_left2
        self.pokers1 = pokers1
        self.pokers2 = pokers2
        self.public1 = public1
        self.public2 = public2
        self.player = player

    def play(self):
        Pokers = []
        for color in ['S', 'C', 'H', 'D']:  # [黑桃, 梅花, 红心, 方块]
            for n in ['05', '06', '07', '08', '09', '10', '11', '12', '13', '14', '16']:
                Pokers.append(n + color)
        Pokers.extend(['18J', '19J'])
        if self.player == 0:
            pokers1 = sorted(random.sample(Pokers, 20))
            public1 = sorted(random.sample(pokers1, 3))
            child = ChanceState(self.n_left1, self.n_left2, 1, pokers1, public1=public1)
            return child
        else:
            for p in self.pokers1:
                Pokers.remove(p)
            pokers2 = sorted(random.sample(Pokers, 17))

            child = PlayerGameState(self.pokers1, pokers2, Pokers, 0, doudizhu.PokerlistToString([]),
                                    self.n_left1, self.n_left2, public1=self.public1)
            return child

    def is_terminal(self):
        return False

    def is_chance_node(self):
        return True


class PlayerGameState(object):
    def __init__(self, own_pokers, enemy_pokers, not_appeared,
                 player, enemy_hand, n_left_own_win, n_left_enemy_win, public1=[], public2=[]):
        """
        :param own_pokers:  自己的手牌
        :param enemy_pokers:  对手的手牌
        :param not_appeared:  对自己来说还没出现过的牌
        :param player:   当前玩家
        :param enemy_hand:   对手方最近的出牌
        :param public1:   自己的明牌
        :param public2:   对手的明牌
        """
        self.own_pokers = own_pokers
        self.enemy_pokers = enemy_pokers
        self.not_appeared = not_appeared
        self.player = player
        self.enemy_hand = enemy_hand
        self.n_left_own_win = n_left_own_win
        self.n_left_enemy_win = n_left_enemy_win
        self.public1 = public1
        self.public2 = public2

        own_pokers_int = [int(p[:-1]) for p in own_pokers]
        self.actions = doudizhu.get_feasible_hand(own_pokers_int, enemy_hand)

    def current_player(self):
        return self.player

    def legal_actions(self):
        return self.actions

    @property
    def info_array(self):
        arr1 = poker_encode(self.own_pokers)
        arr2 = poker_encode(self.not_appeared)
        tmp = self.enemy_hand.split(':')[-1]
        arr3 = poker_encode([int(s) for s in tmp.split('.')] if len(tmp) > 0 else [])

        arr4 = poker_encode(self.public1)
        arr5 = poker_encode(self.public2)

        arr6 = np.zeros(5)
        if self.n_left_own_win > 0:
            arr6[self.n_left_own_win - 1] = 1
        arr7 = np.zeros(5)
        if self.n_left_enemy_win > 0:
            arr7[self.n_left_enemy_win - 1] = 1

        return np.hstack([arr1, arr2, arr3, arr4, arr5, arr6, arr7])

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
                                self.n_left_enemy_win, self.n_left_own_win,
                                public1=self.public2, public2=new_public)
        return child

    def is_terminal(self):
        num_poker = [len(a.split(':')[-1].split('.')) if len(a.split(':')[-1]) else 0
                     for a in self.actions]
        return len(self.own_pokers) - max(num_poker) <= self.n_left_own_win

    def is_chance_node(self):
        return False


if __name__ == '__main__':
    root = ChanceState(0, 1)
    child = root.play().play()
    while(not child.is_terminal()):
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
