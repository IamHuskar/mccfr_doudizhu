import random


# 牌型枚举
class COMB_TYPE:
    PASS, SINGLE, PAIR, TRIPLE, TRIPLE_ONE, TRIPLE_TWO, FOURTH_TWO_ONES, FOURTH_TWO_PAIRS, STRIGHT, BOMB = range(10)


# 3-14 分别代表 3-10, J, Q, K, A
# 16, 18, 19 分别代表 2, little_joker, big_joker
# 将 2 与其他牌分开是为了方便计算顺子
# 定义 HAND_PASS 为过牌
little_joker, big_joker = 18, 19
HAND_PASS = {'type': COMB_TYPE.PASS, 'main': 0, 'component': []}


class DouDiZhu:
    def __init__(self, ALLOW_THREE_ONE=True, ALLOW_THREE_TWO=False, ALLOW_FOUR_TWO=True):
        """
        :param ALLOW_THREE_ONE: 是否允许三带一
        :param ALLOW_THREE_TWO: 是否允许三带二
        :param ALLOW_FOUR_TWO: 是否允许四带二
        """
        self.ALLOW_THREE_ONE = ALLOW_THREE_ONE
        self.ALLOW_THREE_TWO = ALLOW_THREE_TWO
        self.ALLOW_FOUR_TWO = ALLOW_FOUR_TWO
        self.Pokers = sorted(list(range(3, 15)) * 4 + [16] * 4 + [18, 19])

    def deal_pokers(self, n):
        # 洗牌后每个玩家发n张牌
        pokers = self.Pokers.copy()
        random.shuffle(pokers)
        pokers1 = sorted([pokers[1 + 2 * i] for i in range(n)])
        pokers2 = sorted([pokers[2 * (i + 1)] for i in range(n)])
        return (pokers1, pokers2)

    def get_not_appeared(self, own_pokers):
        Pokers = self.Pokers.copy()
        for p in own_pokers:
            Pokers.remove(p)
        return Pokers

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

    def _can_beat(self, comb1, comb2):
        # comb1 先出，问后出的 comb2 是否能打过 comb1
        # 1. 同种牌型比较 main 值, main 值大的胜
        # 2. 炸弹大过其他牌型
        # 3. 牌型不同, 后出为负
        if not comb2 or comb2['type'] == COMB_TYPE.PASS:
            return False

        if not comb1 or comb1['type'] == COMB_TYPE.PASS:
            return True

        if comb1['type'] == comb2['type']:
            return comb2['main'] > comb1['main']
        elif comb2['type'] == COMB_TYPE.BOMB:
            return True
        else:
            return False


    def _toString(self, comb):
        s = str(comb['type']) + ':' + str(comb['main']) + ':' + '.'.join([str(i) for i in comb['component']])
        print(s)
        return s

    def _toComb(self, s):
        s_list = s.split(':')
        comb = {
            'type': int(s_list[0]),
            'main': int(s_list[1]),
            'component': [int(i) for i in s_list[2].split('.')] if len(s_list[2]) else []
        }
        return comb

    def HandtoString(self, hand):
        if len(hand) == 0:
            return self._toString(HAND_PASS)
        else:
            l = [self._toComb(s) for s in self.get_all_hands(hand)]
            size = [len(s['component']) for s in l]
            if max(size) == len(hand):
                comb = l[size.index(max(size))]
                return self._toString(comb)
            else:
                print('传入错误参数')


    def get_all_hands(self, pokers):
        # 根据当前手牌，获取此牌所有可能出的牌型
        # 牌型数据结构为 {牌类型，主牌，包含的牌}
        # 同种牌类型可以通过主牌比较大小
        # 为方便比较大小, 将顺子按照不同长度分为不同牌型
        if not pokers:
            return []

        # 过牌
        combs = [HAND_PASS]

        # 获取每个点数的数目
        dic = {}
        for poker in pokers:
            dic[poker] = pokers.count(poker)

        # 王炸
        if little_joker in pokers and big_joker in pokers:
            combs.append({'type': COMB_TYPE.BOMB, 'main': big_joker, 'component': [big_joker, little_joker]})

        # 非顺子, 非王炸
        for poker in dic:
            if dic[poker] >= 1:
                # 单张
                combs.append({'type': COMB_TYPE.SINGLE, 'main': poker, 'component': [poker]})

            if dic[poker] >= 2:
                # 对子
                combs.append({'type': COMB_TYPE.PAIR, 'main': poker, 'component': [poker, poker]})

            if dic[poker] >= 3:
                # 三带零
                combs.append({'type': COMB_TYPE.TRIPLE, 'main': poker, 'component': [poker, poker, poker]})
                for poker2 in dic:
                    if self.ALLOW_THREE_ONE and dic[poker2] >= 1 and poker2 != poker:
                        # 三带一
                        combs.append(
                            {'type': COMB_TYPE.TRIPLE_ONE, 'main': poker, 'component': [poker, poker, poker, poker2]})
                    if self.ALLOW_THREE_TWO and dic[poker2] >= 2 and poker2 != poker:
                        # 三带二
                        combs.append({'type': COMB_TYPE.TRIPLE_TWO, 'main': poker,
                                      'component': [poker, poker, poker, poker2, poker2]})

            if dic[poker] == 4:
                # 炸弹
                combs.append({'type': COMB_TYPE.BOMB, 'main': poker, 'component': [poker, poker, poker, poker]})
                if self.ALLOW_FOUR_TWO:
                    pairs = []
                    ones = []
                    for poker2 in dic:
                        if dic[poker2] == 1:
                            ones.append(poker2)
                        elif dic[poker2] == 2:
                            pairs.append(poker2)

                    # 四带二单
                    for i in range(len(ones)):
                        for j in range(i + 1, len(ones)):
                            combs.append({'type': COMB_TYPE.FOURTH_TWO_ONES, 'main': poker, \
                                          'component': [poker, poker, poker, poker, ones[i], ones[j]]})

                    # 四带二对
                    for i in range(len(pairs)):
                        combs.append({'type': COMB_TYPE.FOURTH_TWO_ONES, 'main': poker, \
                                      'component': [poker, poker, poker, poker, pairs[i], pairs[i]]})
                        for j in range(i + 1, len(pairs)):
                            combs.append({'type': COMB_TYPE.FOURTH_TWO_PAIRS, 'main': poker, \
                                          'component': [poker, poker, poker, poker, pairs[i], pairs[i], pairs[j],
                                                        pairs[j]]})

        # 所有顺子组合
        # 以 COMB_TYPE.STRIGHT * len(straight) 标志顺子牌型, 不同长度的顺子是不同的牌型
        for straight in self._create_straight(list(set(pokers)), 5):
            combs.append({'type': COMB_TYPE.STRIGHT * len(straight), 'main': straight[0], 'component': straight})

        # 返回所有可能的出牌类型
        return [self._toString(comb) for comb in combs]

    def get_feasible_hand(self, my_pokers, enemy_hand=None):
        # 牌局终止的边界条件
        if len(my_pokers) == 0:
            return []

        # 如果上一手为空, 则将上一手赋值为 HAND_PASS
        if enemy_hand is None:
            enemy_hand = HAND_PASS
        else:
            enemy_hand = self._toComb(enemy_hand)

        feasible_combs = []
        for current_hand in self.get_all_hands(my_pokers):
            current_hand = self._toComb(current_hand)
            if self._can_beat(enemy_hand, current_hand) or \
                    (enemy_hand['type'] != COMB_TYPE.PASS and current_hand['type'] == COMB_TYPE.PASS):
                feasible_combs.append(current_hand)

        return [self._toString(comb) for comb in feasible_combs]
