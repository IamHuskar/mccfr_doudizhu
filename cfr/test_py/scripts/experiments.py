import os
import json
import time
import datetime
import sys
FILE_PATH = os.path.dirname(os.path.abspath(__file__))
ROOT_PATH = os.path.abspath(os.path.join(FILE_PATH, '..'))
sys.path.append(ROOT_PATH)
sys.path.insert(0, os.path.join(ROOT_PATH, 'build/Release' if os.name == 'nt' else 'build'))
#from tensorpack.utils.stats import StatCounter
#from tensorpack.utils.utils import get_tqdm
from multiprocessing import *
from datetime import datetime
#from scripts.envs import make_env
from scripts.agents import make_agent
from card import Card, Category, CardGroup, action_space
from mct import mcsearch, CCard, CCardGroup, CCategory, mcsearch_maybecards, getActionspaceCount
from utils import to_char, to_value, get_mask_alter, give_cards_without_minor, \
    get_mask, action_space_single, action_space_pair, get_category_idx, normalize
types = ['RHCP']
os.environ["CUDA_VISIBLE_DEVICES"] = "0"

'''
def eval_episode(env, agent):
    env.reset()
    env.prepare()
    done = False
    r = 0
    print("agent id is "+str(agent.role_id))
    while not done:
        print( "env role id " + str(env.get_role_ID()) )
        if env.get_role_ID() != agent.role_id:

            #手动出牌
            # r, done = env.manual_step_auto()


            #机器出牌
            r, done = env.step_auto()

        else:

            #机器出牌
            #r, done = env.step(agent.intention(env))


            #手动出牌
            r, done = env.step(agent.manual_intention(env))


    if agent.role_id == 1:
        r = -r
    assert r != 0
    return int(r > 0)


def eval_proc(file_name):

    agent = make_agent('MCT',0)
    env = make_env('MCT')
    for _ in range(1):
        winning_rate = eval_episode(env, agent)
'''

def ccardgroup2char(cg):
    return [to_char(int(c) + 3) for c in cg.cards]

def char2ccardgroup(chars):
    cg = CardGroup.to_cardgroup(chars)
    ccg = CCardGroup([CCard(to_value(c) - 3) for c in cg.cards], CCategory(cg.type), cg.value, cg.len)
    return ccg

m={"dizhu_card_num":20,"wode_card_num":17,"unseed_card_num":29,"last_card_group":[],"whosturn":"unknown","mycards":["*","$","2","A","K","Q","J","J","t","9","9","8","8","7","7","6","6"],"unseencards":["5","K","A","2","5","t","Q","K","A","2","5","6","7","8","9","t","J","Q","K","A","2","5","6","7","8","9","t","J","Q"]}


def test3():
    handcards_char = ['$','2','2','K','Q','J','10','9','8','8','8','5','5']
    chandcards = [CCard(to_value(c) - 3) for c in handcards_char]
    unseen_cards = ['K','9','J','Q','K','A','6','7','9','J','Q','K','A','5','6','7','8','9','10','J','Q','*']
    cunseen_cards = [CCard(to_value(c) - 3) for c in unseen_cards]
    last_cg = char2ccardgroup(['5'])
    caction_maybecards = mcsearch_maybecards(chandcards, [], cunseen_cards, 13, last_cg,1,0, 10, 100, 5000, 1, 1)
    intention_maybecards = ccardgroup2char(caction_maybecards)
    #agent = make_agent('RHCP',1)
    #intention_maybecards = agent.search(handcards_char,['5'])
    print(intention_maybecards)
    
'''
{'dizhu_card_num': 15, 'wode_card_num': 8, 'unseed_card_num': 24, 'last_card_group': [], 'whosturn': 'dizhu', 'mycards': ['2', '2', '2', 'J', 'J', 't', 't', '5'], 'unseencards': ['Q', 'A', '9', 'Q', 'A', '5', '6', '8', '9', 't', 'J', 'Q', 'K', 'A', '2', '5', '6', '7', '8', '9', 't', 'J', 'Q', '$']}
'''


def test2():
    now1=datetime.now()
    handcards_char = ['2', '2', '2', 'J', 'J', '10', '10', '5']
    chandcards = [CCard(to_value(c) - 3) for c in handcards_char]
    unseen_cards = ['Q', 'A', '9', 'Q', 'A', '5', '6', '8', '9', '10', 'J', 'Q', 'K', 'A', '2', '5', '6', '7', '8', '9', '10', 'J', 'Q', '$']
    cunseen_cards = [CCard(to_value(c) - 3) for c in unseen_cards]
    last_cg = char2ccardgroup([])   
    caction_maybecards = mcsearch_maybecards(chandcards, [], cunseen_cards, 4, last_cg,1,0, 10, 500, 7000, 1, 1)
    intention_maybecards = ccardgroup2char(caction_maybecards)
    now2=datetime.now()
    print('time spend',(now2-now1).seconds)
    print(intention_maybecards)


