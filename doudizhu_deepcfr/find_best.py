import random
import time
class FindBest:
    def __init__(self, root, max_iter = 100):
        self.root = root
        self.player = root.player
        self.max_iter = max_iter
    def solve(self):
        l = []
        t1 = time.clock()
        iteration = 0
        for i in range(self.max_iter):

            t2 = time.clock()
            if t2-t1>10:
                break

            state = self.root.play()
            max_v, value = self.traversal(state, self.player, 0)
            l.append(value)
            iteration = i

        cumsum_value = {a: 0 for a in l[0].keys()}
        for value in l:
            for a in cumsum_value.keys():
                cumsum_value[a] += value[a]

        return {a: cumsum_value[a]/(iteration+1) for a in cumsum_value.keys()}

    def traversal(self, state, player, d):
        if d >= 5:
            return 0, {}
        if state.is_terminal():
            return 1 if state.current_player() == player else -1 ,{}

        if state.current_player() == player:
            value = {}
            for a in state.legal_actions():
                value[a], _ = self.traversal(state.play(a), player, d + 1)
            return max(value.values()), value
        else:
            a = random.choice(state.legal_actions())
            return self.traversal(state.play(a), player, d + 1)
