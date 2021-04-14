import random
import os
import numpy as np
import tensorflow as tf
from tensorflow.keras.models import Model, load_model
from tensorflow.keras.layers import Input, Dense, Subtract, Multiply
from tensorflow.keras.layers import LayerNormalization, LeakyReLU, Softmax
from doudizhu import DouDiZhu

doudizhu = DouDiZhu(type='action')
ACTIONS = doudizhu.get_all_hand(sorted(list(range(5, 15)) * 4 + [16] * 4 + [18, 19]))


class ReservoirBuffer(object):
    def __init__(self, reservoir_buffer_capacity):
        self._reservoir_buffer_capacity = reservoir_buffer_capacity
        self._data = []
        self._add_calls = 0

    def add(self, element):
        if len(self._data) < self._reservoir_buffer_capacity:
            self._data.append(element)
        else:
            idx = np.random.randint(0, self._add_calls + 1)
            if idx < self._reservoir_buffer_capacity:
                self._data[idx] = element
        self._add_calls += 1

    def sample(self, num_samples):
        if len(self._data) < num_samples:
            raise ValueError('{} elements could not be sampled from size {}'.format(
                num_samples, len(self._data)))
        return random.sample(self._data, num_samples)

    def clear(self):
        self._data = []
        self._add_calls = 0

    def __len__(self):
        return len(self._data)

    def __iter__(self):
        return iter(self._data)

    @property
    def data(self):
        return np.vstack(self._data)

    def shuffle_data(self):
        random.shuffle(self._data)


def AdvantageNetwork(input_size, policy_network_layers, num_actions):
    I = Input(shape=(input_size,))
    M = Input(shape=(num_actions,))
    l = I
    for n in policy_network_layers:
        l = Dense(n, kernel_initializer='he_normal')(l)
        l = LeakyReLU(alpha=0.05)(l)
    l = LayerNormalization()(l)
    l = Dense(num_actions)(l)
    O = Multiply()([l, M])
    O = LeakyReLU(alpha=0.05)(O)
    network = Model(inputs=[I, M], outputs=O)
    network.compile(optimizer='adam',
                    loss=tf.keras.losses.MAE
                    )
    return network


def PolicyNetwork(input_size, adv_network_layers, num_actions):
    I = Input(shape=(input_size,))
    D = Input(shape=(num_actions,))
    l = I
    for n in adv_network_layers:
        l = Dense(n, kernel_initializer='he_normal')(l)
        l = LeakyReLU(alpha=0.05)(l)
    l = LayerNormalization()(l)
    l = Dense(num_actions)(l)
    l = Subtract()([l, D])
    O = Softmax()(l)
    network = Model(inputs=[I, D], outputs=O)
    network.compile(optimizer='adam',
                    loss=tf.keras.losses.MeanSquaredError()
                    )
    return network


# class LossHistory(tf.keras.callbacks.Callback):
#     def __init__(self):
#         self.losses = []
#
#     def on_train_begin(self, logs={}):
#         pass
#
#     def on_batch_end(self, batch, logs={}):
#         self.losses.append(logs.get('loss'))


