import random

class OutcomeSamplingMCCFR:
    def __init__(self, root):
        self.root = root
        self.cumulative_regret = {}
        self.cumulative_incr = {}
        self._expl = 0.6
    #每次cfr是每次探索一次。
    def iteration(self):
        self._episode(self.root, 0, 1, 1, 1)
        self._episode(self.root, 1, 1, 1, 1)

    def _add_avstrat(self, info, action, incr):
        if info not in self.cumulative_incr:
            self.cumulative_incr[info] = {}
        if action not in self.cumulative_incr[info]:
            self.cumulative_incr[info][action] = 0
        self.cumulative_incr[info][action] += incr

    def _add_regret(self, info, action, regret):
        if info not in self.cumulative_regret:
            self.cumulative_regret[info] = {}
        if action not in self.cumulative_regret[info]:
            self.cumulative_regret[info][action] = 0
        self.cumulative_regret[info][action] += regret

    def _regret_matching(self, state):
        #状态的信息集和动作。
        info, actions = state.inf_set(), state.actions
        #如果信息集不在 累计遗憾值当中。设置为NULL
        if info not in self.cumulative_regret:
            self.cumulative_regret[info] = {}
        #对于状态的每一个动作。
        for a in actions:
            if a not in self.cumulative_regret[info]:
                self.cumulative_regret[info][a] = 0
        sum_regret = sum([max(v, 0) for v in self.cumulative_regret[info].values()])
        if sum_regret > 0:
            policy = {a: max(self.cumulative_regret[info][a], 0) / sum_regret for a in actions}
        else:
            policy = {a: 1./len(actions) for a in actions}
        print("policy",policy)
        return policy

    def _baseline_corrected_child_value(self, sampled_action, action, child_value, sample_prob):
        baseline = 0
        if action == sampled_action:
            return baseline + (child_value - baseline) / sample_prob
        else:
            return baseline

    def _episode(self, state, update_player, my_reach, opp_reach, sample_reach):
        #如果是终结点。如果状态的玩家是 当前更新的玩家。返回奖励1 否则返回奖励-1 
        if state.is_terminal():
            print("当前是终结节点状态 状态的玩家是 ",state.player," 更新的玩家是 ",update_player)
            return 1 if state.player == update_player else -1
        #如果是机会节点。调用play返回
        if state.is_chance_node():
            print("机会节点 ")
            new_state = state.play()
            #采样以后返回一个新的状态。从新的状态开始探索
            return self._episode(new_state, update_player, my_reach, opp_reach, sample_reach)
        
        #遗憾匹配
        policy = self._regret_matching(state)
        if state.player == update_player:
            sample_policy = {a: self._expl * 1./len(policy) + (1.0 - self._expl) * policy[a]
                             for a in policy}
        else:
            sample_policy = policy
        
        sampled_action = random.choices(list(sample_policy.keys()), list(sample_policy.values()))[0]
        new_state = state.play(sampled_action)
        if state.player == update_player:
            new_my_reach = my_reach * policy[sampled_action]
            new_opp_reach = opp_reach
        else:
            new_my_reach = my_reach
            new_opp_reach = opp_reach * policy[sampled_action]
        new_sample_reach = sample_reach * sample_policy[sampled_action]
        child_value = self._episode(new_state, update_player, new_my_reach,
                                    new_opp_reach, new_sample_reach)
        child_values = {}
        for a in state.actions:
            child_values[a] = self._baseline_corrected_child_value(
                sampled_action, a, child_value, sample_policy[a]
            )
        value_estimate = 0
        for a in state.actions:
            value_estimate += policy[sampled_action] * child_values[a]

        if state.player == update_player:
            cf_value = value_estimate * opp_reach / sample_reach
            for a in state.actions:
                cf_action_value = child_values[a] * opp_reach / sample_reach
                self._add_regret(state.inf_set(), a, cf_action_value - cf_value)
            for a in state.actions:
                increment = my_reach * policy[a] / sample_reach
                self._add_avstrat(state.inf_set(), a, increment)

        return value_estimate
