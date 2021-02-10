import random


class GameState:
    def __init__(self, doudizhu, own_pokers, not_appeared, enemy_hand, player):
        self.doudizhu = doudizhu
        self.own_pokers = own_pokers
        self.not_appeared = not_appeared
        self.enemy_hand = enemy_hand
        self.player = player

    def inf_set(self):
        # 自己的牌 + 还没出现过的牌 + 对手最近一次的出牌
        s1 = '.'.join([str(i) for i in self.own_pokers])
        s2 = '.'.join([str(i) for i in self.not_appeared])
        s3 = self.enemy_hand
        return s1 + '#' + s2 + '#' + s3


class RootChanceGameState(GameState):
    def __init__(self, doudizhu, own_pokers, not_appeared, enemy_pokers_size, enemy_hand):
        """
        :param doudizhu:
        :param own_pokers:  自己的手牌
        :param not_appeared:  对自己来说还没出现过的牌
        :param enemy_pokers_size:   对手方现在的牌的张数
        :param enemy_hand:   对手方最近的出牌
        """
        super().__init__(doudizhu, own_pokers, not_appeared, enemy_hand, 0)
        self.enemy_pokers_size = enemy_pokers_size
        self.enemy_hand = enemy_hand
        self.last_chance = None

    def play(self):
        enemy_poker = sorted(random.sample(self.not_appeared, self.enemy_pokers_size))
        child = PlayerGameState(self.doudizhu, self.own_pokers,
                                enemy_poker, self.not_appeared, 0, self.enemy_hand)
        self.last_chance = child
        return child

    def is_terminal(self):
        return False

    def is_chance_node(self):
        return True


class PlayerGameState(GameState):
    def __init__(self, doudizhu, own_pokers, enemy_poker, not_appeared, player, enemy_hand):
        """
        :param doudizhu:
        :param own_pokers:  自己的手牌
        :param enemy_poker:  对手的手牌
        :param not_appeared:  对自己来说还没出现过的牌
        :param player:   当前玩家
        :param enemy_hand:   对手方最近的出牌
        """
        super().__init__(doudizhu, own_pokers, not_appeared, enemy_hand, player)
        self.enemy_poker = enemy_poker
        self.enemy_hand = enemy_hand

        self._child = {}
        self.actions = self.doudizhu.get_feasible_hand(own_pokers, enemy_hand)

    def play(self, action):
        if action in self._child:
            return self._child[action]
        else:
            new_pokers = self.own_pokers.copy()
            if len(action.split(':')[-1]):
                action_pokers = [int(p) for p in action.split(':')[-1].split('.')]
            else:
                action_pokers = []
            for p in action_pokers:
                new_pokers.remove(p)
            new_not_appeared = self.not_appeared.copy()
            for p in new_pokers:
                new_not_appeared.append(p)
            for p in self.enemy_poker:
                new_not_appeared.remove(p)
            child = PlayerGameState(self.doudizhu, self.enemy_poker, new_pokers,
                                    new_not_appeared, (self.player + 1) % 2, action)
            self._child[action] = child
            return self._child[action]

    def is_terminal(self):
        num_poker = [len(a.split(':')[-1].split('.')) if len(a.split(':')[-1]) else 0
                     for a in self.actions]
        return max(num_poker) == len(self.own_pokers)

    def is_chance_node(self):
        return False
