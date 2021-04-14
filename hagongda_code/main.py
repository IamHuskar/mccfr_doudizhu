import json
from PolicyUsage import get_policy
from datetime import datetime
import time

def get_hand(op_data):
    OwnPokers = op_data['OwnPokers']
    OwnPokers_int = [int(p[:-1]) for p in OwnPokers]
    NotAppeared = op_data['NotAppeared']
    enemy_hand = op_data['enemy_hand']
    public1 = op_data['public1']
    public2 = op_data['public2']
    nleft1 = op_data['nleft1']
    nleft2 = op_data['nleft2']

    policy = get_policy(OwnPokers, NotAppeared, enemy_hand, nleft1, nleft2, public1, public2)
    policy = sorted(policy.items(), key=lambda x: x[1])
    action = policy[-1][0]
    if len(action):
        return action.split('.')
    else:
        return []

def read_cards_info(opid):
    file_path="D:\\share\\"+"op"+str(opid)+".json"
    try:
        f=open(file_path,"r")
        alljs=f.readline()
        alljs=alljs.strip()
        if not alljs.endswith("}"):
            return None
        return json.loads(alljs)
    except Exception as e:
        if "No such file or directory" not in str(e):
            print("read json e "+str(e))
        return None

def read_until_ok(iid):
    while True:
        jObj=read_cards_info(iid)
        if not jObj:
             time.sleep(0.5)
             continue
        break
    return jObj

def change_pks(lst):
    num="0"
    rst=[];
    for ch in lst:
        num=ch[:1] if len(ch)==1 else ch[:2]
        inum=int(num);
        if inum<10:
            rst.append(num);
        elif inum ==10:
            rst.append("10");
        elif inum == 11:
            rst.append("J");
        elif inum == 12:
           rst.append("Q");
        elif inum == 13:
           rst.append("K");
        elif inum == 14:
            rst.append("A");
        elif inum == 16:
            rst.append("2");
        elif inum == 18:
            rst.append("LittleJoker");
        elif inum == 19:
            rst.append("BigJoker");
        else:
            rst.append("NULL");
    return rst
    
def ma():
    print("main called")
    global pre_card
    for i in range(0,1000):
        js=read_until_ok(i)
        print("handle ",i) 
        if js['whosturn']=='wo':
            continue
        if js['enemy_card_num']==0 or js['OwnPokersNum']==0 :
            continue
        now1=datetime.now()
        result=get_hand(js)

        ir=change_pks(result)
        print(result," ========>",ir)
        now2=datetime.now()
        print('use',(now2-now1).seconds)
        continue
if __name__ == '__main__':
    ma()
    '''
    path = 'C:/Users/tobin/Documents/WeChat Files/wxid_0804678046112/FileStorage/File/2021-03/op_log2/'
    for i in range(25):
        f = open(path+'op{}.json'.format(i))
        op_data = json.load(f)
        f.close()
        if op_data['whosturn'] == 'wo':
            print(get_hand(op_data))
     '''