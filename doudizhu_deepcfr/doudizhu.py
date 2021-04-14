import itertools

# 3-14 分别代表 3-10, J, Q, K, A
# 16, 18, 19 分别代表 2, little_joker, big_joker
# 将 2 与其他牌分开是为了方便计算顺子
# 定义 HAND_PASS 为过牌
little_joker, big_joker = 18, 19

class DouDiZhu(object):
    def __init__(self, type = 'action'):
        '''
        :param type: 主要针对飞机带翅膀，all表示考虑全部情况,action表示只考虑二顺的情况
        '''
        self.type = type
        self.max_number = 20

    def get_all_hand(self, pokers):
        hand_list = sorted(set([self._toString(comb) for comb in self._get_all_comb(pokers)]))
        return hand_list

    def PokerlistToString(self, pokers):
        if len(pokers) == 0:
            return self._toString({'type': 'PASS', 'main': 0, 'component': []})
        else:
            l = [comb for comb in self._get_all_comb(pokers)]
            size = [len(s['component']) for s in l]
            if max(size) == len(pokers):
                comb = l[size.index(max(size))]
                return self._toString(comb)
            else:
                print('传入错误参数')

    def get_feasible_hand(self, my_pokers, enemy_hand=None):
        # 牌局终止的边界条件
        if len(my_pokers) == 0:
            return []

        # 如果上一手为空, 则将上一手赋值为 HAND_PASS
        if enemy_hand is None:
            enemy_hand = {'type': 'PASS', 'main': 0, 'component': []}
        else:
            enemy_hand = self._toComb(enemy_hand)

        feasible_combs = []
        for current_hand in self.get_all_hand(my_pokers):
            current_hand = self._toComb(current_hand)
            if self._can_beat(enemy_hand, current_hand) or \
                    (enemy_hand['type'] != 'PASS' and current_hand['type'] == 'PASS'):
                feasible_combs.append(current_hand)

        return [self._toString(comb) for comb in feasible_combs]

    def _can_beat(self, comb1, comb2):
        # comb1 先出，问后出的 comb2 是否能打过 comb1
        # 1. 同种牌型比较 main 值, main 值大的胜
        # 2. 炸弹大过其他牌型
        # 3. 牌型不同, 后出为负
        if not comb2 or comb2['type'] == 'PASS':
            return False
        elif not comb1 or comb1['type'] == 'PASS':
            return True
        elif comb1['type'] == 'ROCKET':
            return False
        elif comb2['type'] == 'ROCKET':
            return True
        elif comb1['type'] == comb2['type']:
            return comb2['main'] > comb1['main']
        elif comb1['type'] == 'BOMB' and comb2['type'] != 'BOMB':
            return False
        elif comb2['type'] == 'BOMB' and comb1['type'] != 'BOMB':
            return True
        else:
            return False

    def _get_all_comb(self, pokers):
        func_list = [f for f in dir(self) if f.startswith('_get') and f.endswith('hand')]
        hand_list = []
        for f in func_list:
            hand_list.extend(list(getattr(self, f)(pokers)))
        hand_list = list(hand_list)
        return hand_list

    def _toString(self, comb):
        s = comb['type'] + ':' + str(comb['main']) + ':' + '.'.join([str(i) for i in comb['component']])
        return s

    def _toComb(self, s):
        s_list = s.split(':')
        comb = {
            'type': s_list[0],
            'main': int(s_list[1]),
            'component': [int(i) for i in s_list[2].split('.')] if len(s_list[2]) else []
        }
        return comb

    def _get_pass_hand(self, pokers):
        yield {'type': 'PASS', 'main': 0, 'component': []}

    def _get_single_hand(self, pokers):
        # 返回单牌
        for p in set(pokers):
            yield {'type': 'SINGLE', 'main': p, 'component': [p]}

    def _get_pair_hand(self, pokers):
        # 返回对牌
        for p in set(pokers):
            if pokers.count(p) >= 2:
                yield {'type': 'PAIR', 'main': p, 'component': [p, p]}

    def _get_triple_hand(self, pokers):
        # 返回三张牌
        for p in set(pokers):
            if pokers.count(p) >= 3:
                yield {'type': 'TRIPLE', 'main': p, 'component': [p, p, p]}

    def _get_triple_one_hand(self, pokers):
        # 返回三带一牌
        for p1 in set(pokers):
            if pokers.count(p1) >= 3:
                for p2 in set(pokers):
                    if p2 != p1:
                        yield {'type': 'TRIPLE_ONE', 'main': p1, 'component': [p1, p1, p1, p2]}

    def _get_triple_two_hand(self, pokers):
        # 返回三带二牌
        for p1 in set(pokers):
            if pokers.count(p1) >= 3:
                for p2 in set(pokers):
                    if p2 != p1 and pokers.count(p2) >= 2:
                        yield {'type': 'TRIPLE_TWO', 'main': p1, 'component': [p1, p1, p1, p2, p2]}

    def _get_bomb_hand(self, pokers):
        # 返回炸弹
        for p in set(pokers):
            if pokers.count(p) == 4:
                yield {'type': 'BOMB', 'main': p, 'component': [p, p, p, p]}

    def _get_fourth_two_ones_hand(self, pokers):
        # 返回四带二
        for p in set(pokers):
            if pokers.count(p) == 4:
                left_pokers = [tmp for tmp in pokers if tmp != p]
                for two_ones in sorted(set(itertools.combinations(left_pokers, 2))):
                    yield {'type': 'FOURTH_TWO_ONES', 'main': p, 'component': [p, p, p, p] + list(two_ones)}

    def _get_fourth_two_pairs_hand(self, pokers):
        # 返回四带二对
        for p in set(pokers):
            if pokers.count(p) == 4:
                left_pokers = [tmp for tmp in pokers if tmp != p]
                left_pairs = set([tmp for tmp in left_pokers if left_pokers.count(tmp)>=2])
                for two_pairs in sorted(set(itertools.combinations(left_pairs, 2))):
                    yield {'type': 'FOURTH_TWO_PAIRS', 'main': p, 'component': [p, p, p, p] + sorted(list(two_pairs)*2)}

    def _create_straight(self, list_of_nums, min_length):
        # 根据列表创建顺子
        a = sorted(list_of_nums)
        lens = len(a)
        for start in range(0, lens):
            for end in range(start, lens):
                if a[end] - a[start] != end - start:
                    break
                elif end - start >= min_length - 1:
                    yield list(range(a[start], a[end] + 1))

    def _get_single_straight_hand(self, pokers):
        # 返回单顺(5张及以上)
        for straight in self._create_straight(list(set(pokers)), 5):
            if len(straight) <= self.max_number:
                yield {'type': 'SINGLE_STRAIGHT_' + str(len(straight)), 'main': straight[0], 'component': straight}

    def _get_double_straight_hand(self, pokers):
        # 返回双顺(三对及以上)
        pair_pokers = []
        for p in set(pokers):
            if pokers.count(p) >= 2:
                pair_pokers.append(p)
        for straight in self._create_straight(pair_pokers, 3):
            if len(straight * 2) <= self.max_number:
                yield {'type': 'DOUBLE_STRAIGHT_' + str(len(straight)), 'main': straight[0],
                       'component': sorted(straight * 2)}

    def _get_triple_straight_hand(self, pokers):
        # 返回三顺(两组及以上)
        triple_pokers = []
        for p in set(pokers):
            if pokers.count(p) >= 3:
                triple_pokers.append(p)
        for straight in self._create_straight(triple_pokers, 2):
            if len(straight * 3) <= self.max_number:
                yield {'type': 'TRIPLE_STRAIGHT_' + str(len(straight)), 'main': straight[0],
                       'component': sorted(straight * 3)}

    def _get_plane_wings_hand(self, pokers):
        # 返回飞机带翅膀
        triple_pokers = []
        for p in set(pokers):
            if pokers.count(p) >= 3:
                triple_pokers.append(p)
        for straight in self._create_straight(triple_pokers, 2):
            pokers_copy = pokers.copy()
            for p in straight * 3:
                pokers_copy.remove(p)
            if self.type == 'action' and len(straight) > 2:
                continue

            single_left_pokers = pokers_copy
            double_left_pokers = [p for p in set(pokers_copy) if pokers_copy.count(p) >= 2]
            for p in set(pokers_copy):
                if pokers_copy.count(p) == 4:
                    double_left_pokers.append(p)

            n = len(straight)
            if n * 4 <= self.max_number:
                for single_pokers in sorted(set(itertools.combinations(single_left_pokers, n))):
                    yield {'type': 'Plane_Wings_STRAIGHT_' + str(len(straight)) + 'SINGLE',
                           'main': straight[0], 'component': sorted(straight * 3) + list(single_pokers)}
            if n * 5 <= self.max_number:
                for double_pokers in sorted(set(itertools.combinations(double_left_pokers, n))):
                    yield {'type': 'Plane_Wings_STRAIGHT_' + str(len(straight)) + 'DOUBLE',
                           'main': straight[0], 'component': sorted(straight * 3) + sorted(list(double_pokers) * 2)}

    def _get_rocket_hand(self, pokers):
        if little_joker in pokers and big_joker in pokers:
            yield {'type': 'ROCKET', 'main': 0, 'component': [little_joker, big_joker]}

if __name__ == '__main__':
    Pokers = sorted(list(range(5, 15)) * 4 + [16] * 4 + [18, 19])
    doudizhu = DouDiZhu(type='action')
    hand_list = doudizhu.get_all_hand(Pokers)
    print(len(hand_list))

    OwnPokers = [14, 14, 14, 14, 18, 19]
    # 对自己来说未出现过的牌
    NotAppeared = [6, 6, 7, 8, 8, 9, 9, 9, 10, 10, 11, 11, 11, 12, 13, 16]
    enemy_hand = [6]

    print(doudizhu.get_all_hand(OwnPokers))
    legal_action = doudizhu.get_feasible_hand(OwnPokers, doudizhu.PokerlistToString(enemy_hand))
    print(legal_action)

