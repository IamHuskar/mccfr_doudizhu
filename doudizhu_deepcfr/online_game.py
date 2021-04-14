import random
from doudizhu import DouDiZhu
import numpy as np

doudizhu = DouDiZhu(type='action')
doudizhu.PokerlistToString([])
ACTIONS = doudizhu.get_all_hand(sorted(list(range(5, 15)) * 4 + [16] * 4 + [18, 19]))

doudizhu_all = DouDiZhu(type='all')
doudizhu_all.PokerlistToString([])
ACTIONS_all = doudizhu_all.get_all_hand(sorted(list(range(5, 15)) * 4 + [16] * 4 + [18, 19]))


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
    def __init__(self, own_pokers, not_appeared, enemy_hand, player,
                 public1, public2, nleft1, nleft2):
        self.own_pokers = own_pokers
        self.not_appeared = not_appeared
        self.enemy_hand = enemy_hand
        self.player = player
        self.public1 = public1
        self.public2 = public2
        self.nleft1 = nleft1
        self.nleft2 = nleft2

    def current_player(self):
        return self.player

    def legal_actions(self):
        return self.actions

    def play(self):
        n_enemy_pokers = len(self.not_appeared) - 9
        enemy_pokers = sorted(random.sample(self.not_appeared, n_enemy_pokers))
        enemy_pokers += self.public2
        child = GameState(self.own_pokers, enemy_pokers, self.not_appeared, self.player, self.enemy_hand, self.nleft1,
                          self.nleft2, self.public1, self.public2)
        return child

    def is_terminal(self):
        return False

    def is_chance_node(self):
        return True


class GameState(object):
    def __init__(self, own_pokers, enemy_pokers, not_appeared,
                 player, enemy_hand, n_left_own_win, n_left_enemy_win, public1=[], public2=[]):
        self.own_pokers = sorted(own_pokers)
        self.enemy_pokers = sorted(enemy_pokers)
        self.not_appeared = sorted(not_appeared)
        self.player = player
        self.enemy_hand = enemy_hand
        self.n_left_own_win = n_left_own_win
        self.n_left_enemy_win = n_left_enemy_win
        self.public1 = public1
        self.public2 = public2

        own_pokers_int = [int(p[:-1]) for p in own_pokers]
        enemy_hand_int = [int(p[:-1]) for p in enemy_hand]
        self.actions = doudizhu_all.get_feasible_hand(own_pokers_int,
                                                  doudizhu_all.PokerlistToString(enemy_hand_int))

    def play(self, action):
        if len(action.split(':')[-1]):
            action_pokers = [int(p) for p in action.split(':')[-1].split('.')]
        else:
            action_pokers = []

        new_pokers = self.own_pokers.copy()
        new_pokers_int = [int(p[:-1]) for p in self.own_pokers]
        new_hand = []
        for p in action_pokers:
            idx = new_pokers_int.index(p)
            new_pokers_int.pop(idx)
            t = new_pokers.pop(idx)
            new_hand.append(t)
        new_public = [p for p in self.public1 if p in new_pokers]

        new_not_appeared = self.not_appeared.copy()
        for p in new_pokers:
            if p not in new_public:
                new_not_appeared.append(p)
        for p in self.enemy_pokers:
            if p not in self.public2:
                new_not_appeared.remove(p)

        child = GameState(self.enemy_pokers, new_pokers,
                          new_not_appeared, (self.player + 1) % 2, new_hand,
                          self.n_left_enemy_win, self.n_left_own_win,
                          public1=self.public2, public2=new_public)
        return child

    @property
    def legal_actions_mask(self):
        mask = np.array(
            [1 if a in self.actions else 0 for a in ACTIONS]
        )
        return mask

    def current_player(self):
        return self.player

    def legal_actions(self):
        return self.actions

    @property
    def info_array(self):
        arr1 = poker_encode(self.own_pokers)
        arr2 = poker_encode(self.not_appeared)
        arr3 = poker_encode(enemy_hand)

        arr4 = poker_encode(self.public1)
        arr5 = poker_encode(self.public2)

        arr6 = np.zeros(5)
        if self.n_left_own_win > 0:
            arr6[self.n_left_own_win - 1] = 1
        arr7 = np.zeros(5)
        if self.n_left_enemy_win > 0:
            arr7[self.n_left_enemy_win - 1] = 1

        return np.hstack([arr1, arr2, arr3, arr4, arr5, arr6, arr7])

    def is_terminal(self):
        num_poker = [len(a.split(':')[-1].split('.')) if len(a.split(':')[-1]) else 0
                     for a in self.actions]
        return len(self.own_pokers) - max(num_poker) <= self.n_left_own_win

    def is_chance_node(self):
        return False


if __name__ == '__main__':


    OwnPokers = ['6S', '6D', '7C', '7H', '9C', '12H', '12D', '13S', '14H', '18J']
    # 对自己来说未出现过的牌
    NotAppeared = ['6H', '6C', '7D', '8H', '8D', '9H', '9D', '9S', '10S', '10D', '11S', '11D', '11H',
                   '14S', '14D', '14C', '16S', '16H', '19J']
    enemy_hand = ['5S', '5D']
    public1 = ['6D', '7C', '7H']
    # 我是地主
    root = ChanceState(OwnPokers, NotAppeared, enemy_hand, 0, public1, [], 0, 1)
    child = root.play()
    print(child.actions)
    print(child.player)

    OwnPokers = ['6S', '6D', '7C', '7H', '9C', '12H', '12D', '13S', '14H', '18J']
    # 对自己来说未出现过的牌
    NotAppeared = ['6H', '6C', '9H', '9D', '9S', '10S', '10D', '11S', '11D', '11H',
                   '14S', '14D', '14C', '16S', '16H', '19J']
    enemy_hand = ['5S', '5D']
    public2 = ['7D', '8H', '8D']
    # 我是农民
    root = ChanceState(OwnPokers, NotAppeared, enemy_hand, 1, [], public2, 1, 0)
    child = root.play()
    print(child.actions)
    print(child.player)

    OwnPokers = ['8D', '8C', '8H', '11S', '11D', '13D', '14H', '18J']
    # ______________________
    NotAppeared = ['6S', '6C', '7D', '9C', '9D', '9S', '10H', '10S', '11C', '11H', '12S', '13H', '14D', '16C', '19J']
    enemy_hand = ['5S']
    public2 = ['13H', '14D']
    root = ChanceState(OwnPokers, NotAppeared, enemy_hand, 1, [], public2, 1, 0)
