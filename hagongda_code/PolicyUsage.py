from doudizhu import DouDiZhu
from game import ChanceState
from algorithms import OutcomeSamplingMCCFR
import time

doudizhu = DouDiZhu(type='all')


def get_policy(OwnPokers, NotAppeared, EnemyHand, n_left1, n_left2, public1, public2, max_iter=10000, max_t=12):
    OwnPokers_int = [int(p[:-1]) for p in OwnPokers]
    EnemyHand_int = [int(p[:-1]) for p in EnemyHand]
    feasible_hand = doudizhu.get_feasible_hand(OwnPokers_int,doudizhu.PokerlistToString(EnemyHand_int))
    if len(feasible_hand) == 1:
        a = feasible_hand[0]
        return {a.split(':')[-1]: 1}

    root = ChanceState(OwnPokers, NotAppeared, EnemyHand, n_left1, n_left2, public1, public2)
    cfr_solver = OutcomeSamplingMCCFR(root)
    t1 = time.time()
    for i in range(max_iter):
        t2 = time.time()
        if t2 - t1 > max_t:
            break
        cfr_solver.iteration()

    incr = cfr_solver.cumulative_incr[root.inf_set()]
    cur_policy = {a.split(':')[-1]: incr[a] / sum(incr.values()) for a in incr}
    return cur_policy


if __name__ == '__main__':
    # 3-14 分别代表 3-10, J, Q, K, A
    # 16, 18, 19 分别代表 2, little_joker, big_joker
    # 将 2 与其他牌分开是为了方便计算顺子

    OwnPokers = ['6S', '6D', '7C', '7H', '9C', '12H', '12D', '13S', '14H', '18J']
    NotAppeared = ['6H', '6C', '7D', '8H', '8D', '9H', '9D', '9S', '10S', '10D', '11S', '11D', '11H',
                   '14S', '14D', '14C', '16S', '16H', '19J']
    enemy_hand = ['5S', '5D']
    public = ['6D', '7C', '7H']
    t1 = time.time()
    policy1 = get_policy(OwnPokers, NotAppeared, enemy_hand, 0, 1, public, [])
    t2 = time.time()
    print(sorted(policy1.items(), key=lambda x: -x[1]))
    print('使用时间为', t2 - t1)
    print('=========================')

    OwnPokers = ['5S', '6C', '7S', '8H', '9C', '12S', '12H', '13C', '14H', '18J']
    NotAppeared = ['6S', '6H', '7C', '8S', '8D', '9D', '9S', '9H', '10S', '10D', '11C', '11S', '11H', '16S']
    enemy_hand = ['6D']
    public = ['9C', '12S', '12H']
    t1 = time.time()
    policy1 = get_policy(OwnPokers, NotAppeared, enemy_hand, 0, 1, public, [])
    t2 = time.time()
    print(sorted(policy1.items(), key=lambda x: -x[1]))
    print('使用时间为', t2 - t1)
    print('=========================')

    OwnPokers = ['8D', '9H', '10C', '11S', '12D', '13D', '14H', '18J']
    NotAppeared = ['6S', '8S', '9C', '9D', '9S', '10H', '10S', '11D', '11C', '11H', '12S', '13H', '14D', '16C']
    enemy_hand = []
    public = ['6C', '7D', '8H']
    t1 = time.time()
    policy1 = get_policy(OwnPokers, NotAppeared, enemy_hand, 1, 0, [], public)
    t2 = time.time()
    print(sorted(policy1.items(), key=lambda x: -x[1]))
    print('使用时间为', t2 - t1)
    print('=========================')

    OwnPokers = ['8D', '8C', '8H', '11S', '11D', '13D', '14H', '18J']
    # ______________________
    NotAppeared = ['6S', '6C', '7D', '9C', '9D', '9S', '10H', '10S', '11C', '11H', '12S', '13H', '14D', '16C']
    enemy_hand = ['5S']
    public = ['13H', '14D', '16C']
    t1 = time.time()
    policy1 = get_policy(OwnPokers, NotAppeared, enemy_hand, 1, 0, [], public)
    t2 = time.time()
    print(sorted(policy1.items(), key=lambda x: -x[1]))
    print('使用时间为', t2 - t1)
    print('=========================')

    OwnPokers = ['8D', '9H', '9S', '10C', '11S', '12D', '14H', '14C']
    # ______________________
    NotAppeared = ['6S', '8S', '9C', '9D', '10H', '10S', '11D', '11C', '11H', '12S', '14D', '16C']
    enemy_hand = ['5D', '5C']
    public = ['6C']
    t1 = time.time()
    policy1 = get_policy(OwnPokers, NotAppeared, enemy_hand, 1, 0, [], public)
    t2 = time.time()
    print(sorted(policy1.items(), key=lambda x: -x[1]))
    print('使用时间为', t2 - t1)
    print('=========================')

    OwnPokers = ['8D', '8S', '8C', '9H', '9S', '9D', '10C', '10S', '14H', '14C', '18J']
    # ______________________
    NotAppeared = ['5D', '6S', '9C', '10H', '10D', '11D', '11C', '11H', '12S', '13H', '14D', '16C', '16D']
    enemy_hand = []
    public = ['6C']
    t1 = time.time()
    policy1 = get_policy(OwnPokers, NotAppeared, enemy_hand, 1, 0, [], public)
    t2 = time.time()
    print(sorted(policy1.items(), key=lambda x: -x[1]))
    print('使用时间为', t2 - t1)
    print('=========================')

    OwnPokers = ['5D', '6H', '8D', '8S', '8C', '9H', '9S', '9D', '10C', '10S', '14H', '14C']
    # ______________________
    NotAppeared = ['6S', '9C', '10H', '10D', '11D', '11H', '12S', '13H', '14D', '16H', '16D']
    enemy_hand = []
    public = ['6H', '8D', '14C']
    t1 = time.time()
    policy1 = get_policy(OwnPokers, NotAppeared, enemy_hand, 0, 1, public, [])
    t2 = time.time()
    print(sorted(policy1.items(), key=lambda x: -x[1]))
    print('使用时间为', t2 - t1)
    print('=========================')

    OwnPokers = ['5D', '6H', '8D', '8S', '8C', '9H', '9S', '9D', '10C', '10S', '14H', '14C']
    # ______________________
    NotAppeared = ['5C', '5H', '5S', '6S', '6H', '9C', '10H', '10D', '11D', '11H', '12S', '13H', '14D', '16H', '16D',
                   '18J']
    enemy_hand = []
    public = ['9D', '10C', '10S']
    t1 = time.time()
    policy1 = get_policy(OwnPokers, NotAppeared, enemy_hand, 0, 1, public, [])
    t2 = time.time()
    print(sorted(policy1.items(), key=lambda x: -x[1]))
    print('使用时间为', t2 - t1)
    print('=========================')

    OwnPokers = ['5D', '6H', '7D', '8C', '9H', '9S', '10C', '10S', '11C', '11H', '12S', '12H', '13H', '14C']
    NotAppeared = ['5C', '5H', '5S', '6S', '6D', '9C', '10H', '10D', '11D', '11S', '12D', '13D', '14D', '16H', '16D',
                   '18J']
    enemy_hand = []
    public = ['12S', '12H', '13H']
    t1 = time.time()
    policy1 = get_policy(OwnPokers, NotAppeared, enemy_hand, 1, 0, [], public)
    t2 = time.time()
    print(sorted(policy1.items(), key=lambda x: -x[1]))
    print('使用时间为', t2 - t1)
    print('=========================')