def test():
    #['5', '5', '6', '6', '7', '7', '8', '8', '9', '10', 'J', 'J', 'Q', 'Q', 'K', 'K', 'A', '2', '2', '$']
    
    handcards_char = wrap_change_10(["$","2","2","K","Q","J","t","9","8","8","8","5","5"])
    chandcards = [CCard(to_value(c) - 3) for c in handcards_char]
    unseen_cards = wrap_change_10(["K","9","J","Q","K","A","6","7","9","J","Q","K","A","5","6","7","8","9","t","J","Q","*"])
    cunseen_cards = [CCard(to_value(c) - 3) for c in unseen_cards]
    last_cg = char2ccardgroup(['5'])
    #1 shoupai
    #2 kenengde pai zheli chuan null
    #3 weizhipai
    #4 dizhushoupaichu
    #5 shangyishoupai shuzu
    #6 chupairende id
    #7 dangqian big pai de wanjia id zheli xie 0
    #8  15 
    #9  shoupaixipai 100ci
    #10 meicidiedai1000ci
    #11 
    #12 rangpai zhangshu   1
    #0 dizhu 1 wanjia 
    now1=datetime.now()
    '''
    caction_maybecards = mcsearch_maybecards(chandcards, [], cunseen_cards, 20, last_cg,
                                                 1,
                                              0, 30, 200, 6000, 2, 1)
    '''
    print("1111")
    caction_maybecards=mcsearch(chandcards, cunseen_cards, 13, last_cg,
                                                 1,
                                                 0, 8, 20, 100, 2, 1)
    print("2222")
    now2=datetime.now()
    print('use',(now2-now1).seconds)
    intention_maybecards = ccardgroup2char(caction_maybecards)
    print(intention_maybecards)





def wrap_change_10(ll):

    return [c if c!='t' else '10' for c in ll]
    
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

pre_card = []

def handle_json(js):
    handcards_char=wrap_change_10(js['mycards'])
    chandcards = [CCard(to_value(c) - 3) for c in handcards_char]
    unseen_cards = wrap_change_10(js['unseencards'])
    cunseen_cards = [CCard(to_value(c) - 3) for c in unseen_cards]
    last_cg = char2ccardgroup(wrap_change_10(js['last_card_group']))
    
    print("未知牌:"+str(len(unseen_cards)))
    '''
    caction_maybecards = mcsearch_maybecards(chandcards, [], cunseen_cards, js['dizhu_card_num'], last_cg,
                                                 1,
                                                 0, 8, 30, 5000, 2, 1)
    '''
    totalcount=40*(3000-100)*2
    monicishu=50
    mtccishu=2400
    if len(unseen_cards)>15+9:
        monicishu=100 
        mtccishu=2400
    elif len(unseen_cards)>10+9:
        monicishu=250 #1000
        mtccishu=1200
    elif len(unseen_cards)>5+9:
        monicishu=450 # 500
        mtccishu=800
    else:
        monicishu=600 #400
        mtccishu=500
    
    mtccishu=totalcount//monicishu
    
    print("runt",monicishu,mtccishu)
    '''
    vector<Card> self_cards, vector<Card> unseen_cards,
	int next_handcards_cnt, const CardGroup &_pre_group,
	const CardGroup &last_cardgroup, int current_idx, int current_controller,
	int n_threads, int max_d, int max_iter, int weight, int n_r_weight
    '''
    #for c in pre_card:
    #    print(c)
    '''
    caction_maybecards=mcsearch(chandcards, cunseen_cards, js['dizhu_card_num'],pre_card, last_cg,
                                                 1,
                                                 0, 20, 500, mtccishu, 2, 1)
    '''
    
    caction_maybecards=mcsearch(chandcards, cunseen_cards, js['dizhu_card_num'],pre_card, last_cg,
                                                 1,
                                                 0, 20, 10, 200, 2, 1)
    intention_maybecards = ccardgroup2char(caction_maybecards)
    print(intention_maybecards)
    return 

def read_until_ok(iid):
    while True:
        jObj=read_cards_info(iid)
        if not jObj:
             time.sleep(0.5)
             continue
        break
    return jObj


def run():
    test2()


def ma():
    print("main called")
    global pre_card
    for i in range(0,1000):
        js=read_until_ok(i)
        print("handle ",i) 
        if js['whosturn']!='dizhu':
            print(js['last_card_group'])
            pre_card=char2ccardgroup(wrap_change_10(js['last_card_group']))
            #pre_card = [CCard(to_value(c) - 3) for c in www_pre]
            continue
        print(js)
        if js['dizhu_card_num']==0 or js['wode_card_num']==0:
            print("end game ")
            continue
        
        if js['wode_card_num']+js['dizhu_card_num']>14:
            continue
        
        now1=datetime.now()
        handle_json(js)
        now2=datetime.now()
        print('use',(now2-now1).seconds)
        continue

if __name__ == '__main__':
    ii=0
    if ii==1:
        run()
    else:
        ma()
    



