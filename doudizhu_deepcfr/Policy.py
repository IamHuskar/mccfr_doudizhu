import random
import os
import numpy as np
import tensorflow as tf
from tensorflow.keras import layers, Sequential
from tensorflow.keras.models import load_model
from doudizhu import DouDiZhu
from find_best import FindBest
from online_game import ChanceState as BestRoot

doudizhu = DouDiZhu()
ACTIONS = doudizhu.get_all_hand(sorted(list(range(5, 15)) * 4 + [16] * 4 + [18, 19]))
doudizhu_all = DouDiZhu(type='all')
ACTIONS_all = doudizhu_all.get_all_hand(sorted(list(range(5, 15)) * 4 + [16] * 4 + [18, 19]))

def Encode(own_pokers, not_appeared, enemy_hand, public1, public2, n_left_own_win, n_left_enemy_win):
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
    arr1 = poker_encode(own_pokers)
    arr2 = poker_encode(not_appeared)
    arr3 = poker_encode(enemy_hand)
    arr4 = poker_encode(public1)
    arr5 = poker_encode(public2)
    arr6 = np.zeros(5)
    if n_left_own_win > 0:
        arr6[n_left_own_win - 1] = 1
    arr7 = np.zeros(5)
    if n_left_enemy_win > 0:
        arr7[n_left_enemy_win - 1] = 1
    return np.hstack([arr1, arr2, arr3, arr4, arr5, arr6, arr7])

PolicyNetWork_list = []
for p in [0, 1]:
    l = []
    for n in [1, 2, 3, 4, 5]:
        model = load_model('strategy_networks/strategy{}-{}.h5'.format(p, n))
        l.append(model)
    PolicyNetWork_list.append(l)

def get_action(OwnPokers, NotAppeared, enemy_hand, public1, public2, nleft1, nleft2, player):
    enemy_hand_int = [int(p[:-1]) for p in enemy_hand]
    OwnPokers_int = [int(p[:-1]) for p in OwnPokers]
    array = Encode(OwnPokers, NotAppeared, enemy_hand, public1, public2, nleft1, nleft2)
    legal_action = doudizhu.get_feasible_hand(OwnPokers_int, doudizhu.PokerlistToString(enemy_hand_int))
    legal_action_all = doudizhu_all.get_feasible_hand(OwnPokers_int, doudizhu.PokerlistToString(enemy_hand_int))
    if len(enemy_hand_int):
        for hand in legal_action_all:
            if hand.startswith('Plane_Wings_STRAIGHT_3') or hand.startswith('Plane_Wings_STRAIGHT_4'):
                return hand

    mask = np.array([[1 if a in legal_action else 0 for a in ACTIONS]])
    if player == 0:
        n_left = nleft2
    else:
        n_left = nleft1
    Root = BestRoot(OwnPokers, NotAppeared, enemy_hand, player, public1, public2, nleft1, nleft2)
    findbest = FindBest(Root)
    eval = findbest.solve()
    actions = [a for a in eval.keys() if eval[a] > 0.95]
    if len(actions):
        return random.choice(actions)

    proba = PolicyNetWork_list[player][n_left - 1].predict([array.reshape(1, -1), (1 - mask) * 1000000])
    proba_dict = {}
    for i in range(len(ACTIONS)):
        if proba[0][i] > 0:
            proba_dict[ACTIONS[i]] = proba[0][i]

    actions = [t[0] for t in proba_dict.items()]
    p = [t[1] for t in proba_dict.items()]

    return random.choices(actions, weights=p)[0]



if __name__ == '__main__':
    OwnPokers = ['6S', '6D', '7C', '7H', '9C', '12H', '12D', '13S', '14H', '18J']
    # 对自己来说未出现过的牌
    NotAppeared = ['6H', '6C', '7D', '8H', '8D', '9H', '9D', '9S', '10S', '10D', '11S', '11D', '11H',
                   '14S', '14D', '14C', '16S', '16H', '19J']
    enemy_hand = ['5S', '5D']
    public = ['6D', '7C', '7H']
    print(get_action(OwnPokers, NotAppeared, enemy_hand, public, [], 0, 1, 0))

    OwnPokers = ['5S', '6C', '7S', '8H', '9C', '12S', '12H', '13C', '14H', '18J']
    # 对自己来说未出现过的牌
    NotAppeared = ['6S', '6H', '7C', '8S', '8D', '9D', '9S', '9H', '10S', '10D', '11C', '11S', '11H', '16S']
    enemy_hand = ['6D']
    public = ['9C', '12S', '12H']
    print(get_action(OwnPokers, NotAppeared, enemy_hand, public, [], 0, 1, 0))

    OwnPokers = ['8D', '9H', '10C', '11S', '12D', '13D', '14H', '18J']
    # 对自己来说未出现过的牌
    NotAppeared = ['6S', '8S', '9C', '9D', '9S', '10H', '10S', '11D', '11C', '11H', '12S', '13H', '14D', '16C']
    enemy_hand = []
    public = ['6C', '7D', '8H']
    print(get_action(OwnPokers, NotAppeared, enemy_hand, [], public, 1, 0, 1))

    OwnPokers = ['8D', '8C', '8H', '11S', '11D', '13D', '14H', '18J']
    # 对自己来说未出现过的牌
    NotAppeared = ['6S', '6C', '7D', '9C', '9D', '9S', '10H', '10S', '11C', '11H', '12S', '13H', '14D', '16C']
    enemy_hand = ['5S']
    public = ['13H', '14D', '16C']
    print(get_action(OwnPokers, NotAppeared, enemy_hand, [], public, 1, 0, 1))

    OwnPokers = ['8D', '9H', '9S', '10C', '11S', '12D', '14H', '14C']
    # 对自己来说未出现过的牌
    NotAppeared = ['6S', '8S', '9C', '9D', '10H', '10S', '11D', '11C', '11H', '12S', '13H', '14D', '16C']
    enemy_hand = ['5D', '5C']
    public = ['6C', '7D', '8H']
    print(get_action(OwnPokers, NotAppeared, enemy_hand, [], public, 1, 0, 1))

    OwnPokers = ['8D', '8S', '8C', '9H', '9S', '9D', '10C', '10S', '14H', '14C', '18J']
    # 对自己来说未出现过的牌
    NotAppeared = ['6S', '9C', '10H', '10D', '11D', '11C', '11H', '12S', '13H', '14D', '16C']
    enemy_hand = []
    public = ['6C', '7D', '19J']
    print(get_action(OwnPokers, NotAppeared, enemy_hand, [], public, 1, 0, 1))

    OwnPokers = ['5D', '6H', '8D', '8S', '8C', '9H', '9S', '9D', '10C', '10S', '14H', '14C']
    # 对自己来说未出现过的牌
    NotAppeared = ['6S', '9C', '10H', '10D', '11D', '11H', '12S', '13H', '14D', '16H', '16D']
    enemy_hand = []
    public = ['6H', '8D', '14C']
    print(get_action(OwnPokers, NotAppeared, enemy_hand, [], public, 0, 1, 0))

    OwnPokers = ['5D', '6H', '8D', '8S', '8C', '9H', '9S', '9D', '10C', '10S', '14H', '14C']
    # 对自己来说未出现过的牌
    NotAppeared = ['5C', '5H', '5S', '6S', '6H', '9C', '10H', '10D', '11D', '11H', '12S', '13H', '14D', '16H', '16D', '18J']
    enemy_hand = []
    public = ['9D', '10C', '10S']
    print(get_action(OwnPokers, NotAppeared, enemy_hand, [], public, 0, 1, 0))