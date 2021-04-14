import json
from Policy import get_action

f = open('C:/Users/tobin/Documents/WeChat Files/wxid_0804678046112/FileStorage/File/2021-03/op_log2/op0.json')
op_data = json.load(f)
f.close()


def get_hand(op_data):
    OwnPokers = op_data['OwnPokers']
    OwnPokers_int = [int(p[:-1]) for p in OwnPokers]
    NotAppeared = op_data['NotAppeared']
    enemy_hand = op_data['enemy_hand']
    public1 = op_data['public1']
    public2 = op_data['public2']
    nleft1 = op_data['nleft1']
    nleft2 = op_data['nleft2']
    player = 0 if nleft1 == 0 else 1  # 地主则为0 农民则为1
    action = get_action(OwnPokers, NotAppeared, enemy_hand, public1, public2, nleft1, nleft2, player)
    if len(action.split(':')[-1]):
        OwnPokers_copy = OwnPokers.copy()
        OwnPokers_int_copy = OwnPokers_int.copy()
        hand_int = [int(p) for p in action.split(':')[-1].split('.')]
        hand = []
        for pInt in hand_int:
            idx = OwnPokers_int_copy.index(pInt)
            OwnPokers_int_copy.pop(idx)
            hand.append(OwnPokers_copy.pop(idx))
    else:
        hand = []
    return hand

if __name__ == '__main__':

    path = 'C:/Users/tobin/Documents/WeChat Files/wxid_0804678046112/FileStorage/File/2021-03/op_log2/'
    for i in range(25):
        f = open(path+'op{}.json'.format(i))
        op_data = json.load(f)
        f.close()
        if op_data['whosturn'] == 'wo':
            print(get_hand(op_data))