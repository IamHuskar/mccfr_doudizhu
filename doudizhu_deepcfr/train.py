from game import ChanceState
from deepcfr import DeepCFRSolver

root_dict = {n: ChanceState(0, n) for n in [1, 2, 3, 4, 5]}
deep_cfr_solver = DeepCFRSolver(
    root_dict=root_dict,
    policy_network_layers=(500, 500),
    advantage_network_layers=(500, 500),
    num_iterations=1000,
    num_traversals=5,
    batch_size_advantage=500,
    batch_size_strategy=500,
    memory_capacity=1e6,
    advantage_networks_path='advantage_networks',
    strategy_network_path='strategy_networks'
)
deep_cfr_solver.solve()
print("finish!!!!")
