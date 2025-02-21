from selenium import webdriver 
from selenium.webdriver.common.by import By
import os
from datetime import datetime
import numpy as np
import random
import matplotlib.pyplot as plt

from array import array

def is_file_empty(file_title):
    return os.stat(file_title).st_size == 0

def scrape_svenska_spel(tips_type: str):
    tips_type = tips_type.lower()
    assert(tips_type in ['europatipset', 'stryktipset'])
    file_title = create_filename_with_date(tipstype=tips_type)
    if file_title not in os.listdir():
        url = f'https://spela.svenskaspel.se/{tips_type}'
        driver = webdriver.Firefox()


        driver.get(url)
        #Accept cookies
        driver.implicitly_wait(5)
        try:
            cookies_button = driver.find_element(by=By.CSS_SELECTOR,value='#onetrust-reject-all-handler')
        except Exception:
            cookies_button = driver.find_element(by=By.CSS_SELECTOR,value='#onetrust-accept-btn-handler')
            
        cookies_button.click()
        driver.implicitly_wait(5)
        main_elem = driver.find_element(by=By.ID, value="tipsen").text
        driver.close()
        betting_info = main_elem.split("\n")
        write_to_file(info_list=betting_info, file_title=file_title)
    
    else:
        print(f"File {file_title} already exists!")

def create_filename_with_date(tipstype:str):
    today_date = datetime.now().strftime("%Y%m%d")
    filename = f"{tipstype.lower()}_Betting_Info_{today_date}.txt"
    return filename

# Function to extract Spelomgång title
def extract_spelomgang_title(info_list):
    for item in info_list:
        if item.startswith('Spelomgång'):
            return item.split(' ', 1)[1]

# Function to write betting info to a file
def write_to_file(info_list, file_title):
    with open(file_title, 'w') as file:
        for item in info_list:
            file.write(f"{item}\n")



def read_betting_info(file_title):
    with open(file_title, 'r') as file:
        betting_info = file.readlines()

    # Process the betting info as needed
    # For example, you can strip newline characters and return the list
    betting_info_list = [line.strip() for line in betting_info]

    return betting_info_list


# Function to extract odds and probabilities for each game
def extract_odds_and_probabilities(info_list):
    odds_dict = {}
    probabilities_dict = {}
    
    
    PIX_index = info_list.index("Fyll i med PIX!")
    spel_guide_index = info_list.index("Spelguide")    
    games_info = info_list[PIX_index+2:spel_guide_index]

    
    print(games_info)
    for counter, elem in enumerate(games_info):
        
        if elem == '-':
            home_team = games_info[counter - 1]
            away_team = games_info[counter + 1]
        if elem == 'Svenska folket':
            probs = list(map(lambda a: float(a.split('%')[0])/100, games_info[counter+1:counter+4]))
        if elem == 'Odds':
            odds = list(map(lambda a: float(a.replace(',', '.')),games_info[counter+1:counter+4]))
        
            odds_dict[away_team + ' vs ' + home_team] = odds
            probabilities_dict[away_team + ' vs ' + home_team] = probs
            
            
    return odds_dict, probabilities_dict

def simulate_games(odds):
    possible_outcomes = [0,1,2]
    odds = list(odds.values())
    rad = 13*[0]
    for game in range(13):
        probs = [1/odds[game][i] for i in range(3)]
        sigma = sum(probs) - 1
        probs = [probs[i] - sigma/3 for i in range(3)] 
        result = np.random.choice(possible_outcomes, 1, p=probs)[0]
        rad[game] = result
    return rad

def generate_pick(picking_probs, odds):
    possible_picks = [0,1,2]
    final_pick = [0]*13
    actual_prob = 1
    odds = list(odds.values())
    for match_cc, picking_prob in enumerate(picking_probs.values()):
        result = np.random.choice(possible_picks, 1, p=picking_prob)[0]
        final_pick[match_cc] = result
        actual_prob *= 1/odds[match_cc][result]
    
    return final_pick, actual_prob

def make_y_pick():
    return [random.choice([0,1,2]) for _ in range(13)]

def number_of_correct_picks(correct_rad: list, pick: list):
    count = sum(x == y for x, y in zip(correct_rad, pick))
    return count

def estimate_EV(best_pick, pool, EV_samples=1000):
    EV = 0
    N = len(pool)
    assert(len(best_pick) > 0 and len(pool) > 0)
    random.seed(47)
    for _ in range(EV_samples):
        opp = random.randint(0, N-1)
        pick = pool[opp]

        prob = pick[1]
        s = number_of_correct_picks(best_pick, pick[0])
        e,l = E_L(s=s, pick=pick, pool=pool)
        
        try:
            EV += prob * (l + np.power(e,(N+1), dtype=np.float64) - np.power(l,(N+1), dtype=np.float64))/e
        except ZeroDivisionError and OverflowError:
            print("Inf expected value:", best_pick)
        
    return N*EV/EV_samples       

def simulate_pool_path(your_pick, pool):
    pass
def E_L(pick, s, pool, E_L_samples=1000):
    import time 
    E_x_s = 0
    L_x_s = 0
    t0 = time.perf_counter()
    for pool_pick in pool:
        prob = pool_pick[1]
        rad = pool_pick[0]
        correct_picks = number_of_correct_picks(correct_rad=pick[0], pick=rad)
        if  correct_picks == s:    
            E_x_s += prob
        if correct_picks < s:
            L_x_s += prob
    t1 = time.perf_counter()
    print(f"E_L took: {t1-t0}. E={E_x_s}, L={L_x_s}")        
    return np.float64(E_x_s), np.float64(L_x_s)

def compute_score_parallell(pick_and_correct_rad:tuple):
    return number_of_correct_picks(correct_rad=pick_and_correct_rad[1], pick=pick_and_correct_rad[0])

def main():


    
    tips_type = 'stryktipset'
    scrape_svenska_spel(tips_type=tips_type)
    file_title = create_filename_with_date(tipstype=tips_type)
    betting_info = read_betting_info(file_title=file_title)
    odds, probs = extract_odds_and_probabilities(betting_info)
    enum_odds = []
    enum_probs = []
    for i, key in enumerate(odds.keys()):
        for i in odds[key]:
            enum_odds.append(i)
        
    for i, key in enumerate(probs.keys()):
        for i in probs[key]:
            enum_probs.append(i)
        
        
        
    
    odds_filename = "odds.bin"
    probs_filename = "probs.bin"

    with open(odds_filename, 'wb') as f:
        array('d', enum_odds).tofile(f)
        

    with open(probs_filename, 'wb') as f:
        array('d', enum_probs).tofile(f)

    print(len(enum_odds))
    
if __name__ == "__main__":
    main()