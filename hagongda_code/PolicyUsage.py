from doudizhu import DouDiZhu
from game import RootChanceGameState
from algorithms import OutcomeSamplingMCCFR
import time


def get_policy(OwnPokers, NotAppeared, EnemyPokerSize, EnemyHand, max_iter=1000,
               ALLOW_THREE_ONE=True, ALLOW_THREE_TWO=False, ALLOW_FOUR_TWO=True):
    """
    :param OwnPokers: 自己的手牌
    :param NotAppeared:  对自己来说还未见的牌
    :param EnemyPokerSize:  对手当前手牌的数目
    :param EnemyHand:  对手最近一次的出牌
    :param max_iter:  MCCFR最大迭代次数
    :param ALLOW_THREE_ONE:  是否允许三带一
    :param ALLOW_THREE_TWO:  是否允许三带二
    :param ALLOW_FOUR_TWO:  是否允许四带二
    :return: 每一个动作的概率,key为出牌动作,value为概率
    """
    doudizhu = DouDiZhu(ALLOW_THREE_ONE, ALLOW_THREE_TWO, ALLOW_FOUR_TWO)
    EnemyHandString = doudizhu.HandtoString(EnemyHand)
    root = RootChanceGameState(doudizhu, OwnPokers, NotAppeared, EnemyPokerSize, EnemyHandString)
    cfr_solver = OutcomeSamplingMCCFR(root)

    for i in range(max_iter):
        cfr_solver.iteration()

    incr = cfr_solver.cumulative_incr[root.inf_set()]
    cur_policy = {a.split(':')[-1]: incr[a] / sum(incr.values()) for a in incr}
    return cur_policy


if __name__ == '__main__':
    # 3-14 分别代表 3-10, J, Q, K, A
    # 16, 18, 19 分别代表 2, little_joker, big_joker
    # 将 2 与其他牌分开是为了方便计算顺子

    # 自己的牌
    OwnPokers1 = [4, 4, 5, 5, 6, 6, 7, 7, 7, 8,
                  8, 9, 10, 10, 12, 12, 13, 14, 16, 18]
    # 对自己来说未出现过的牌
    NotAppeared1 = [3, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 8, 8,
                    9, 9, 9, 10, 10, 11, 11, 11, 11, 12, 12,
                    13, 13, 13, 14, 14, 14, 16, 16, 16, 19]
    # 对手当前的牌数
    EnemyPokerSize1 = 17
    # 对手最近一次的出牌,刚开局或者过牌,则为空列表
    EnemyHand1 = []
    t1 = time.time()
    policy1 = get_policy(OwnPokers1, NotAppeared1, EnemyPokerSize1, EnemyHand1, 1000)
    t2 = time.time()
    print(policy1)
    print('使用时间为', t2 - t1)

    print('=========================')

    OwnPokers2 = [4, 4, 5, 5, 6, 6, 7,
                  9, 12, 12, 13, 14, 18]
    # 对自己来说未出现过的牌
    NotAppeared2 = [3, 3, 3, 3, 4, 6, 6, 7, 8, 8, 9, 9, 9,
                    10, 10, 11, 11, 11, 14, 14, 14, 16, 16, 19]

    # 对手当前的牌数
    EnemyPokerSize2 = 12
    # 对手最近一次的出牌,刚开局或者过牌,则为空列表
    EnemyHand2 = [3, 3]
    t1 = time.time()
    policy2 = get_policy(OwnPokers2, NotAppeared2, EnemyPokerSize2, EnemyHand2, 1000)
    t2 = time.time()
    print(policy2)
    print('使用时间为', t2 - t1)