class DeepCFRSolver(object):
    def __init__(self,
                 root_dict,
                 policy_network_layers=(256, 256),
                 advantage_network_layers=(128, 128),
                 num_iterations: int = 100,
                 num_traversals: int = 100,
                 batch_size_advantage: int = 2048,
                 batch_size_strategy: int = 2048,
                 memory_capacity: int = int(1e6),
                 advantage_networks_path: str = 'advantage_networks',
                 strategy_network_path: str = 'strategy_network'):

        self._batch_size_advantage = batch_size_advantage
        self._batch_size_strategy = batch_size_strategy
        self._policy_network_layers = policy_network_layers
        self._advantage_network_layers = advantage_network_layers
        self._num_players = 2
        self._root_dict = root_dict
        self._embedding_size = 240
        self._num_iterations = num_iterations
        self._num_traversals = num_traversals
        self._num_actions = len(ACTIONS)
        self._advantage_networks_path = advantage_networks_path
        self._strategy_network_path = strategy_network_path
        self._expl = 0.6

        self._adv_networks = []
        self._policy_networks = []

        # 初始化网络
        os.makedirs(self._advantage_networks_path, exist_ok=True)
        os.makedirs(self._strategy_network_path, exist_ok=True)
        for player in range(self._num_players):
            adv_networks = []
            policy_networks = []
            for n in self._root_dict.keys():
                if os.path.exists(self._advantage_networks_path + '/advantage{}-{}.h5'.format(player, n)):
                    network = load_model(self._advantage_networks_path + '/advantage{}-{}.h5'.format(player, n))
                    adv_networks.append(network)
                else:
                    adv_networks.append(
                        AdvantageNetwork(self._embedding_size, self._advantage_network_layers,
                                         self._num_actions))

                if os.path.exists(self._strategy_network_path + '/strategy{}-{}.h5'.format(player, n)):
                    network = load_model(self._strategy_network_path + '/strategy{}-{}.h5'.format(player, n))
                    policy_networks.append(network)
                else:
                    policy_networks.append(PolicyNetwork(self._embedding_size, self._policy_network_layers,
                                                         self._num_actions))

            self._adv_networks.append(adv_networks)
            self._policy_networks.append(policy_networks)

        self._create_memories(memory_capacity)

    def _create_memories(self, memory_capacity):
        """Create memory buffers and associated feature descriptions."""
        self._strategy_memories = [
            [ReservoirBuffer(memory_capacity) for n in self._root_dict.keys()]
            for _ in range(self._num_players)]
        self._advantage_memories = [
            [ReservoirBuffer(memory_capacity) for n in self._root_dict.keys()]
            for _ in range(self._num_players)]

    def solve(self):
        """Solution logic for Deep CFR."""
        for i in range(self._num_iterations):
            for p in range(self._num_players):
                for n in self._root_dict.keys():
                    for nt in range(self._num_traversals):
                        self._traverse_game_tree(self._root_dict[n], p, n, 0, i + 1)

                    self._train_advantage_network(p, n, i + 1)
                    self._save_advantage_network(p, n)
                    if (i+1)%100 == 0:
                        self._save_advantage_data(p, n, i + 1)
                        self._save_strategy_data(p, n, i + 1)
                    if (i+1)%10 == 0:
                        self._train_policy_network(p, n, i + 1)
                        self._save_strategy_network(p, n)

            print('iteration:', i)


    def _train_advantage_network(self, player, n_left,  i):
        data = self._advantage_memories[player][n_left-1].data
        X, y, mask, iterations = np.split(
            data,
            [self._embedding_size, self._embedding_size + self._num_actions,
             self._embedding_size + self._num_actions * 2],
            axis=1)
        self._adv_networks[player][n_left-1].fit(
            [X, mask], y, batch_size=self._batch_size_advantage, sample_weight=iterations.ravel() * 2 / i,
            epochs=10)

    def _train_policy_network(self, player, n_left, i):
        data = self._strategy_memories[player][n_left-1].data
        X, y, mask, iterations = np.split(
            data,
            [self._embedding_size, self._embedding_size + self._num_actions,
             self._embedding_size + self._num_actions * 2],
            axis=1)
        self._policy_networks[player][n_left-1].fit(
            [X, (1 - mask) * 1000000], y, batch_size=self._batch_size_strategy,
            sample_weight=iterations.ravel() * 2 / i, epochs=10)

    def _save_advantage_data(self, player, n_left, iteration):
        os.makedirs('advantage_data', exist_ok=True)
        data = self._advantage_memories[player][n_left-1].data
        np.save('advantage_data/advantage_data{}-{}-{}.npy'.format(player, n_left, iteration), data)

    def _save_advantage_network(self, player, n_left):
        os.makedirs(self._advantage_networks_path, exist_ok=True)
        self._adv_networks[player][n_left-1].save(self._advantage_networks_path + '/advantage{}-{}.h5'.format(player, n_left))

    def _save_strategy_data(self, player, n_left, iteration):
        os.makedirs('strategy_data', exist_ok=True)
        data = self._strategy_memories[player][n_left-1].data
        np.save('strategy_data/strategy{}-{}-{}.npy'.format(player, n_left, iteration), data)

    def _save_strategy_network(self, player, n_left):
        os.makedirs(self._strategy_network_path, exist_ok=True)
        self._policy_networks[player][n_left-1].save(self._strategy_network_path + '/strategy{}-{}.h5'.format(player, n_left))

    def _sample_action_from_advantage(self, state, n_left):
        X = state.info_array.reshape(1, -1)
        mask = state.legal_actions_mask.reshape(1, -1)
        adv = self._adv_networks[state.current_player()][n_left-1].predict([X, mask])[0]
        adv = np.maximum(adv, 0)
        adv[state.legal_actions_mask == 0] = 0
        summed_regret = adv.sum()
        if summed_regret > 0:
            prob = adv / summed_regret
        else:
            prob = np.ones(self._num_actions)
            prob[state.legal_actions_mask == 0] = 0
            prob = prob / prob.sum()
        return adv, prob

    def _traverse_game_tree(self, state, player, n_left, d, i):
        if state.is_terminal():
            # Terminal state get returns.
            return 10. / d if state.player == player else -10. / d
        if state.is_chance_node():
            return self._traverse_game_tree(state.play(), player, n_left, d + 1, i)

        _, strategy = self._sample_action_from_advantage(state, n_left)

        if state.current_player() == player:
            uniform_policy = np.zeros(self._num_actions)
            action_type = [a.split(':')[0] for a in state.legal_actions()]
            for a in state.legal_actions():
                uniform_policy[ACTIONS.index(a)] = 1./action_type.count(a.split(':')[0])
            uniform_policy /= uniform_policy.sum()
            sample_strategy = self._expl * uniform_policy + (1.0 - self._expl) * strategy
        else:
            sample_strategy = strategy

        sample_strategy /= sample_strategy.sum()
        sampled_aidx = np.random.choice(range(self._num_actions), p=sample_strategy)

        child_state = state.play(ACTIONS[sampled_aidx])

        child_value = self._traverse_game_tree(child_state, player, n_left, d + 1, i)

        child_values = np.zeros(self._num_actions, dtype=np.float64)
        child_values[sampled_aidx] = child_value / sample_strategy[sampled_aidx]
        value_estimate = np.sum(strategy * child_values)


        legal_actions_mask = state.legal_actions_mask
        if state.current_player() == player:
            cf_value = value_estimate
            cf_action_value = child_values
            samp_regret = cf_action_value - cf_value
            samp_regret[legal_actions_mask == 0] = 0
            save_regret = np.where(samp_regret > 0, samp_regret, 0.05 * samp_regret)
            self._advantage_memories[player][n_left-1].add(
                np.hstack((state.info_array, save_regret, legal_actions_mask, i))
            )
        else:
            self._strategy_memories[player][n_left-1].add(
                np.hstack((state.info_array, strategy, legal_actions_mask, i))
            )

        return value_estimate

